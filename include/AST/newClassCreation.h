#ifndef __NEWCLASSCREATION_H__
#define __NEWCLASSCREATION_H__

#include "primary.h"
#include "name.h"
#include "argumentsStar.h"

class NewClassCreation : public Primary {
    // Rule: MAKE_NEW_CLASS
    private:
        Name* classType;
        ArgumentsStar* args;
    public:
        NewClassCreation(Name* classType, ArgumentsStar* args) : classType(classType), args(args) {}
        ~NewClassCreation() {
            delete classType;
            delete args;
        }

        Name* getClassName() { return classType; }
        ArgumentsStar* getArgsToCreateClass() { return args; }
};

#endif