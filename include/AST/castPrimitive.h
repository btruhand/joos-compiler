#ifndef __CASTPRIMITIVE_H__
#define __CASTPRIMITIVE_H__

#include "castExpression.h"
#include "primitiveType.h"

class CastPrimitive : public CastExpression {
    // Rule : CAST_PRIMITIVE
    private:
        int dimsRule;
        PrimitiveType* castType;
    public:
        CastPrimitive(int dimsRule, PrimitiveType* castType, Expression* toCast) : CastExpression(toCast), dimsRule(dimsRule),
                      castType(castType) {}
        ~CastPrimitive() {
            delete castType;
            delete toCast;
        }

        PrimitiveType* getPrimitiveTypeToCastTo() { return castType; }
        bool isPrimitiveArrayCast() { return dimsRule == ARRAY_DIMS; }
        int getDimensionRule() { return dimsRule; }
};

#endif