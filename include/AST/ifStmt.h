#ifndef __IFSTMT_H__
#define __IFSTMT_H__

#include "blockStmts.h"

class IfStmt : public BlockStmts {
    private:
        Expression* eval;
        BlockStmts* exTrue;
        BlockStmts* exFalse;
    public:
        IfStmt(Expression* eval, BlockStmts* exTrue, BlockStmts* exFalse) : BlockStmts(), eval(eval), exTrue(exTrue), exFalse(exFalse) {}
        ~IfStmt() {
            delete eval;
            delete exTrue;
            delete exFalse;
        }

        Expression* getExpressionToEvaluate() { return eval; }
        BlockStmts* getExecuteTruePart() { return exTrue; }
        BlockStmts* getExecuteFalsePart() { return exFalse; }
};

#endif
