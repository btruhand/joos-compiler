#include <cassert>
#include <iostream>
#include <stack>
#include <algorithm>

// AST
#include "classDecl.h"
#include "classBodyStar.h"
#include "classBodyDecls.h"
#include "fieldDecl.h"
#include "classMethod.h"
#include "constructor.h"
#include "expression.h"
#include "binaryExpression.h"
#include "arrayAccessName.h"
#include "arrayAccessPrimary.h"
#include "primaryExpression.h"
#include "name.h"
#include "bracketedExpression.h"
#include "fieldAccess.h"
#include "methodNormalInvoke.h"
#include "invokeAccessedMethod.h"
#include "newClassCreation.h"
#include "primaryNewArray.h"
#include "primaryExpression.h"
#include "nameExpression.h"
#include "qualifiedThis.h"
#include "assignName.h"
#include "assignField.h"
#include "assignArray.h"
#include "literalOrThis.h"
#include "negationExpression.h"
#include "castName.h"
#include "castPrimitive.h"
#include "instanceof.h"
#include "blockStmtsStar.h"
#include "localDecl.h"
#include "ifStmt.h"
#include "whileStmt.h"
#include "forStmt.h"
#include "returnStmt.h"
#include "stmtExprAssign.h"
#include "stmtExprInvoke.h"
#include "stmtExprCreation.h"
#include "nestedBlock.h"
#include "expressionStar.h"
#include "referenceType.h"

// Symbol table
#include "compilationTable.h"
#include "classTable.h"
#include "paramTable.h"

// Code generation related
#include "codeGenerator.h"
#include "startup.h"
// Managers
#include "vtableManager.h"
#include "objectLayoutManager.h"
#include "inheritanceTableManager.h"
#include "implementedInterfaceMethodTableManager.h"
#include "staticFieldsManager.h"

// Tables and layouts
#include "vtableLayout.h"
#include "objectLayout.h"
#include "implementedInterfaceMethodTable.h"
#include "inheritanceTable.h"
#include "staticFields.h"

// Label
#include "labelManager.h"
#include "RLG.h"

// ------------------------------------------------------------------------------------
// Helper

void CodeGenerator::CALL_FUNCTION(std::string fn_name) {
    std::string constructor_prefix = LabelManager::labelizeForConstructor(processing->getCanonicalName()) + "$";
    std::string method_prefix = processing->getCanonicalName() + "$";

    // If it is a constructor we need to allocate 'this'
    if (fn_name.find(LabelManager::getConstructor()) == 0) {
        // Get canonincal name of type constructor being invoked
        std::string canonical_name = fn_name.substr(LabelManager::getConstructor().size());
        canonical_name = canonical_name.substr(0, canonical_name.find("$"));

        CALL_FUNCTION(LabelManager::labelizeForAlloc(canonical_name));
        asma("push eax ; push this");
    }

    // Check if we need to extern the constructor
    if (fn_name.find(constructor_prefix) != 0 &&
               fn_name.find(method_prefix) != 0) {
        asma("extern " << fn_name);   
    }
    asma("call " << fn_name);

    // If it is a constructor we need to pop 'this'
    if (fn_name.find(LabelManager::getConstructor()) == 0) {
        asma("pop ebx ; pop this");
    }
}

void CodeGenerator::CALL_IDIOM() {
    asma("push ebp");
    asma("mov ebp, esp");
}

void CodeGenerator::RETURN_IDIOM() {
    asma("mov esp, ebp");
    asma("pop ebp");
    asma("ret");
}

void CodeGenerator::createNullForEBX() {
    // Save eax to prevent thrashing
    asma("push eax");
    // Create an array of size 4 * 4
    asma("mov eax, 16");
    CALL_FUNCTION("makeArrayBanana$");

    // insert the inheritance table for char[]
    std::string charArrayInheritance = LabelManager::getLabelForArrayInheritanceTable("char");
    asma("extern " << charArrayInheritance);
    asma("mov [eax - 4], " << charArrayInheritance);
    // Write the word "null" in char array
    asma("mov [eax - 12], 110 ;; n");
    asma("mov [eax - 16], 117 ;; u");
    asma("mov [eax - 20], 108 ;; l");
    asma("mov [eax - 24], 108 ;; l");
    asma("mov ebx, eax");
    asma("pop eax");

    // Save eax to prevent thrashing
    asma("push eax");
    // Push char[] for constructor of java.lang.String
    asma("push ebx");
    // Call constructor for java.lang.String(char.array)
    CALL_FUNCTION(LabelManager::labelizeForConstructor("java.lang.String", 1, "char.array"));
    asma("pop ebx ; get newly created String");
    asma("pop eax ; pop old ebx");
    asma("pop eax ; pop old eax");
}

SymbolTable* CodeGenerator::getSymbolTableForName(Name* name) {
    if (name->isReferringToField()) {
        return name->getReferredField();
    } else if (name->isReferringToParameter()) {
        return name->getReferredParameter();
    } else if (name->isReferringToLocalVar()) {
        return name->getReferredLocalVar();
    } else {
        assert(false);
    }

    return NULL;
}

void CodeGenerator::setParameterOffsetFromEBP(ParamList* params, int start_offset) {
    // Param List is right to left, but we need to increment by left to right
    // So flip it
    std::stack<ParamList*> paramStack;
    unsigned int param_offset = start_offset;
    while (params != NULL) {
        paramStack.push(params);
        params = params->getNextParameter();
    }

    while (!paramStack.empty()) {
        params = paramStack.top();
        paramStack.pop();

        addressTable[params->getParamTable()] = param_offset;
        param_offset += 4;
    }
}

