/** Handling for function/method declaration nodes
 * @file
 *
 * This source file is part of the Cone Programming Language C compiler
 * See Copyright Notice in conec.h
*/

#include "../ir.h"

#include <string.h>
#include <assert.h>

// Create a new function declaraction node
FnDclNode *newFnDclNode(Name *namesym, uint16_t flags, INode *type, INode *val) {
    FnDclNode *node;
    newNode(node, FnDclNode, FnDclTag);
    node->flags = flags;
    node->vtype = type;
    node->namesym = namesym;
    node->value = val;
    node->llvmvar = NULL;
    node->genname = namesym? &namesym->namestr : "";
    node->nextnode = NULL;
    node->genericinfo = NULL;
    return node;
}

// Return a clone of a function/method declaration
INode *cloneFnDclNode(CloneState *cstate, FnDclNode *oldfn) {
    uint32_t dclpos = cloneDclPush();
    FnDclNode *newnode = memAllocBlk(sizeof(FnDclNode));
    memcpy(newnode, oldfn, sizeof(FnDclNode));
    newnode->genericinfo = NULL;
    newnode->nextnode = NULL; // clear out linkages
    newnode->vtype = cloneNode(cstate, oldfn->vtype);
    newnode->value = cloneNode(cstate, oldfn->value);
    cloneDclPop(dclpos);
    return (INode*)newnode;
}

// Serialize a function node
void fnDclPrint(FnDclNode *node) {
    if (node->namesym)
        inodeFprint("fn %s", &node->namesym->namestr);
    else
        inodeFprint("fn");
    if (node->genericinfo)
        genericInfoPrint(node->genericinfo);
    inodePrintNode(node->vtype);
    if (node->value) {
        inodeFprint(" {} ");
        if (node->value->tag == BlockTag)
            inodePrintNL();
        inodePrintNode(node->value);
    }
}

// Resolve all names in a function
void fnDclNameRes(NameResState *nstate, FnDclNode *fndclnode) {
    // Resolve generic parameters
    INode **nodesp;
    uint32_t cnt;
    if (fndclnode->genericinfo) {
        for (nodesFor(fndclnode->genericinfo->parms, cnt, nodesp))
            inodeNameRes(nstate, nodesp);
    }

    nametblHookPush();
    if (fndclnode->genericinfo) {
        // Hook generic parms so we can resolve them throughout type
        for (nodesFor(fndclnode->genericinfo->parms, cnt, nodesp))
            nametblHookNode(((VarDclNode *)*nodesp)->namesym, *nodesp);
    }
    inodeNameRes(nstate, &fndclnode->vtype);

    if (fndclnode->value) {

        uint16_t oldscope = nstate->scope;
        nstate->scope = 1;

        // Hook function's parameters into global fndclnode table
        // so that when we walk the function's logic, parameter names are resolved
        FnSigNode *fnsig = (FnSigNode*)fndclnode->vtype;
        for (nodesFor(fnsig->parms, cnt, nodesp))
            nametblHookNode(((VarDclNode *)*nodesp)->namesym, *nodesp);

        inodeNameRes(nstate, &fndclnode->value);

        nstate->scope = oldscope;
    }

    nametblHookPop();
}

// Syntactic sugar: Turn last statement implicit returns into explicit returns
void fnImplicitReturn(INode *rettype, BlockNode *blk) {
    INode *laststmt;
    if (blk->stmts->used == 0)
        nodesAdd(&blk->stmts, (INode*)newReturnNodeExp((INode*)newNilLitNode()));
    laststmt = nodesLast(blk->stmts);
    if (rettype->tag == VoidTag) {
        if (laststmt->tag != ReturnTag)
            nodesAdd(&blk->stmts, (INode*)newReturnNodeExp((INode*)newNilLitNode()));
    }
    else {
        // Inject return in front of expression
        if (isExpNode(laststmt)) {
            BreakRetNode *retnode = newReturnNodeExp(laststmt);
            nodesLast(blk->stmts) = (INode*)retnode;
        }
        else if (laststmt->tag != ReturnTag)
            errorMsgNode(laststmt, ErrorNoRet, "A return value is expected but this statement cannot give one.");
    }
}

// Type checking a function's logic does more than you might think:
// - Turn implicit returns into explicit returns
// - Perform type checking for all statements
// - Perform data flow analysis on variables and references
void fnDclTypeCheck(TypeCheckState *pstate, FnDclNode *fnnode) {
    // Wait until a generic function is instantiated before type checking
    if (fnnode->genericinfo)
        return;

    iTypeTypeCheck(pstate, &fnnode->vtype);
    // No need to type check function body if no body or is a default method of a trait
    if (!fnnode->value 
        || ((fnnode->flags & FlagMethFld) && pstate->typenode->tag == StructTag && (pstate->typenode->flags & TraitType)))
        return;

    // Ensure self parameter on a method is (reference to) its enclosing type
    if (fnnode->flags & FlagMethFld) {
        INode *selfparm = nodesGet(((FnSigNode *)(fnnode->vtype))->parms, 0);
        if (iexpGetDerefTypeDcl(selfparm) != pstate->typenode)
            errorMsgNode((INode*)fnnode, ErrorInvType, "self parameter for a method must match, or be a reference to, its type");
    }

    // Syntactic sugar: Turn implicit returns into explicit returns
    fnImplicitReturn(((FnSigNode*)fnnode->vtype)->rettype, (BlockNode *)fnnode->value);

    // Type check/inference of the function's logic
    FnDclNode *svFn = pstate->fn;
    pstate->fn = fnnode;
    inodeTypeCheck(pstate, &fnnode->value, noCareType);
    pstate->fn = svFn;

    // Immediately perform the data flow pass for this function
    // We run data flow separately as it requires type info which is inferred bottoms-up
    if (errors)
        return;
    FlowState fstate;
    fstate.fnsig = (FnSigNode *)fnnode->vtype;
    fstate.scope = 1;
    blockFlow(&fstate, (BlockNode **)&fnnode->value);
}
