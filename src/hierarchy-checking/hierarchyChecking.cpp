#include "hierarchyChecking.h"
#include "compilationUnit.h"
#include "classDecl.h"
#include "error.h"
#include "classDecl.h"
#include "interfaceDecl.h"
#include "methodHeader.h"
#include "constructor.h"
#include "classMethod.h"
#include "interfaceMethod.h"
#include <iostream>
#include <queue>
#include <sstream>
#include <set>
#include <stack>
#include <cassert>

HierarchyChecking::HierarchyChecking(std::map<std::string, std::vector<CompilationTable*> >& packages) : packages(packages) {
    std::vector<CompilationTable*>& types = packages["java.lang"];
    for(unsigned int i = 0; i < types.size(); i++) {
        if(types[i]->getClassOrInterfaceName() == "Object") {
            object = types[i];
            break;
        }
    }
}

CompilationTable* HierarchyChecking::retrieveCompilationOfTypeName(CompilationTable* compilation, Name* name, Token* token) {
    CompilationTable* next_compilation = NULL;
    std::string typeName = name->getNameId()->getIdAsString();

    if (name->isQualifiedName()) {
        std::string qualifier = name->getQualifier();
        for (unsigned int i = 0; i < packages[qualifier].size(); i++) {
            if (packages[qualifier][i]->getClassOrInterfaceName() == typeName) {
                return packages[qualifier][i];
            }
        }
    } else {
        next_compilation = compilation->checkTypePresenceFromSingleImport(typeName);
        if (next_compilation != NULL) { return next_compilation; }

        next_compilation = compilation->checkTypePresenceInPackage(typeName);
        if (next_compilation != NULL) { return next_compilation; }

        next_compilation = compilation->checkTypePresenceFromImportOnDemand(typeName, token);
        if (next_compilation != NULL) { return next_compilation; }

        // Check java.lang if not found
        std::vector<CompilationTable*>::iterator it2;
        for (it2 = packages["java.lang"].begin(); it2 != packages["java.lang"].end(); it2++) {
            if ((*it2)->getClassOrInterfaceName() == typeName) {
                return (*it2);
            }
        }
    }

    std::stringstream ss;
    if (compilation->isClassSymbolTable()) {
        ss << "Class '" << compilation->getClassOrInterfaceName() << "' must include the qualified name"
           << " for '" << typeName << "' or import the package.";
    } else {
       ss << "Interface '" << compilation->getClassOrInterfaceName() << "' must include the qualified name"
          << " for '" << typeName << "' or import the package.";
    }
    Error(E_HIERARCHY, token, ss.str());

    return NULL;
}

void HierarchyChecking::classNotExtendInterface(CompilationTable* compilation) {
    if (compilation->isClassSymbolTable()) {
        SymbolTable* st = compilation->getSymbolTable();

        if (st->isClassTable()) {
            ClassDecl* cd = static_cast<ClassTable*>(st)->getClass();
            Token* token = cd->getClassId()->getToken();

            if (!cd->noSuperClass()) {
                Name* name = cd->getSuper()->getSuperName();
                CompilationTable* processing = retrieveCompilationOfTypeName(compilation, name, token);
                if (processing == NULL) { return; }

                if (processing->getSymbolTable() && !processing->isClassSymbolTable()) {
                    std::stringstream ss;
                    ss << "Class '" << processing->getClassOrInterfaceName() << "' cannot extend"
                       << " from an interface.";
                    Error(E_HIERARCHY, token, ss.str());
                }
            }
        }
    }
}