void CodeGenerator::callInitializersOfDeclaredFields() {
    // Call fields initializers here
    SymbolTable* symTable = processing->getSymbolTable()->getNextTable();
    while(symTable != NULL) {
        if(symTable->isFieldTable()) {
            if(!((FieldTable*) symTable)->getField()->isStatic()) {
                // a non-static field, call it's initializer
                asma("call " << (((FieldTable*) symTable)->generateFieldInitializerLabel()));
            }
        }
        symTable = symTable->getNextTable();
    }
}


void CodeGenerator::exceptionCall() {
    asma("extern __exception");
    asma("call __exception");
}
// -------------------------------------------------------------------------------------
// Real deal

CodeGenerator::CodeGenerator(std::map<std::string, CompilationTable*>& compilations, CompilationTable* firstUnit) :
    compilations(compilations),
    starter(new Startup(compilations, firstUnit)),
    virtualManager(new VTableManager(compilations)),
    objManager(new ObjectLayoutManager(compilations)),
    inhManager(new InheritanceTableManager(compilations)),
    interManager(new ImplInterfaceMethodTableManager(compilations)),
    staticManager(new StaticFieldsManager(compilations)),
    fs(NULL),
    scope_offset(0)
{}

CodeGenerator::~CodeGenerator() {
    delete starter;
    delete virtualManager;
    delete objManager;
    delete inhManager;
    delete interManager;
    delete staticManager;
}

void CodeGenerator::generateStartFile() {
    // generate _startup.s
    std::vector<InheritanceTable*> arrayInheritance;
    std::vector<StaticFields*> statics;
    std::map<std::string, CompilationTable*>::iterator it;
    for(it = compilations.begin(); it != compilations.end(); it++) {
        if(it->second->aTypeWasDefined()) {
            // for each type defined, get their corresponding array type
            arrayInheritance.push_back(inhManager->getTableForType(it->second->getCanonicalName() + "[]"));
            
            if(it->second->isClassSymbolTable()) {
                // a class, grab all it's static methods
                statics.push_back(staticManager->getStaticFieldsForClass(it->second));
            }
        }
    }
    // get the array types for primitive types
    arrayInheritance.push_back(inhManager->getTableForType("int[]"));
    arrayInheritance.push_back(inhManager->getTableForType("short[]"));
    arrayInheritance.push_back(inhManager->getTableForType("byte[]"));
    arrayInheritance.push_back(inhManager->getTableForType("char[]"));
    arrayInheritance.push_back(inhManager->getTableForType("boolean[]"));

    starter->generateStartupFile(virtualManager->getVTableLayoutForArray(), arrayInheritance,
                interManager->getTableForArray(), statics);
}

// --------------------------------------------------------------------------------
// Code generation section

void CodeGenerator::traverseAndGenerate() {
    std::map<std::string, CompilationTable*>::iterator it;
    for(it = compilations.begin(); it != compilations.end(); it++) {
        if(it->second->aTypeWasDefined() && it->second->isClassSymbolTable()) {
            // a type was defined and it's a class
            std::string classCanonicalName = it->second->getCanonicalName();
#if defined(CODE_OUT)
            std::stringstream ss;
            ss << CODE_OUT << "/" << classCanonicalName << ".s";
            fs = new std::ofstream(ss.str());
#else
            fs = new std::ofstream(classCanonicalName + ".s");
#endif
            processing = it->second;

            // data section
            section("data");

            // expose all static fields declared within the class
            // initialize all of them to default values
            std::vector<FieldTable*>& staticFields = staticManager->getStaticFieldsForClass(processing)->getAllStaticFieldsOfClass();
            for(unsigned int i = 0; i < staticFields.size(); i++) {
                asmgl(staticFields[i]->generateFieldLabel());
                asma("dd " <<  0);
            }
            
            // Make the virtual, inheritance and interface method table for this class
            virtualManager->getVTableLayoutForType(classCanonicalName)->outputVTableToFile(*fs);
            inhManager->getTableForType(classCanonicalName)->outputInheritanceTableToFile(*fs);
            interManager->getTableForType(classCanonicalName)->outputImplInterfaceMethodTableToFile(*fs);

            // text section
            section("text");

            asml(LabelManager::labelizeForAlloc(classCanonicalName));
            asma("mov eax, " << objManager->getLayoutForClass(processing)->sizeOfObject());

            CALL_FUNCTION("__malloc");
            asma("mov [eax], " << virtualManager->getVTableLayoutForType(classCanonicalName)->getVirtualTableName());
            asma("mov [eax-4], " << inhManager->getTableForType(classCanonicalName)->generateInheritanceTableName());
            asma("mov [eax-8], " << interManager->getTableForType(classCanonicalName)->generateTableName());
            asma("push eax ; push allocated object onto the stack as 'this'");
            
            ObjectLayout* layoutOfClass = objManager->getLayoutForClass(processing);
            asmc("Initialize all fields to their default 0");
            for(unsigned int i = 0; i < layoutOfClass->numberOfFieldsInObject(); i++) {
                asma("mov [eax - " << ObjectLayout::transformToFieldIndexInAClass(i) << "], 0");
            }
            
            asma("ret ; get back to whoever called this allocator, with eax pointing to the new object");
            traverseAndGenerate(((ClassTable*)it->second->getSymbolTable())->getClass());
            delete fs;
        }
    }
}

void CodeGenerator::traverseAndGenerate(ClassDecl* aClass) {
    traverseAndGenerate(aClass->getClassMembers());
}

void CodeGenerator::traverseAndGenerate(ClassBodyStar* body) {
    if(!body->isEpsilon()) {
        traverseAndGenerate(body->getBody());
    }
}

void CodeGenerator::traverseAndGenerate(ClassBodyDecls* body) {
    if(!body->isLastClassMember()) {
        traverseAndGenerate(body->getNextDeclaration());
    }

    if(body->isField()) {
        traverseAndGenerate((FieldDecl*) body);
    } else if(body->isClassMethod()) {
        traverseAndGenerate((ClassMethod*) body);
    } else {
        traverseAndGenerate((Constructor*) body);
    }
}

