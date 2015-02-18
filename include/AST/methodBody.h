#ifndef __METHODBODY_H__
#define __METHODBODY_H__

#include "ast.h"
#include "blockStmtsStar.h"

class MethodBody : public Ast {
    private:
        BlockStmtsStar* stmts;
    public:
        MethodBody(BlockStmtsStar* stmts) : stmts(stmts) {}
        ~MethodBody() {
            delete stmts;
        }

        BlockStmtsStar* getBlockStmtsStar() { return stmts; }
        bool noBody() { return stmts == NULL; }
};

#endif