void HierarchyChecking::duplicateInterface(CompilationTable* compilation) {
    InterfaceList* interface_list = NULL;
    SymbolTable* st = compilation->getSymbolTable();
    Token* token;

    if (compilation->isClassSymbolTable()) {
        ClassDecl* cd = static_cast<ClassTable*>(st)->getClass();
        token = cd->getClassId()->getToken();

        if (!cd->noImplementedInterfaces()) {
            interface_list = cd->getImplementInterfaces();
        }
    } else if (st) {
        InterfaceDecl* id = static_cast<InterfaceTable*>(st)->getInterface();
        token = id->getInterfaceId()->getToken();

        if (!id->noExtendedInterfaces()) {
            interface_list = id->getExtendedInterfaces();
        }
    }

    if (interface_list != NULL) {
        Interfaces* interface = interface_list->getListOfInterfaces();

        std::set<std::string> extendsOrImplements;
        std::pair<std::set<std::string>::iterator,bool> ret;
        while (interface != NULL) {
            Name* name = interface->getCurrentInterface();

            CompilationTable* source = retrieveCompilationOfTypeName(compilation, name, token);
            // Error generated in retrieveCompilationOfTypeName.
            if (source == NULL) { break; }

            ret = extendsOrImplements.insert(source->getCanonicalName());
            if (ret.second == false) {
                std::stringstream ss;
                if (compilation->isClassSymbolTable()) {
                    ss << "Class '" << compilation->getClassOrInterfaceName() << "' cannot implement "
                       << name->getNameId()->getIdAsString() << " multiple times.";
                } else {
                    ss << "Interface '" << compilation->getClassOrInterfaceName() << "' cannot extend "
                       << name->getNameId()->getIdAsString() << " multiple times.";
                }

                Error(E_HIERARCHY, token, ss.str());
                break; 
            }

            interface = interface->getNextInterface();
        }
    }
}

void HierarchyChecking::interfaceNotExtendClass(CompilationTable* compilation) {
    SymbolTable* st = compilation->getSymbolTable();
    Token* token;

    if (st && !compilation->isClassSymbolTable()) {
        InterfaceDecl* id = static_cast<InterfaceTable*>(st)->getInterface();
        token = id->getInterfaceId()->getToken();

        if (!id->noExtendedInterfaces()) {
            Interfaces* interface = id->getExtendedInterfaces()->getListOfInterfaces();

            while (interface != NULL) {
                Name* name = interface->getCurrentInterface();

                CompilationTable* source = retrieveCompilationOfTypeName(compilation, name, token);
                // Error generated in retrieveCompilationOfTypeName.
                if (source == NULL) { break; }

                if (source->isClassSymbolTable()) {
                    std::stringstream ss;
                    ss << "Interface '" << compilation->getClassOrInterfaceName() << "' cannot extend "
                       << "from a class.";

                    Error(E_HIERARCHY, token, ss.str());
                    break;
                }

                interface = interface->getNextInterface();
            }
        }
    }
}

void HierarchyChecking::noDuplicateSignature(CompilationTable* compilation) {
    SymbolTable* st = compilation->getSymbolTable();
    Token* token;

    if (compilation->isClassSymbolTable()) {
        ClassDecl* cd = static_cast<ClassTable*>(st)->getClass();
        token = cd->getClassId()->getToken();

        if (!cd->emptyBody()) {
            ClassBodyStar* cbs = cd->getClassMembers();

            if (!cbs->isEpsilon()) {
                ClassBodyDecls* cbd = cbs->getBody();
                std::set<std::string> method_signatures;
                std::set<std::string> constructor_signatures;
                std::pair<std::set<std::string>::iterator,bool> ret;

                while (cbd != NULL) {
                    if (cbd->isClassMethod()) {
                        MethodHeader* mh = static_cast<ClassMethod*>(cbd)->getMethodHeader();
                        std::string signature = mh->methodSignatureAsString();

                        ret = method_signatures.insert(signature);
                        if (ret.second == false) {
                            std::stringstream ss;
                            ss << "Class '" << compilation->getClassOrInterfaceName() << "' has multiple methods with the signature '"
                               << signature << "'.";

                            Error(E_HIERARCHY, token, ss.str());
                        }
                    }
                    else if(cbd->isConstructor())
                    {
                        std::string signature = static_cast<Constructor*>(cbd)->constructorSignatureAsString();
                        ret = constructor_signatures.insert(signature);
                        if (ret.second == false) {
                            std::stringstream ss;
                            ss << "Class '" << compilation->getClassOrInterfaceName() << "' has multiple constructors with the signature '"
                               << signature << "'.";

                            Error(E_HIERARCHY, token, ss.str());
                        }
                    }

                    cbd = cbd->getNextDeclaration();
                }
            }
        }
    } else if (st) {
        InterfaceDecl* id = static_cast<InterfaceTable*>(st)->getInterface();
        token = id->getInterfaceId()->getToken();

        if (!id->emptyInterfaceBody()) {
            InterfaceBodyStar* ibs = id->getInterfaceBodyStar();

            if (!ibs->isEpsilon()) {
                InterfaceMethod* im = ibs->getInterfaceMethods();
                std::set<std::string> method_signatures;
                std::pair<std::set<std::string>::iterator,bool> ret;
                
                while (im != NULL) {
                    std::string signature = im->methodSignatureAsString();

                    ret = method_signatures.insert(signature);
                    if (ret.second == false) {
                        std::stringstream ss;
                        ss << "Interface '" << compilation->getClassOrInterfaceName() << "' has multiple methods with the signature '"
                           << signature << "'.";

                        Error(E_HIERARCHY, token, ss.str());
                    }
                    
                    im = im->getNextInterfaceMethod();
                }
            }
        }
    }
}

