#ifndef __METHODNORMALINVOKE_H__
#define __METHODNORMALINVOKE_H__

#include "methodInvoke.h"
#include "name.h"

class MethodNormalInvoke : public MethodInvoke {
    private:
        Name* methodInvoked;
    public:
        MethodNormalInvoke(Name* methodInvoked, ArgumentsStar* args) : MethodInvoke(args), methodInvoked(methodInvoked) {}
        ~MethodNormalInvoke() {
            delete methodInvoked;
        }

        Name* getInvokedMethod() { return methodInvoked; }
};

#endif