void CodeGenerator::traverseAndGenerate(FieldDecl* field) {
    if(field->isInitialized()) {
        std::string initializerLabel = field->getFieldTable()->generateFieldInitializerLabel();
        bool isStatic = field->isStatic();

        asmc("Field initializer for " << initializerLabel);
        
        if(isStatic) {
            // static field, initializer needs to be globaled
            asmc("Initializer of static needs to be globalled");
            asmgl(initializerLabel);
        } else {
            asml(initializerLabel);
        }

        unsigned int indexOfField = 0;
        if(!isStatic) {
            // not static field, 
            // then need to have the call idiom
            CALL_IDIOM();
            // non-static get the index of the field in the object
            indexOfField = objManager->getLayoutForClass(processing)->indexOfFieldInObject(field->getFieldTable());
        }

        traverseAndGenerate(field->getInitializingExpression());
        asmc("Initialize field with it's initializer value");
        if(isStatic) {
            asma("mov [" << field->getFieldTable()->generateFieldLabel() << "], eax");
        } else {
            asma("mov ebx, [ebp + 8] ; get this");
            asma("mov [ebx - " << indexOfField << "], eax");
        }

        if(isStatic) {
            asma("ret ; just return from static initializer");
        } else {
            RETURN_IDIOM();
        }
    }
}

void CodeGenerator::traverseAndGenerate(Expression* expr) {
    if(expr->isRegularBinaryExpression()) {
        traverseAndGenerate((BinaryExpression*) expr);
    } else if(expr->isPrimaryExpression()) {
        traverseAndGenerate(((PrimaryExpression*) expr)->getPrimaryExpression());
    } else if(expr->isNameExpression()) {
        traverseAndGenerate(((NameExpression*) expr)->getNameExpression());
    } else if(expr->isNumericNegation() || expr->isBooleanNegation()) {
        traverseAndGenerate((NegationExpression*) expr);
    } else if(expr->isCastToArrayName() || expr->isCastToReferenceType() || expr->isCastToPrimitiveType()) {
        traverseAndGenerate((CastExpression*) expr);
    } else if(expr->isInstanceOf()) {
        traverseAndGenerate((InstanceOf*) expr);
    } else {
        // assignment
        traverseAndGenerate((Assignment*) expr);
    }
}