void HierarchyChecking::OverrideChecks(CompilationTable* compilation) {
    std::set<CompilationTable*> visited;
    std::pair<std::set<CompilationTable*>::iterator,bool> visited_ret;
    std::stack<CompilationTable*> traverse;
    traverse.push(compilation);

    std::set<std::string> protected_nonabstract_methods;
    std::map<std::string, bool> protected_abstract_methods;
    std::set<std::string> public_methods;
    std::set<std::string> non_static_methods;
    std::set<std::string> static_methods;
    std::set<std::string> abstract_methods;
    CompilationTable* processing;
    bool initial = true;
    while (!traverse.empty()) {
        processing = traverse.top();
        traverse.pop();

        visited_ret= visited.insert(processing);
        if (visited_ret.second == false || processing == NULL) {
            continue;
        }

        SymbolTable* st = processing->getSymbolTable();
        Token* token;

        if (processing->isClassSymbolTable()) {
            ClassDecl* cd = static_cast<ClassTable*>(st)->getClass();
            token = cd->getClassId()->getToken();

            if (!cd->emptyBody()) {
                ClassBodyStar* cbs = cd->getClassMembers();

                if (!cbs->isEpsilon()) {
                    ClassBodyDecls* cbd = cbs->getBody();

                    while (cbd != NULL) {
                        if (cbd->isClassMethod()) {
                            MethodHeader* mh = static_cast<ClassMethod*>(cbd)->getMethodHeader();
                            std::string signature = mh->methodSignatureAsString();

                            // Check Static vs. Instance Overriding vs. Abstract overriding
                            if (cbd->isStatic()) {
                                static_methods.insert(signature);
                                
                                if (non_static_methods.count(signature)) {
                                    std::stringstream ss;
                                    ss << "Static method '" << signature << "' in class '" << processing->getClassOrInterfaceName()
                                       << "' cannot be overriden as an instance method.";

                                    Error(E_HIERARCHY, token, ss.str());
                                    break;
                                }

                                if(abstract_methods.count(signature)) {
                                    std::stringstream ss;
                                    ss << "Static method '" << signature << "' in class '" << processing->getClassOrInterfaceName()
                                       << "' cannot be overriden as an abstract method.";
                                    Error(E_HIERARCHY, token, ss.str());
                                    break;
                                }
                            } else {
                                non_static_methods.insert(signature);

                                if (static_methods.count(signature)) {
                                    std::stringstream ss;
                                    ss << "Instance method '" << signature << "' in class '" << processing->getClassOrInterfaceName()
                                       << "' cannot be overriden as a static method.";

                                    Error(E_HIERARCHY, token, ss.str());
                                    break;
                                }
                            }

                            // Check Protected vs. Public Overriding
                            if (cbd->isProtected() && cbd->isAbstract()) {
                                protected_abstract_methods[signature] = initial;
                            } 
                            else if(cbd->isProtected() && !cbd->isAbstract())
                            {
                                protected_nonabstract_methods.insert(signature);
                            }
                            else 
                            {
                                public_methods.insert(signature);

                                if (protected_abstract_methods.count(signature) || protected_nonabstract_methods.count(signature)) {
                                    std::stringstream ss;
                                    ss << "Public method '" << signature << "' in class '" << processing->getClassOrInterfaceName()
                                       << "' cannot be overriden as protected.";

                                    Error(E_HIERARCHY, token, ss.str());
                                    break;
                                }
                            }

                            // check Abstract vs. Static overriding
                            if(cbd->isAbstract()) {
                                abstract_methods.insert(signature);
                                if(static_methods.count(signature)) {
                                    std::stringstream ss;
                                    ss << "Abstract method '" << signature << "' in class '" << processing->getClassOrInterfaceName()
                                       << "' cannot be overriden or implemented as a static method.";
                                    Error(E_HIERARCHY, token, ss.str());
                                    break;
                                }
                            }
                        }

                        cbd = cbd->getNextDeclaration();
                    }
                }
            }

            if (!cd->noImplementedInterfaces()) {
                Interfaces* il = cd->getImplementInterfaces()->getListOfInterfaces();

                while (il != NULL) {
                    traverse.push(il->getImplOrExtInterfaceTable());
                    il = il->getNextInterface();
                }
            }
            
            if (!cd->noSuperClass()) {
                traverse.push(cd->getSuper()->getSuperClassTable());
            }

            if (cd->getSuper()->isImplicitlyExtending()) {
                traverse.push(cd->getSuper()->getSuperClassTable());
            }
            
        } else if (st) {
            InterfaceDecl* id = static_cast<InterfaceTable*>(st)->getInterface();

            if (!id->emptyInterfaceBody()) {
                InterfaceBodyStar* ibs = id->getInterfaceBodyStar();
                Token* token = id->getInterfaceId()->getToken();

                if (!ibs->isEpsilon()) {
                    InterfaceMethod* im = ibs->getInterfaceMethods();

                    while (im != NULL) {
                        std::string signature = im->methodSignatureAsString();

                        // Check Protected vs. Public Overriding, by default interface method is public
                        if(public_methods.count(signature) == 0){
                            public_methods.insert(signature);

                            if (protected_nonabstract_methods.count(signature)  || (protected_abstract_methods.count(signature) && protected_abstract_methods[signature] == true)) {
                                std::stringstream ss;
                                ss << "Interface method '" << signature << "' in interface '" << processing->getClassOrInterfaceName()
                                   << "' cannot be overriden as protected.";

                                Error(E_HIERARCHY, token, ss.str());
                                break;
                            }
                        }

                        // Check Abstract vs. Static overriding, by default interface methods are abstract
                        abstract_methods.insert(signature);
                        if(static_methods.count(signature)) {
                            std::stringstream ss;
                            ss << "Interface method '" << signature << "' in interface '" << processing->getClassOrInterfaceName()
                               << "' cannot be overriden or implemented as static.";
                            Error(E_HIERARCHY, token, ss.str());
                            break;
                        }

                        im = im->getNextInterfaceMethod();
                    }
                }
            }

            if (!id->noExtendedInterfaces()) {
                Interfaces* il = id->getExtendedInterfaces()->getListOfInterfaces();

                while (il != NULL) {
                    traverse.push(il->getImplOrExtInterfaceTable());
                    il = il->getNextInterface();
                }
            }
        }
        initial = false;
    }
}

