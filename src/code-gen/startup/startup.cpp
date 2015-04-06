#include <cassert>
#include <fstream>
#include <sstream>

// Code generation related
#include "startup.h"
#include "vtableLayout.h"
#include "objectLayout.h"
#include "inheritanceTable.h"
#include "implementedInterfaceMethodTable.h"
#include "staticFields.h"

// Symbol table related
#include "compilationTable.h"
#include "classTable.h"
#include "interfaceTable.h"
#include "classMethodTable.h"
#include "fieldTable.h"

// AST related
#include "classDecl.h"
#include "interfaceDecl.h"
#include "classMethod.h"
#include "fieldDecl.h"

#include "labelManager.h"

#include "error.h"

class InterfaceMethodTable;

Startup::Startup(std::map<std::string, CompilationTable*>& compilations, CompilationTable* firstUnit) : 
    compilations(compilations), firstUnit(firstUnit) {
}

void Startup::generateStartupFile(VTableLayout* arrayVTable, std::vector<InheritanceTable*>& arrayInheritance,
                ImplInterfaceMethodTable* arrayMethods, std::vector<StaticFields*>& statics) {
#if defined(CODE_OUT)
            std::stringstream ss;
            ss << CODE_OUT << "/_startup.s";
            std::ofstream fs(ss.str());
#else
            std::ofstream fs("_startup.s");
#endif
    
    // data section
    fs << "section .data\n";

    // generate virtual table for arrays
    arrayVTable->outputVTableToFile(fs);

    // generate inheritance table for arrays
    for(unsigned int i = 0; i < arrayInheritance.size(); i++) {
        arrayInheritance[i]->outputInheritanceTableToFile(fs);
    }

    // generate implemented interface method table for array
    arrayMethods->outputImplInterfaceMethodTableToFile(fs);
    
    fs << "section .text\n";

    // method for array creation
    fs << "global makeArrayBanana$\nmakeArrayBanana$:\n";
    fs << "extern __malloc\n";
    fs << "; assumption is eax contains the size of the created array (not including space for length and the tables) in bytes\n";
    fs << "push eax\n";
    fs << "add eax, 24 ; add 24 bytes for length and all of table space and type number\n";
    fs << "call __malloc ; call malloc\n";
    fs << "pop ebx ; get back old pushed eax\n";
    fs << "shr ebx, 2 ; divide ebx by 4\n";
    fs << "mov [eax], dword -1 ; store invalid component type\n";
    fs << "mov [eax - 4], ebx ; store length\n";
    fs << "add eax, -8 ; sub 8 to start storing the tables\n";
    fs << "mov [eax], dword " << LabelManager::getLabelForArrayVirtualTable() << " ; store array virtual table\n";
    fs << "mov [eax-8], dword " << LabelManager::getLabelForArrayImplInterfaceMethodTable() << " ; store array interface method table\n";
    fs << "; initialize array to all 0s\n";
    fs << "mov ecx, ebx\n";
    fs << "lea ebx, [eax - 16]\n";
    fs << "initArray:\n";
    fs << "mov [ebx], dword 0\n";
    fs << "add ebx, -4\n";
    fs << "loop initArray\n";
    fs << "ret\n\n";

    fs << "global _start\n_start:\n";
    // call static fields initializer
    for(unsigned int i = 0; i < statics.size(); i++) {
        std::vector<FieldTable*>& staticFields = statics[i]->getAllStaticFieldsOfClass();
        for(unsigned int j = 0; j < staticFields.size(); j++) {
            std::string staticInitCall = staticFields[j]->generateFieldInitializerLabel();
            fs << "extern " << staticInitCall << '\n';
            fs << "call " << staticInitCall << '\n';
        }
    }

    fs << std::endl;
    // call static int test() of the first compilation unit
    // given in the command line to joosc
    if (!firstUnit->getAClassMethod("test()") ||
        !firstUnit->getAClassMethod("test()")->getClassMethod()->isStatic() ||
        firstUnit->getAClassMethod("test()")->getClassMethod()->getMethodHeader()->getReturnType()->getTypeAsString() != "int") {
        Error(E_DEFAULT, NULL, "The file '" + firstUnit->getCanonicalName() + "' does not contain the method 'static int test()'.");
        CHECK_ERROR();
    }
    std::string testMethod = firstUnit->getAClassMethod("test()")->generateMethodLabel();
    fs << "extern " << testMethod << '\n';
    fs << "call " << testMethod << '\n';
    fs << "extern __debexit" << '\n';
    fs << "call __debexit" << std::endl;
    fs.close();
}
