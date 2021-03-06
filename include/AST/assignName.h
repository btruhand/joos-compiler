#ifndef __ASSIGNNAME_H__
#define __ASSIGNNAME_H__

#include "expression.h"
#include "name.h"
#include "assignment.h"

class AssignName : public Assignment {
    // Rule: LVALUE_NAME
    private:
        Name* LHS;
    public:
        AssignName(Name* LHSname, Expression* RHSexpr) : Assignment(RHSexpr), LHS(LHSname) {}
        ~AssignName() {
            delete LHS;
        }

        Name* getNameToAssign() { return LHS; }
};

#endif