void HierarchyChecking::classNotImplementClass(CompilationTable* compilation){
    TypeDecl *typedecl = compilation->getCompilationUnit()->getTypeDecl();
    if(typedecl->isClass() && !dynamic_cast<ClassDecl*>(typedecl)->noImplementedInterfaces())
    {
        Interfaces *interface = dynamic_cast<ClassDecl*>(typedecl)->getImplementInterfaces()->getListOfInterfaces();
        while(interface != NULL)
        {
            Name *interfaceName = interface->getCurrentInterface();
            CompilationTable* source = retrieveCompilationOfTypeName(compilation, interfaceName, dynamic_cast<ClassDecl*>(typedecl)->getClassId()->getToken());
            assert(source->getClassOrInterfaceName() == interfaceName->getNameId()->getIdAsString());
            if(source->getCompilationUnit()->getTypeDecl()->isClass())
            {
                Error(E_HIERARCHY, interfaceName->getNameId()->getToken(), "error: class cannot implement a class\n");
            }
            interface = interface->getNextInterface();
        }
    }
}

void HierarchyChecking::classNotExtendFinalClass(CompilationTable* compilation){
    TypeDecl *typedecl = compilation->getCompilationUnit()->getTypeDecl();
    if(typedecl->isClass() && !dynamic_cast<ClassDecl*>(typedecl)->noSuperClass())
    {
        Name *superName = dynamic_cast<ClassDecl*>(typedecl)->getSuper()->getSuperName();
        //CompilationTable* source = retrieveCompilationOfTypeName(compilation, superName, dynamic_cast<ClassDecl*>(typedecl)->getClassId()->getToken());
        CompilationTable* source = dynamic_cast<ClassDecl*>(typedecl)->getSuper()->getSuperClassTable();
        assert(source->getClassOrInterfaceName() == superName->getNameId()->getIdAsString());
        //if it's not a class, we will catch that in a different check. Combine checks?
        if(source->getCompilationUnit()->getTypeDecl()->isClass())
        {
            Modifiers *modifiers = dynamic_cast<ClassDecl*>(source->getCompilationUnit()->getTypeDecl())->getClassModifiers();
            while(modifiers != NULL)
            {
                if(modifiers->getCurrentModifierAsString() == "final")
                {
                    Error(E_HIERARCHY, superName->getNameId()->getToken(), "error: class cannot extend a final class\n");
                }
                modifiers = modifiers->getNextModifier();
            }
        }
    }
}

