/** The cloning pass
 * @file
 *
 * This source file is part of the Cone Programming Language C compiler
 * See Copyright Notice in conec.h
*/

#include "ir.h"

#include <assert.h>

// Deep copy a node
INode *cloneNode(CloneState *cstate, INode *nodep) {
    INode *node;
    switch (nodep->tag) {
    case AllocateTag:
        node = cloneAllocateNode(cstate, (AllocateNode *)nodep); break;
    case AssignTag:
        node = cloneAssignNode(cstate, (AssignNode *)nodep); break;
    case BlockTag:
        node = cloneBlockNode(cstate, (BlockNode *)nodep); break;
    case ULitTag:
        node = cloneULitNode((ULitNode *)nodep); break;
    case FLitTag:
        node = cloneFLitNode((FLitNode *)nodep); break;
    case StringLitTag:
        node = cloneSLitNode((SLitNode *)nodep); break;
    default:
        assert(0 && "Do not know how to clone a node of this type");
    }
    node->instnode = cstate->instnode;
    return node;
}
