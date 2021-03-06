#ifndef __WEEDS_NO_ABSTRACT_BODY_H__
#define __WEEDS_NO_ABSTRACT_BODY_H__

#include "weed.h"
#include <cassert>

class NoAbstractNativeBody : public Weed
{
    public:
        NoAbstractNativeBody()
        {
            rule = METHOD_HEADER_AND_BODY;
        }

        unsigned int hasAbstractMod(ParseTree* node) {
            unsigned int found = 0;

            switch (node->rule) {
                case METHOD_HEADER_AND_BODY:
                case METHOD_TYPE:
                case METHOD_VOID:
                case MEMBER_MOD:
                case MEMBER_MOD_LIST:
                    for (unsigned int i = 0; i < node->children.size(); i++) {
                        found += hasAbstractMod(node->children[i]);
                    }
                    break;
                case MEMBER_MOD_ABSTRACT:
                    return 1;
                default:
                    return 0;
            }

            return found;
        }

        unsigned int hasNativeMod(ParseTree* node) {
            unsigned int found = 0;

            switch (node->rule) {
                case METHOD_HEADER_AND_BODY:
                case METHOD_TYPE:
                case METHOD_VOID:
                case MEMBER_MOD:
                case MEMBER_MOD_LIST:
                    for (unsigned int i = 0; i < node->children.size(); i++) {
                        found += hasNativeMod(node->children[i]);
                    }
                    break;
                case MEMBER_MOD_NATIVE:
                    return 1;
                default:
                    return 0;
            }

            return found;
        }

        std::string getMethodName(ParseTree* node) {
            for (unsigned int i = 0; i < node->children.size(); i++) {
                if (node->children[i]->rule == METHOD_TYPE ||
                    node->children[i]->rule == METHOD_VOID) {
                    node = node->children[i];
                    break;
                }
            }

            for (unsigned int i = 0; i < node->children.size(); i++) {
                if (node->children[i]->rule == METHOD_DECL) {
                    node = node->children[i];
                    break;
                }
            }

            for (unsigned int i = 0; i < node->children.size(); i++) {
                if (node->children[i]->rule == IDENTIFIER) {
                    token = node->children[i]->children[0]->token;
                    return node->children[i]->children[0]->token->getString();
                }
            }

            assert(false);
        }

        void check(ParseTree* node)
        {
            if (hasAbstractMod(node) ||
                hasNativeMod(node)) {
                for (unsigned int i = 0; i < node->children.size(); i++) {
                    if (node->children[i]->rule == METHOD_BODY) {
                        std::stringstream ss;

                        if (hasAbstractMod(node)) {
                            ss << "Abstract method '";
                        } else {
                            ss << "Native method '";
                        }
                        ss << getMethodName(node) << "' cannot have a body.";

                        Error(E_WEEDER, token, ss.str());
                    }
                }
            }
        }
};

#endif
