/** Handling for array literals
 * @file
 *
 * This source file is part of the Cone Programming Language C compiler
 * See Copyright Notice in conec.h
*/

#include "../ir.h"

// Note:  Creation, serialization and name checking are done with array type logic,
// as we don't yet know whether [] is a type or an array literal

// Type check an array literal
void arrayLitTypeCheckDimExp(TypeCheckState *pstate, ArrayNode *arrlit) {

    // Handle array literal "fill" format: [dimen, fill-value]
    if (arrlit->dimens->used > 0) {

        // Ensure only one constant integer dimension
        if (arrlit->dimens->used > 1) {
            errorMsgNode((INode*)arrlit, ErrorBadArray, "Array literal may only specify one dimension");
            return;
        }
        INode **dimnodep = &nodesGet(arrlit->dimens, 0);
        INode *dimnode = *dimnodep;
        if (dimnode->tag == ULitTag)
            ((ULitNode*)dimnode)->vtype = (INode*)usizeType; // Force type
        // Ensure it coerces to usize
        if (iexpTypeCheckCoerce(pstate, (INode*)usizeType, dimnodep) != 1)
            errorMsgNode((INode*)arrlit, ErrorBadArray, "Array literal dimension must coerce to usize");

        // Handle and type the single fill value
        if (arrlit->elems->used != 1 || !isExpNode(nodesGet(arrlit->elems, 0))) {
            errorMsgNode((INode*)arrlit, ErrorBadArray, "Array fill value may only be one value");
            return;
        }
        INode **elemnodep = &nodesGet(arrlit->elems, 0);
        size_t dimsize = 0;
        if (dimnode->tag != ULitTag) {
            while (dimnode->tag == VarNameUseTag) {
                INode *dclnode = ((NameUseNode*)dimnode)->dclnode;
                if (dclnode->tag == ConstDclTag)
                    dimnode = ((ConstDclNode*)dclnode)->value;
                else
                    break;
            }
        }
        if (dimnode->tag == ULitTag)
            dimsize = (size_t)((ULitNode*)dimnode)->uintlit;
        if (iexpTypeCheckAny(pstate, elemnodep)) {
            arrlit->vtype = (INode*)newArrayNodeTyped((INode*)arrlit,
                dimsize, ((IExpNode*)*elemnodep)->vtype);
        }
        return;
    }

    // Otherwise handle multi-value array literal
    if (arrlit->elems->used == 0) {
        errorMsgNode((INode*)arrlit, ErrorBadArray, "Array literal list may not be empty");
        return;
    }

    // Ensure all elements are consistently typed (matching first element's type)
    INode *matchtype = unknownType;
    INode **nodesp;
    uint32_t cnt;
    for (nodesFor(arrlit->elems, cnt, nodesp)) {
        if (iexpTypeCheckAny(pstate, nodesp) == 0)
            continue;
        if (matchtype == unknownType) {
            // Get element type from first element
            // Type of array literal is: array of elements whose type matches first value
            matchtype = ((IExpNode*)*nodesp)->vtype;
        }
        else if (!iTypeIsSame(((IExpNode *) *nodesp)->vtype, matchtype))
            errorMsgNode((INode*)*nodesp, ErrorBadArray, "Inconsistent type of array literal value");
    }
    arrlit->vtype = (INode*)newArrayNodeTyped((INode*)arrlit, arrlit->elems->used, matchtype);
}

// The default type check
void arrayLitTypeCheck(TypeCheckState *pstate, ArrayNode *arrlit) {

    // In the default scenario (not as part of region allocation),
    // we must insist that array literal's dimension is a constant unsigned integer
    if (arrlit->dimens->used > 0 && !litIsLiteral(nodesGet(arrlit->dimens, 0))) {
        errorMsgNode((INode*)arrlit, ErrorBadArray, "Array literal dimension value must be a constant");
    }
    arrayLitTypeCheckDimExp(pstate, arrlit);
}

// Is the array actually a literal?
int arrayLitIsLiteral(ArrayNode *node) {
    INode **nodesp;
    uint32_t cnt;
    for (nodesFor(node->elems, cnt, nodesp)) {
        INode *elem = *nodesp;
        if (!litIsLiteral(elem))
            return 0;
    }
    return 1;
}