void CodeGenerator::traverseAndGenerate(BinaryExpression* binExpr) {
    Expression* lhs_expr = binExpr->getLeftExpression();
    Expression* rhs_expr = binExpr->getRightExpression();
    std::string lhs_type = lhs_expr->getExpressionTypeString();
    std::string rhs_type = rhs_expr->getExpressionTypeString();

    // Order based on JLS 15.7.2, except || (azy or) and && (lazy and)
    traverseAndGenerate(lhs_expr);
    if(!binExpr->isLazyOr() && binExpr->isLazyAnd()) {
        asma("push eax");
        traverseAndGenerate(rhs_expr);
        asma("mov ebx, eax ; put RHS value into ebx");
        asma("pop eax ; put LHS value into eax");
    }

    if(binExpr->isLazyOr()) {
        // Order based on JLS 15.23

        // If LHS is true, then jump
        std::string lbl = LABEL_GEN();
        asmc("Lazy OR expr");
        asma("cmp eax, 1");
        asma("je " << lbl);

        // Else, evaluate RHS
        traverseAndGenerate(rhs_expr);

        asml(lbl);
    } else if(binExpr->isLazyAnd()) {
        // Order based on JLS 15.24
        std::string lbl = LABEL_GEN();

        // If LHS is false, then jump
        asmc("Lazy AND expr");
        asma("cmp eax, 0");
        asma("je " << lbl);

        // Else, evaluate RHS
        traverseAndGenerate(rhs_expr);

        asml(lbl);
    } else if(binExpr->isEagerOr()) {
        // Specific: JLS 15.22.2
        asmc("Eager OR expr");
        asma("or eax, ebx");
    } else if(binExpr->isEagerAnd()) {
        // Specific: JLS 15.22.2
        asmc("Eager AND expr");
        asma("and eax, ebx");
    } else if(binExpr->isEqual()) {
        // Specific: JLS 15.21
        std::string lbl_true = LABEL_GEN();
        std::string lbl_end = LABEL_GEN();

        asmc("EQUAL expr");
        asma("cmp eax, ebx");
        // If equal, jump to true and set 1
        asma("je " << lbl_true);
        // Else set 0
        asma("mov eax, 0");
        asma("jmp " << lbl_end);
        
        // Set to 1 since true
        asml(lbl_true);
        asma("mov eax, 1");

        // End
        asml(lbl_end);
    } else if(binExpr->isNotEqual()) {
        // Specific: JLS 15.21
        std::string lbl_true = LABEL_GEN();
        std::string lbl_end = LABEL_GEN();

        asmc("NOT EQUAL expr");
        asma("cmp eax, ebx");
        // If not equal, jump to true and set 1
        asma("jne " << lbl_true);
        // Else set 0
        asma("mov eax, 0");
        asma("jmp " << lbl_end);
        
        // Set to 1 since true
        asml(lbl_true);
        asma("mov eax, 1");

        // End
        asml(lbl_end);
    } else if(binExpr->isLT()) {
        // Specific: JLS 15.20
        std::string lbl_true = LABEL_GEN();
        std::string lbl_end = LABEL_GEN();

        asmc("LESS THAN expr");
        asma("cmp eax, ebx");
        // If less, jump to true and set 1
        asma("jl " << lbl_true);
        // Else set 0
        asma("mov eax, 0");
        asma("jmp " << lbl_end);
        
        // Set to 1 since true
        asml(lbl_true);
        asma("mov eax, 1");

        // End
        asml(lbl_end);
    } else if(binExpr->isLTE()) {
        // Specific: JLS 15.20 
        std::string lbl_true = LABEL_GEN();
        std::string lbl_end = LABEL_GEN();

        asmc("LESS THAN OR EQUAL expr");
        asma("cmp eax, ebx");
        // If less than or equal, jump to true and set 1
        asma("jle " << lbl_true);
        // Else set 0
        asma("mov eax, 0");
        asma("jmp " << lbl_end);
        
        // Set to 1 since true
        asml(lbl_true);
        asma("mov eax, 1");

        // End
        asml(lbl_end);
    } else if(binExpr->isGT()) {
        // Specific: JLS 15.20
        std::string lbl_true = LABEL_GEN();
        std::string lbl_end = LABEL_GEN();

        asmc("GREATER THAN expr");
        asma("cmp eax, ebx");
        // If greater, jump to true and set 1
        asma("jg " << lbl_true);
        // Else set 0
        asma("mov eax, 0");
        asma("jmp " << lbl_end);
        
        // Set to 1 since true
        asml(lbl_true);
        asma("mov eax, 1");

        // End
        asml(lbl_end);
    } else if(binExpr->isGTE()) {
        // Specific: JLS 15.20
        std::string lbl_true = LABEL_GEN();
        std::string lbl_end = LABEL_GEN();

        asmc("GREATER THAN OR EQUAL expr");
        asma("cmp eax, ebx");
        // If greater than or equal, jump to true and set 1
        asma("jge " << lbl_true);
        // Else set 0
        asma("mov eax, 0");
        asma("jmp " << lbl_end);
        
        // Set to 1 since true
        asml(lbl_true);
        asma("mov eax, 1");

        // End
        asml(lbl_end);
    } else if(binExpr->isAddition()) {
        // Specific: JLS 15.18
        if (lhs_type != "java.lang.String" &&
            rhs_type != "java.lang.String") {
            asmc("ADD expr");
            asma("add eax, ebx");
        } else if ((lhs_type == "java.lang.String" && rhs_type != "java.lang.String") &&
                   (lhs_type != "java.lang.String" && rhs_type == "java.lang.String")) {
            // Fake it and swap lhs with rhs
            if (lhs_type != "java.lang.String" && rhs_type == "java.lang.String") {
                Expression* temp = lhs_expr;
                lhs_expr = rhs_expr;
                rhs_expr = temp;

                asma("mov edx, eax");
                asma("mov eax, ebx");
                asma("mov ebx, edx");
            }

            // Primitive Type
            // - Create object for primitive, then call toString
            if (rhs_expr->isExprTypeInt()) {
                asmc("CONCAT string with int primitive");
                // Save eax to prevent thrashing
                asma("push eax");
                // Push integer for constructor parameter
                asma("push ebx");
                // Call Constructor for java.lang.Integer
                CALL_FUNCTION(LabelManager::labelizeForConstructor("java.lang.Integer", 1, "int"));
                asma("pop ebx ; get newly created Integer");
                asma("pop eax ; get old ebx");
                asma("pop eax ; get old eax");

                // Save eax to prevent thrashing
                asma("push eax");
                // Push integer for toString (this)
                asma("push ebx");
                // Call toString on java.lang.Integer
                CALL_FUNCTION("java.lang.Integer.toString$$");
                asma("pop ebx");
                asma("mov ebx, eax");
                asma("pop eax");
            } else if (rhs_expr->isExprTypeShort()) {
                asmc("CONCAT string with short primitive");
                // Save eax to prevent thrashing
                asma("push eax");
                // Push integer for constructor parameter
                asma("push ebx");
                // Call Constructor for java.lang.Short
                CALL_FUNCTION(LabelManager::labelizeForConstructor("java.lang.Short", 1, "short"));
                asma("pop ebx ; get newly created Short");
                asma("pop eax ; get old ebx");
                asma("pop eax ; get old eax");

                // Save eax to prevent thrashing
                asma("push eax");
                // Push short for toString (this)
                asma("push ebx");
                // Call toString on java.lang.Short
                CALL_FUNCTION("java.lang.Short.toString$$");
                asma("pop ebx");
                asma("mov ebx, eax");
                asma("pop eax");
            } else if (rhs_expr->isExprTypeByte()) {
                asmc("CONCAT string with byte primitive");
                // Save eax to prevent thrashing
                asma("push eax");
                // Push integer for constructor parameter
                asma("push ebx");
                // Call Constructor for java.lang.Byte
                CALL_FUNCTION(LabelManager::labelizeForConstructor("java.lang.Byte", 1, "byte"));
                asma("pop ebx ; get newly created Byte");
                asma("pop eax ; get old ebx");
                asma("pop eax ; get old eax");

                // Save eax to prevent thrashing
                asma("push eax");
                // Push short for toString (this)
                asma("push ebx");
                // Call toString on java.lang.Byte
                CALL_FUNCTION("java.lang.Byte.toString$$");
                asma("pop ebx");
                asma("mov ebx, eax");
                asma("pop eax");
            } else if (rhs_expr->isExprTypeChar()) {
                asmc("CONCAT string with character primitive");
                // Save eax to prevent thrashing
                asma("push eax");
                // Push integer for constructor parameter
                asma("push ebx");
                // Call Constructor for java.lang.Character
                CALL_FUNCTION(LabelManager::labelizeForConstructor("java.lang.Character", 1, "char"));
                asma("pop ebx ; get newly created Character");
                asma("pop eax ; get old ebx");
                asma("pop eax ; get old eax");

                // Save eax to prevent thrashing
                asma("push eax");
                // Push short for toString (this)
                asma("push ebx");
                // Call toString on java.lang.Character
                CALL_FUNCTION("java.lang.Character.toString$$");
                asma("pop ebx");
                asma("mov ebx, eax");
                asma("pop eax");
            } else if (rhs_expr->isExprTypeBoolean()) {
                asmc("CONCAT string with boolean primitive");
                // Save eax to prevent thrashing
                asma("push eax");
                // Push integer for constructor parameter
                asma("push ebx");
                // Call Constructor for java.lang.Boolean
                CALL_FUNCTION(LabelManager::labelizeForConstructor("java.lang.Boolean", 1, "boolean"));
                asma("pop ebx ; get newly created Boolean");
                asma("pop eax ; get old ebx");
                asma("pop eax ; get old eax");

                // Save eax to prevent thrashing
                asma("push eax");
                // Push short for toString (this)
                asma("push ebx");
                // Call toString on java.lang.Boolean
                CALL_FUNCTION("java.lang.Boolean.toString$$");
                asma("pop ebx");
                asma("mov ebx, eax");
                asma("pop eax");
            } else {
                // Reference Type and null type
                asmc("CONCAT string with reference type");
                // Save eax to prevent thrashing
                asma("push eax");
                // Push object for valueOf parameter
                asma("push ebx");
                // Call valueOf function in java.lang.String
                CALL_FUNCTION("java.lang.String.valueOf$java.lang.Object$");
                asma("pop ebx");
                asma("mov ebx, eax");
                asma("pop eax");

                // Check if returned string is null
                std::string non_null_lbl = LABEL_GEN();
                asma("cmp ebx, 0");
                asma("jne " <<  non_null_lbl);
                
                asmc("If previous attempt returns null, make the 'null' string");
                asma("push eax");
                asma("push ebx");
                CALL_FUNCTION("java.lang.String.valueOf$java.lang.Object$");
                asma("pop ebx");
                asma("mov ebx, eax");
                asma("pop eax");

                asml(non_null_lbl);
            }

            if (lhs_type != "java.lang.String" && rhs_type == "java.lang.String") {
                // Push the real rhs string as a parameter
                asma("push eax");
                // Push the real lhs string as (this)
                asma("push ebx");
            } else {
                // Push new integer string as a parameter
                asma("push ebx");
                // Push lhs string as (this)
                asma("push eax");
            }
            // Call concat
            CALL_FUNCTION("java.lang.Integer.concat$java.lang.String$");
            asma("pop ebx");
            asma("pop ebx");
        } else {
            asmc("CONCAT two strings");
            // Push new integer string as a parameter
            asma("push ebx");
            // Push lhs string as (this)
            asma("push eax");
            // Call concat
            CALL_FUNCTION("java.lang.Integer.concat$java.lang.String$");
            asma("pop ebx");
            asma("pop ebx");
        }
    } else if(binExpr->isMinus()) {
        // Specific: JLS 15.18
        asmc("SUBTRACT expr");
        asma("sub eax, ebx");
    } else if(binExpr->isMultiplication()) {
        // Specific: JLS 15.17
        asmc("MULTIPLY expr");
        asma("imul eax, ebx");
    } else if(binExpr->isDivision()) {
        // Specific: JLS 15.17
        std::string lbl_valid_div = LABEL_GEN();

        asmc("DIVIDE expr");
        asma("cmp ebx, 0");
        asma("jne " << lbl_valid_div);
        exceptionCall();
        asml(lbl_valid_div);
        asma("cdq");
        asma("idiv ebx");
    } else if (binExpr->isModulo()) {
        // Specific: JLS 15.17
        // TODO: Check that yea.
        std::string lbl_valid_div = LABEL_GEN();

        asmc("MODULO expr");
        asma("cmp ebx, 0");
        asma("jne " << lbl_valid_div);
        exceptionCall();
        asml(lbl_valid_div);
        asma("cdq");
        asma("idiv ebx");
        asma("mov eax, edx");
    } else {
        assert(false);
    }
}