//This check is very similar to the no static overide check
void HierarchyChecking::checkMethodModifiers(CompilationTable* compilation){
    bool checkAbstract = true;
    if(!compilation->isClassSymbolTable() || dynamic_cast<ClassDecl*>(compilation->getCompilationUnit()->getTypeDecl())->isAbstract())
    {
        checkAbstract = false;
    }

    std::set<CompilationTable*> visited;
    std::pair<std::set<CompilationTable*>::iterator,bool> visited_ret;
    std::stack<CompilationTable*> traverse;
    traverse.push(compilation);

    std::map<std::string, std::string> methods;
    CompilationTable* processing;
    while (!traverse.empty()) {
        processing = traverse.top();
        traverse.pop();

        visited_ret= visited.insert(processing);
        if (visited_ret.second == false || processing == NULL) {
            continue;
        }

        SymbolTable* st = processing->getSymbolTable();
        Token* token;

        if (processing->isClassSymbolTable()) {
            ClassDecl* cd = static_cast<ClassTable*>(st)->getClass();
            token = cd->getClassId()->getToken();

            if (!cd->emptyBody()) {
                ClassBodyStar* cbs = cd->getClassMembers();

                if (!cbs->isEpsilon()) {
                    ClassBodyDecls* cbd = cbs->getBody();

                    while (cbd != NULL) {
                        if (cbd->isClassMethod()) {
                            MethodHeader* mh = static_cast<ClassMethod*>(cbd)->getMethodHeader();
                            std::string signature = mh->methodSignatureAsString();
                            if(methods.count(signature) == 1)
                            {
                                if((mh->isVoidReturnType() && methods[signature] != "void") || (!mh->isVoidReturnType() && methods[signature] != mh->getReturnType()->getTypeAsString()))
                                {
                                    std::stringstream ss;
                                    if(mh->isVoidReturnType())
                                    {
                                        ss << "Method '" << signature << "' with return type 'void' in class '" << processing->getClassOrInterfaceName()
                                            << "' cannot be overriden by a method with return type " << methods[signature] << ".";
                                    }
                                    else
                                    {
                                        ss << "Method '" << signature << " with return type " << mh->getReturnType()->getTypeAsString() << "' in class '" << processing->getClassOrInterfaceName()
                                            << "' cannot be overriden by a method with return type " << methods[signature] << ".";
                                    }
                                    Error(E_HIERARCHY, token, ss.str());
                                    break;
                                }
                            }
                            if(methods.count(signature) == 1 && cbd->isFinal())
                            {
                                std::stringstream ss;
                                ss << "Final method '" << signature << "' in class '" << processing->getClassOrInterfaceName()
                                   << "' cannot be overriden.";
                                Error(E_HIERARCHY, token, ss.str());
                                break;
                            }

                            if(checkAbstract) {
                                if(processing == compilation) {
                                    // if it's actually itself being checked
                                    if(cbd->isAbstract()) {
                                        std::stringstream ss;
                                        ss << "Abstract method '" << signature << "' cannot be declared in class '"
                                           << processing->getClassOrInterfaceName() << "' because the class is not abstract.";
                                        Error(E_HIERARCHY, token, ss.str());
                                    }
                                } else if(methods.count(signature) == 0 && cbd->isAbstract()) {
                                    std::stringstream ss;
                                    ss << "Abstract method '" << signature << "' in class '" << processing->getClassOrInterfaceName()
                                       << "' must be overriden.";
                                    Error(E_HIERARCHY, token, ss.str());
                                    break;
                                }
                            }

                            if(mh->isVoidReturnType())
                            {
                                methods[signature] = "void";
                            }
                            else
                            {
                                methods[signature] = mh->getReturnType()->getTypeAsString();
                            }
                        }

                        cbd = cbd->getNextDeclaration();
                    }
                }
            }
            //Do not change the order in which these are added to traverse ***************************************************
            if (!cd->noImplementedInterfaces()) {
                Interfaces* il = cd->getImplementInterfaces()->getListOfInterfaces();

                while (il != NULL) {
                    traverse.push(il->getImplOrExtInterfaceTable());
                    il = il->getNextInterface();
                }
            }  
            
            if (!cd->noSuperClass()) {
                processing = cd->getSuper()->getSuperClassTable();
                if (processing == NULL) { break; }
                traverse.push(processing);
            }
            
            if (cd->getSuper()->isImplicitlyExtending()) {
                traverse.push(cd->getSuper()->getSuperClassTable());
            }
            //****************************************************************
        } else if (st) {
            InterfaceDecl* id = static_cast<InterfaceTable*>(st)->getInterface();

            if (!id->emptyInterfaceBody()) {
                InterfaceBodyStar* ibs = id->getInterfaceBodyStar();
                Token* token = id->getInterfaceId()->getToken();

                if (!ibs->isEpsilon()) {
                    InterfaceMethod* im = ibs->getInterfaceMethods();

                    while (im != NULL) {
                        std::string signature = im->methodSignatureAsString();
                        if(methods.count(signature) == 1)
                        {
                            if((im->isVoidReturnType() && methods[signature] != "void") || (!im->isVoidReturnType() && methods[signature] != im->getReturnType()->getTypeAsString()))
                            {
                                std::stringstream ss;
                                if(im->isVoidReturnType())
                                {
                                    ss << "Method '" << signature << "' with return type  void ' in interface '" << processing->getClassOrInterfaceName()
                                        << "' cannot be overriden by a method with return type " << methods[signature] << ".";
                                }
                                else
                                {
                                    ss << "Method '" << signature << "' with return type " << im->getReturnType()->getTypeAsString() << "' in interface '" << processing->getClassOrInterfaceName()
                                        << "' cannot be overriden by a method with return type " << methods[signature] << ".";
                                }
                                Error(E_HIERARCHY, token, ss.str());
                                break;
                            }
                        }
                        if (methods.count(signature) == 0 && checkAbstract) {
                            std::stringstream ss;
                            ss << compilation->getClassOrInterfaceName() << ": Abstract method '" << signature << "' in interface '" << processing->getClassOrInterfaceName()
                               << "' must be overriden.";
                            Error(E_HIERARCHY, token, ss.str());
                            break;
                        }
                        if(im->isVoidReturnType())
                        {
                            methods[signature] = "void";
                        }
                        else
                        {
                            methods[signature] = im->getReturnType()->getTypeAsString();
                        }

                        im = im->getNextInterfaceMethod();
                    }
                }
            }
            if (!id->noExtendedInterfaces()) {
                Interfaces* il = id->getExtendedInterfaces()->getListOfInterfaces();

                while (il != NULL) {
                    traverse.push(il->getImplOrExtInterfaceTable());
                    il = il->getNextInterface();
                }
            }
            else
            {
                //this is a terrible hack in which we treat interfaces that do not extend anyone as extending the object class
                traverse.push(object);
            }
        }
    }
}

