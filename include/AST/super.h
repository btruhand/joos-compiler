#ifndef __SUPER_H__
#define __SUPER_H__

#include "ast.h"
#include "name.h"

class CompilationTable;

class Super : public Ast {
    // Rule: EXTENDS_CLASS || EXTENDS_CLASS_EPSILON
    private:
        Name* superClass;
        CompilationTable* superClassTable;
        bool implicitExtend;
    public:
        Super(Name* superClass) : superClass(superClass), superClassTable(NULL), implicitExtend(false) {}
        ~Super() {
            delete superClass;
        }

        Name* getSuperName() { return superClass; }
        void setSuperClassTable(CompilationTable* set) { superClassTable = set; }
        CompilationTable* getSuperClassTable() { return superClassTable; }
        bool isEpsilon() { return superClass == NULL; }
        void setImplicitExtend() { implicitExtend = true; }
        bool isImplicitlyExtending() { return implicitExtend; }
};

#endif