void CodeGenerator::traverseAndGenerate(PrimaryExpression* primExpr) {
    traverseAndGenerate(primExpr->getPrimaryExpression());
}

void CodeGenerator::traverseAndGenerate(Primary* prim) {
    if(prim->isArrayAccessName() || prim->isArrayAccessPrimary()) {
        traverseAndGenerate((ArrayAccess*) prim);
    } else if(prim->isBracketedExpression()) {
        traverseAndGenerate(((BracketedExpression*) prim)->getExpressionInside());
    } else if(prim->isFieldAccess()) {
        traverseAndGenerate((FieldAccess*) prim);
    } else if(prim->isNormalMethodCall() || prim->isAccessedMethodCall()) {
        traverseAndGenerate((MethodInvoke*) prim);
    } else if(prim->isNewClassCreation()) {
        traverseAndGenerate((NewClassCreation*) prim);
    } else if(prim->isNewPrimitiveArray() || prim->isNewReferenceArray()) {
        traverseAndGenerate((PrimaryNewArray*) prim);
    } else if(prim->isQualifiedThis()) {
        traverseAndGenerate((QualifiedThis*) prim);
    } else {
        // LiteralOrThis
        traverseAndGenerate((LiteralOrThis*) prim);
    }
}

void CodeGenerator::traverseAndGenerate(ArrayAccess* access) {
    // Order based on JLS 15.13
    traverseAndGenerate(access->getAccessExpression());

    asma("push eax ; push the result of the index access");
    if(access->isArrayAccessName()) {
        traverseAndGenerate(((ArrayAccessName*) access)->getNameOfAccessedArray());
    } else {
        traverseAndGenerate(((ArrayAccessPrimary*) access)->getAccessedPrimaryArray());
    }

    asma("cmp eax, 0 ; check if value of array is null or not");
    std::string null_lbl_chk = LABEL_GEN();
    asma("jne " << null_lbl_chk << " ; if array is not null");
    exceptionCall();
    
    asml(null_lbl_chk);
    std::string exceptional_index_access = LABEL_GEN();
    std::string proper_index_access = LABEL_GEN();
    asma("pop ebx ; retrieve index access back from stack");
    asma("cmp ebx, 0 ; see if index is less than 0 or not");
    asma("jl " << exceptional_index_access);
    asma("cmp ebx, [eax + 4] ; see if specified index is greater than or equal to length of the array");
    asma("jge " << exceptional_index_access);
    asma("jmp " << proper_index_access << " ; everything is fine");
    asml(exceptional_index_access);
    exceptionCall();
    asml(proper_index_access);
    asma("neg ebx ; negate index");
    asma("mov eax, [eax + 4*ebx] ; get the value of the array at the specified index multiplied by 4");
}