void HierarchyChecking::checkForCycles(CompilationTable* compilation){
    std::map<CompilationTable*, std::set<CompilationTable*> >dependencies;
    std::queue<CompilationTable*> traverse;
    traverse.push(compilation);

    CompilationTable* processing;
    CompilationTable* dependency;
    Token *token;
    while (!traverse.empty()) {
        processing = traverse.front();
        traverse.pop();
        SymbolTable* st = processing->getSymbolTable();
        if (processing->isClassSymbolTable()) {
            ClassDecl* cd = static_cast<ClassTable*>(st)->getClass();
            token = cd->getClassId()->getToken();
            if (!cd->noSuperClass()) {
                dependency = cd->getSuper()->getSuperClassTable();
                if (dependency == NULL) { break; }
                dependencies[processing].insert(dependency);
                if(dependencies.count(dependency) == 0)
                {
                    traverse.push(dependency);
                }
                for(std::map<CompilationTable*, std::set<CompilationTable*> >::iterator it = dependencies.begin(); it != dependencies.end(); it++){
                    if(it->second.count(processing) == 1)
                    {
                        it->second.insert(dependency);
                        if(dependency == it->first)
                        {
                            std::stringstream ss;
                            ss << compilation->getClassOrInterfaceName() << ": cyclical dependency is not allowed";
                            Error(E_HIERARCHY, token, ss.str());
                        }
                    }
                }
            }
            
            if (!cd->noImplementedInterfaces()) {
                Interfaces* il = cd->getImplementInterfaces()->getListOfInterfaces();

                while (il != NULL) {
                    dependency = il->getImplOrExtInterfaceTable();
                    dependencies[processing].insert(dependency);
                    if(dependencies.count(dependency) == 0)
                    {
                        traverse.push(dependency);
                    }
                    for(std::map<CompilationTable*, std::set<CompilationTable*> >::iterator it = dependencies.begin(); it != dependencies.end(); it++){
                        if(it->second.count(processing) == 1)
                        {
                            it->second.insert(dependency);
                            if(dependency == it->first)
                            {
                                std::stringstream ss;
                                ss << compilation->getClassOrInterfaceName() << ": cyclical dependency is not allowed";
                                Error(E_HIERARCHY, token, ss.str());
                            }
                        }
                    }
                    il = il->getNextInterface();
                }
            }  
        }
        else if (st) {
            InterfaceDecl* id = static_cast<InterfaceTable*>(st)->getInterface();
            token = id->getInterfaceId()->getToken();
            if (!id->noExtendedInterfaces()) {
                Interfaces* il = id->getExtendedInterfaces()->getListOfInterfaces();

                while (il != NULL) {
                    dependency = il->getImplOrExtInterfaceTable();
                    dependencies[processing].insert(dependency);
                    if(dependencies.count(dependency) == 0)
                    {
                        traverse.push(dependency);
                    }
                    for(std::map<CompilationTable*, std::set<CompilationTable*> >::iterator it = dependencies.begin(); it != dependencies.end(); it++){
                        if(it->second.count(processing) == 1)
                        {
                            it->second.insert(dependency);
                            if(dependency == it->first)
                            {
                                std::stringstream ss;
                                ss << compilation->getClassOrInterfaceName() << ": cyclical dependency is not allowed";
                                Error(E_HIERARCHY, token, ss.str());
                            }
                        }
                    }
                    il = il->getNextInterface();
                }
            }
        }
        
    }
}