void CodeGenerator::traverseAndGenerate(Name* name, CompilationTable** prevTypeForName) {
    // Order implicit based on JLS 15.7
    if (!name->isLastPrefix()) {
        CompilationTable* prevType = NULL;
        traverseAndGenerate(name->getNextName(), &prevType);

        Name* qualifier = name->getNextName();

        if (name->isReferringToField()) {
            assert(prevType != NULL);

            FieldTable* field = name->getReferredField();
            if(qualifier->isReferringToType()) {
                std::string staticName = field->generateFieldLabel();

                asmc("Accessing static variable, get global variable");
                if(prevType != processing) {
                    asma("extern " << staticName);
                }
                asma("mov eax, " << staticName);
            } else {
                std::string null_chk_lbl = LABEL_GEN();

                asmc("Accessing non-statically. Assumption is eax has 'this' if we are here");
                asma("cmp eax, 0");
                asma("jne " << null_chk_lbl);
                CALL_FUNCTION("__exception");
                asml(null_chk_lbl);
                asma("mov eax, [eax - " << objManager->getLayoutForClass(prevType)->indexOfFieldInObject(field) << "]");
            }

            Type* fieldType = name->getReferredField()->getField()->getFieldType();
            if (fieldType->isReferenceType()) {
                *prevTypeForName = ((ReferenceType*) fieldType)->getReferenceTypeTable();
            } //  ELSE IS PRIMITIVE TYPE, BETTER NOT BE A PREFIX
        } else if(name->isReferringToType()) {
            *prevTypeForName = name->getReferredType();
        } // ELSE IS PACKAGE/FIELD DECL/PARAM <- the last two is impossible. We hope.
    } else {
        if (name->isReferringToField()) {
            FieldTable* field = name->getReferredField();
            asma("mov eax, [ebp + 8] ; get this");
            asma("mov eax, [eax - " << objManager->getLayoutForClass(processing)->indexOfFieldInObject(field) << "]");

            Type* fieldType = field->getField()->getFieldType();
            if (fieldType->isReferenceType() && prevTypeForName != NULL) {
                *prevTypeForName = ((ReferenceType*) fieldType)->getReferenceTypeTable();
            } //  ELSE IS PRIMITIVE TYPE
        } else if(name->isReferringToParameter() || name->isReferringToLocalVar()) {
            SymbolTable* paramOrLocal = getSymbolTableForName(name);
            if (name->isReferringToParameter()) {
                Type* paramType = name->getReferredParameter()->getParameter()->getParameterType();

                if (paramType->isReferenceType() && prevTypeForName != NULL) {
                    *prevTypeForName = static_cast<ReferenceType*>(paramType)->getReferenceTypeTable();
                } //  ELSE IS PRIMITIVE TYPE
            } else {
                Type* localType = name->getReferredLocalVar()->getLocalDecl()->getLocalType();

                if (localType->isReferenceType() && prevTypeForName != NULL) {
                    *prevTypeForName = static_cast<ReferenceType*>(localType)->getReferenceTypeTable();
                } //  ELSE IS PRIMITIVE TYPE
            }

            asma("mov eax, [ebp + " << addressTable[paramOrLocal] << "]");
        } else if (name->isReferringToType() && prevTypeForName != NULL) {
            *prevTypeForName = name->getReferredType();
        } // BETTER BE CORRECT - ELSE IS PACKAGE
    }
}

void CodeGenerator::traverseAndGenerate(FieldAccess* access) {
    // Order based on JLS 15.11.1
    traverseAndGenerate(access->getAccessedFieldPrimary());
}

void CodeGenerator::traverseAndGenerate(MethodInvoke* invoke) {
    // Order based on JLS 15.12.4
    traverseAndGenerate(invoke->getArgsForInvokedMethod());
    if(invoke->isNormalMethodCall()) {
    } else {
    }
}

void CodeGenerator::traverseAndGenerate(ArgumentsStar* args) {
    if(!args->isEpsilon()) {
        traverseAndGenerate(args->getListOfArguments());
    }
}

void CodeGenerator::traverseAndGenerate(Arguments* arg) {
    // Left to right on the stack, with this at the top
    if(!arg->lastArgument()) {
        traverseAndGenerate(arg->getNextArgs());
    }
}

void CodeGenerator::traverseAndGenerate(NewClassCreation* create) {
    // Order based on JLS 15.9.4
    traverseAndGenerate(create->getArgsToCreateClass());
}

void CodeGenerator::traverseAndGenerate(PrimaryNewArray* newArray) {
    // Order based on JLS 15.10.1
    traverseAndGenerate(newArray->getTheDimension());
}

void CodeGenerator::traverseAndGenerate(QualifiedThis* qual) {
    // JLS 15.8.4
}

void CodeGenerator::traverseAndGenerate(LiteralOrThis* lit) {
    if(lit->isThis()) {
        // JLS 15.8.3
        asmc("This literal");
        asma("mov eax, [ebp + 8]");
    } else if(lit->isNumber()) {
        // JLS 15.8.1
        asmc("Number Literal");
        asma("mov eax, " << lit->getLiteralToken()->getString());
    } else if(lit->isTrueBoolean()) {
        // JLS 15.8.1
        asmc("True Boolean literal");
        asma("mov eax, 1");
    } else if(lit->isFalseBoolean()) {
        // JLS 15.8.1
        asmc("False Boolean literal");
        asma("mov eax, 0");
    } else if(lit->isCharLiteral()) {
        // JLS 15.8.1
        std::string character = lit->getLiteralToken()->getString();
        character.erase(std::remove(character.begin(), character.end(), '\''), character.end());

        asmc("Character literal");
        asma("mov eax, " << ((int)(character.c_str()[0])));
    } else if(lit->isStringLiteral()) {
        // JLS 15.8.1
        std::string string_literal = lit->getLiteralToken()->getString();
        string_literal.erase(std::remove(string_literal.begin(), string_literal.end(), '"'), string_literal.end());

        asmc("String literal");
        // Create character array to hold string
        asma("mov eax, " << (string_literal.length() * 4));
        CALL_FUNCTION("makeArrayBanana$");

        // insert the inheritance table for char[]
        std::string charArrayInheritance = LabelManager::getLabelForArrayInheritanceTable("char");
        asma("extern " << charArrayInheritance);
        asma("mov [eax - 4], " << charArrayInheritance);
        // Copy over string into array
        unsigned int offset = 12;
        for (unsigned int i = 0; i < string_literal.length(); i++) {
            asma("mov [eax - " << offset << "], " << ((int)string_literal[i]));
            offset += 4;
        }

        asmc("Call String constructor");
        asma("push eax");
        CALL_FUNCTION(LabelManager::labelizeForConstructor("java.lang.String$char.array$"));
        //CALL_FUNCTION(LabelManager::labelizeForConstructor("java.lang.String", 1, "char.array"));
        asma("pop eax ; pop created string into eax");
        asma("pop ebx ; pop old eax");
    } else if (lit->isNull()) {
        // JLS 15.8.1
        asmc("Null literal");
        asma("mov eax, 0");
    } else {
        assert(false);
    }
}

void CodeGenerator::traverseAndGenerate(NegationExpression* negExpr) {
    // Order based on JLS 15.7
    traverseAndGenerate(negExpr->getNegatedExpression());
    if(negExpr->isNumericNegation()) {
        // Specific: JLS 15.18.2
        asmc("Numeric Negation");
        asma("neg eax");
    } else {
        // Specific: JLS 15.15.6
        // boolean negation
        asmc("Boolean Negation");
        asma("xor eax, 1");
    }
}

void CodeGenerator::traverseAndGenerate(CastExpression* cast) {
    // Order based on JLS 15.7
    // Specific: JLS 15.20.2 and JLS 5.2 (assignment conversion rules for Joos, review A3 type-checking)
    traverseAndGenerate(cast->getExpressionToCast());
    if(cast->isCastToPrimitiveType()) {
    } else {
        // cast to array or some reference type
    }
}

void CodeGenerator::traverseAndGenerate(InstanceOf* instanceof) {
    // Order based on JLS 15.7
    // Specific: JLS 15.20.2 and JLS 5.2 (assignment conversion rules for Joos, review A3 type-checking)
    traverseAndGenerate(instanceof->getExpressionToCheck());
}

void CodeGenerator::traverseAndGenerate(Assignment* assign) {
    // Order based on JLS 15.7
    // Specific: JLS 15.26
    if(assign->isAssignName()) {
        traverseAndGenerate(((AssignName*) assign)->getNameToAssign());
    } else if(assign->isAssignField()) {
        traverseAndGenerate(((AssignField*) assign)->getAssignedField());
    } else {
        // assigning to an array component
        traverseAndGenerate(((AssignArray*) assign)->getAssignedArray());
    }
    
    traverseAndGenerate(assign->getExpressionToAssign());
}

void CodeGenerator::traverseAndGenerate(ClassMethod* method) {
    if(!method->getMethodBody()->noDefinition()) {
        std::string signature = processing->getCanonicalName() + "." + method->getMethodHeader()->labelizedMethodSignature();
        asmc("Method Body - " << signature);
        asmgl(signature);
        CALL_IDIOM();
        scope_offset = -4;

        // Add Parameters to the address table
        // If static there is no implicit this
        int param_offset = method->isStatic() ? 8 : 12;

        setParameterOffsetFromEBP(method->getMethodHeader()->getClassMethodParams()->getListOfParameters(), param_offset);
        // the method has a body, then generate code
        // for the body
        traverseAndGenerate(method->getMethodBody());

        if(method->getMethodBody()->canMethodCompleteNormally()) {
            // by this stage all methods that can complete normally
            // are methods that have return type void
            asmc("Implicit Return");
            RETURN_IDIOM();
        }
    }
}

void CodeGenerator::traverseAndGenerate(MethodBody* body) {
    traverseAndGenerate(body->getBlockStmtsStar());
}

void CodeGenerator::traverseAndGenerate(BlockStmtsStar* stmts) {
    if(!stmts->isEpsilon()) {
        traverseAndGenerate(stmts->getStatements());
    }
}

void CodeGenerator::traverseAndGenerate(BlockStmts* stmt) {
    if(!stmt->isLastStatement()) {
        traverseAndGenerate(stmt->getNextBlockStmt());
    }

    if(stmt->isLocalVarDecl()) {
        traverseAndGenerate((LocalDecl*) stmt);
    } else if(stmt->isIfStmt() || stmt->isIfThenElseStmt()) {
        traverseAndGenerate((IfStmt*) stmt);
    } else if(stmt->isWhileStmt()) {
        traverseAndGenerate((WhileStmt*) stmt);
    } else if(stmt->isForStmt()) {
        traverseAndGenerate((ForStmt*) stmt);
    } else if(stmt->isAssignStmt() || stmt->isClassCreationStmt() || stmt->isMethodInvokeStmt()) {
        traverseAndGenerate((StmtExpr*) stmt);
    } else if(stmt->isNestedBlock()) {
        traverseAndGenerate((NestedBlock*) stmt);
    } else if(stmt->isReturnStmt()) {
        traverseAndGenerate((ReturnStmt*) stmt);
    }
}