void HierarchyChecking::establishInheritance(CompilationTable* compilation) {
    if(compilation->isClassSymbolTable() && !compilation->isInheritanceEstablished()) {
        // if this is a compilation of a class and it's inheritance has
        // not yet been established, then register the class's own methods and constructors
        compilation->registerClassMethodsAndConstructors();
        ClassDecl* aClass = ((ClassTable*) compilation->getSymbolTable())->getClass();
        if(!aClass->noSuperClass() || aClass->getSuper()->isImplicitlyExtending()) {
            // if there is a superclass either explicitly extended or implicitly
            // recursively establish the superclass's constructors and methods first
            establishInheritance(aClass->getSuper()->getSuperClassTable());
        }

        Interfaces* implInterface = aClass->getImplementInterfaces()->getListOfInterfaces();
        while(implInterface != NULL) {
            // establish inheritance for implemented interface first
            establishInheritance(implInterface->getImplOrExtInterfaceTable());
            implInterface = implInterface->getNextInterface();
        }
        // make sure this class inherits all the methods and fields of its superclass
        // and superinterface, if any
        compilation->inheritClassFieldsAndMethods();
    } else if(!compilation->isClassSymbolTable() && !compilation->isInheritanceEstablished()) {
        // an interface that has not had it's inheritance established
        compilation->registerInterfaceMethods();
        InterfaceDecl* anInterface = ((InterfaceTable*) compilation->getSymbolTable())->getInterface();
        if(!anInterface->noExtendedInterfaces()) {
            Interfaces* extended = anInterface->getExtendedInterfaces()->getListOfInterfaces();
            while(extended != NULL) {
                establishInheritance(extended->getImplOrExtInterfaceTable());
                extended = extended->getNextInterface();
            }
        }
        // make sure this interface inherits all the methods from it's superinterfaces, if any
        compilation->inheritInterfaceMethods(object);
    }
}

void HierarchyChecking::check() {
    std::map<std::string, std::vector<CompilationTable*> >::iterator it;
    std::vector<CompilationTable*>::iterator it2;
    for (it = packages.begin(); it != packages.end(); it++) {
        for (it2 = it->second.begin(); it2 != it->second.end(); it2++) {
            // PLACE CHECKS HERE
            if((*it2)->aTypeWasDefined()) {
                // only do the checks if a type was defined
                classNotImplementClass(*it2);
                classNotExtendInterface(*it2);
                classNotExtendFinalClass(*it2);
                duplicateInterface(*it2);
                interfaceNotExtendClass(*it2);
                noDuplicateSignature(*it2);
                OverrideChecks(*it2);
                checkMethodModifiers(*it2);
                checkForCycles(*it2);
            }

            if (Error::count() > 0) { return; }
        }
    }

    // checks have been done and no error, establish inheritance
    for (it = packages.begin(); it != packages.end(); it++) {
        for (it2 = it->second.begin(); it2 != it->second.end(); it2++) {
            if((*it2)->aTypeWasDefined()) {
                // if a type was defined
                establishInheritance(*it2);
            }
        }
    }
}