void CodeGenerator::traverseAndGenerate(LocalDecl* local) {
    // Order based on JLS 14.4.4
    std::string id = local->getLocalId()->getIdAsString();
    std::string type = local->getLocalType()->getTypeAsString();

    // Everything primitive/reference type/array will be stored in a DD on the stack
    asmc("Local Decl - " << id);
    addressTable[local->getLocalTable()] = scope_offset;
    scope_offset -= 4;

    // Set the value of the expression to the new declaration
    traverseAndGenerate(local->getLocalInitExpr());
    asmc("Initialize Local Decl - " << id);
    asma("push eax");
}

void CodeGenerator::traverseAndGenerate(IfStmt* stmt) {
    // Order based on JLS 14.9
    asmc("If Statement");
    traverseAndGenerate(stmt->getExpressionToEvaluate());

    std::string lbl_false = LABEL_GEN();
    std::string lbl_end = LABEL_GEN();
    // Check if exprssion is true, if not jump to lbl_false
    asma("cmp eax, 1");
    asma("jne " << lbl_false);
    traverseAndGenerate(stmt->getExecuteTruePart());
    asma("jmp " << lbl_end);

    // ELSE
    asmc("If Statement Else");
    asml(lbl_false);
    if(!stmt->noElsePart()) {
        traverseAndGenerate(stmt->getExecuteFalsePart());
    }

    // END
    asmc("If statement end");
    asml(lbl_end);
}

void CodeGenerator::traverseAndGenerate(WhileStmt* stmt) {
    // Order based on JLS 14.11
    std::string lbl_begin = LABEL_GEN();
    std::string lbl_end = LABEL_GEN();

    asmc("While statement");

    // Check expression is true, if not lbl_end
    asml(lbl_begin);
    traverseAndGenerate(stmt->getExpressionToEvaluate());
    asma("cmp eax, 1");
    asma("jne " << lbl_end);

    asmc("While statement body");
    // If true run loop statement
    traverseAndGenerate(stmt->getLoopStmt());
    asma("jmp " << lbl_begin);

    // END
    asmc("While statement end");
    asml(lbl_end);
}

void CodeGenerator::traverseAndGenerate(ForStmt* stmt) {
    // Order based on JLS 14.13.2
    asmc("For statement init");
    int saved_scope_offset = scope_offset;
    if(!stmt->emptyForInit()) {
        traverseAndGenerate(stmt->getForInit());
    }

    std::string lbl_begin = LABEL_GEN();
    std::string lbl_end = LABEL_GEN();

    // Check expression is true, if not lbl_end
    asmc("For Statement condition");
    asml(lbl_begin);
    traverseAndGenerate(stmt->getExpressionToEvaluate());
    asma("cmp eax, 1");
    asma("jne " << lbl_end);

    asmc("For statement body");
    // If true run loop statement and update
    traverseAndGenerate(stmt->getLoopStmt());
    if(!stmt->emptyForUpdate()) {
        traverseAndGenerate(stmt->getForUpdate());
    }
    asma("jmp " << lbl_begin);

    // END
    asmc("For statement end");
    asml(lbl_end);
    scope_offset = saved_scope_offset;
}

void CodeGenerator::traverseAndGenerate(ExpressionStar* exprStar) {
    if(!exprStar->isEpsilon()) {
        traverseAndGenerate(exprStar->getExpression());
    }
}

void CodeGenerator::traverseAndGenerate(StmtExpr* stmt) {
    if(stmt->isAssignStmt()) {
        traverseAndGenerate(((StmtExprAssign*) stmt)->getAssignmentExpression());
    } else if(stmt->isClassCreationStmt()) {
        traverseAndGenerate(((StmtExprCreation*) stmt)->getCreatedClass());
    } else {
        // method invocation
        traverseAndGenerate(((StmtExprInvoke*) stmt)->getMethodInvoked());
    }
}

void CodeGenerator::traverseAndGenerate(NestedBlock* stmt) {
    // JLS 14.2
    if(!stmt->isEmptyNestedBlock()) {
        int saved_scope_offset = scope_offset;
        traverseAndGenerate(stmt->getNestedBlock());
        scope_offset = saved_scope_offset;
    }
}

void CodeGenerator::traverseAndGenerate(ReturnStmt* stmt) {
    // JLS 14.16
    traverseAndGenerate(stmt->getReturnExpr());
    RETURN_IDIOM();
}

void CodeGenerator::traverseAndGenerate(Constructor* ctor) {
    // JLS 12.5 for what happens when a constructor is called
    std::string signature = ctor->labelizedConstructorSignature();

    asmc("Constructor Body - " << signature);
    asmgl(signature);
    CALL_IDIOM();

    CompilationTable* superclass = ((ClassTable*) processing->getSymbolTable())->getClass()->getSuper()->getSuperClassTable();
    if(superclass != NULL) {
        // there is a superclass -> then call the superclass
        // zero argument constructor
        asma("push dword [ebp - 8] ; push created this onto the stack before calling super constructor");
        std::string superctor = superclass->getAConstructor("()")->getConstructor()->labelizedConstructorSignature();
        asma("extern " << superctor);
        asma("call " << superctor);
        asma("pop ebx ; pop pushed this");
    }

    // call initializers here
    callInitializersOfDeclaredFields();

    // set offset for constructor parameters from EBP,
    // starting offset is always 12 for constructors, because
    // there's always this
    setParameterOffsetFromEBP(ctor->getConstructorParameters()->getListOfParameters(), 12);

    scope_offset = -4;
    traverseAndGenerate(ctor->getConstructorBody());

    if(ctor->canConstructorCompleteNormally()) {
        asmc("Implicit Return");
        RETURN_IDIOM();
    }
}
