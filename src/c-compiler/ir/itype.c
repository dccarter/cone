/** Generic Type node handling
 * @file
 *
 * This source file is part of the Cone Programming Language C compiler
 * See Copyright Notice in conec.h
*/

#include "ir.h"

#include <string.h>
#include <assert.h>

// Return node's type's declaration node
// (Note: only use after it has been type-checked)
INode *iTypeGetTypeDcl(INode *node) {
    assert(isTypeNode(node));
    while (1) {
        switch (node->tag) {
        case TypeNameUseTag:
            node = ((NameUseNode *)node)->dclnode;
            break;
        case TypedefTag:
            node = ((TypedefNode *)node)->typeval;
        default:
            return node;
        }
    }
}

// Return node's type's declaration node (or vtexp if a ref or ptr)
INode *iTypeGetDerefTypeDcl(INode *node) {
    INode *typNode = iTypeGetTypeDcl(node);
    if (typNode->tag == RefTag || typNode->tag == VirtRefTag)
        return iTypeGetTypeDcl(((RefNode *) node)->vtexp);
    else if (node->tag == PtrTag)
        return iTypeGetTypeDcl(((StarNode *) node)->vtexp);
    return typNode;
}

// Look for named field/method in type
INode *iTypeFindFnField(INode *type, Name *name) {
    switch (type->tag) {
    case StructTag:
    case UintNbrTag:
    case IntNbrTag:
    case FloatNbrTag:
        return iNsTypeFindFnField((INsTypeNode*)type, name);
    case PtrTag:
        return iNsTypeFindFnField(ptrType, name);
    default:
        return NULL;
    }
}

// Type check node, expecting it to be a type. Give error and return 0, if not.
int iTypeTypeCheck(TypeCheckState *pstate, INode **node) {
    inodeTypeCheckAny(pstate, node);
    if (!isTypeNode(*node)) {
        errorMsgNode(*node, ErrorNotTyped, "Expected a type.");
        return 0;
    }
    return 1;
}

// Return 1 if nominally (or structurally) identical, 0 otherwise
// Nodes must both be types, but may be name use or declare nodes
int iTypeIsSame(INode *node1, INode *node2) {

    node1 = iTypeGetTypeDcl(node1);
    node2 = iTypeGetTypeDcl(node2);

    // If they are the same type name, types match
    if (node1 == node2)
        return 1;
    if (node1->tag != node2->tag)
        return 0;

    // For non-named types, equality is determined structurally
    // because they specify the same typed parts
    switch (node1->tag) {
    case RefTag: 
        return refIsSame((RefNode*)node1, (RefNode*)node2);
    case VirtRefTag:
        return refIsSame((RefNode*)node1, (RefNode*)node2);
    case ArrayRefTag:
        return arrayRefIsSame((RefNode*)node1, (RefNode*)node2);
    case PtrTag:
        return ptrEqual((StarNode*)node1, (StarNode*)node2);
    case ArrayTag:
        return arrayEqual((ArrayNode*)node1, (ArrayNode*)node2);
    case TTupleTag:
        return ttupleEqual((TupleNode*)node1, (TupleNode*)node2);
    case FnSigTag:
        return fnSigEqual((FnSigNode*)node1, (FnSigNode*)node2);
    case VoidTag:
        return 1;
    default:
        return 0;
    }
}

// Calculate the hash for a type to use in type table indexing
size_t iTypeHash(INode *type) {
    INode *dclType = iTypeGetTypeDcl(type);
    switch (dclType->tag) {
    case RefTag:
    case VirtRefTag:
        return refHash((RefNode*)dclType);
    case ArrayRefTag:
        return arrayRefHash((RefNode*)dclType);
    case PermTag:
        return ((size_t)immPerm) >> 3;  // Hash for all static permissions is the same
    default:
        // Turn type's pointer into the hash, removing expected 0's in bottom bits
        return ((size_t)dclType) >> 3;
    }
}

// Return 1 if nominally (or structurally) identical at runtime, 0 otherwise
// Nodes must both be types, but may be name use or declare nodes
// Is a companion for indexing into the type table
int iTypeIsRunSame(INode *node1, INode *node2) {

    node1 = iTypeGetTypeDcl(node1);
    node2 = iTypeGetTypeDcl(node2);

    // If they are the same type name, types match
    if (node1 == node2)
        return 1;
    if (node1->tag != node2->tag)
        return 0;

    // For non-named types, equality is determined structurally
    // because they specify the same typed parts
    switch (node1->tag) {
    case RefTag:
        return refIsRunSame((RefNode*)node1, (RefNode*)node2);
    case VirtRefTag:
        return refIsRunSame((RefNode*)node1, (RefNode*)node2);
    case ArrayRefTag:
        return arrayRefIsRunSame((RefNode*)node1, (RefNode*)node2);
    case PtrTag:
        return ptrEqual((StarNode*)node1, (StarNode*)node2);
    case ArrayTag:
        return arrayEqual((ArrayNode*)node1, (ArrayNode*)node2);
    case TTupleTag:
        return ttupleEqual((TupleNode*)node1, (TupleNode*)node2);
    case FnSigTag:
        return fnSigEqual((FnSigNode*)node1, (FnSigNode*)node2);
    case VoidTag:
        return 1;
    case PermTag:
        return 1;    // Static permissions are erased/equivalent at runtime
    default:
        return 0;
    }
}

// Is toType equivalent or a subtype of fromType
TypeCompare iTypeMatches(INode *toType, INode *fromType, SubtypeConstraint constraint) {
    fromType = iTypeGetTypeDcl(fromType);
    toType = iTypeGetTypeDcl(toType);

    // If they are the same value type info, types match
    if (toType == fromType)
        return EqMatch;

    // Type-specific matching logic
    switch (toType->tag) {

    case UintNbrTag:
    case IntNbrTag:
    case FloatNbrTag:
        return nbrMatches(toType, fromType, constraint);

    case StructTag:
        return structMatches((StructNode*)toType, fromType, constraint);

    case TTupleTag:
        if (fromType->tag == TTupleTag)
            return iTypeIsSame(toType, fromType) ? EqMatch : NoMatch;
        return NoMatch;

    case ArrayTag:
        if (fromType->tag == ArrayTag)
            return arrayMatches((ArrayNode*)toType, (ArrayNode*)fromType, constraint);
        return NoMatch;

    case FnSigTag:
        if (fromType->tag == FnSigTag)
            return fnSigMatches((FnSigNode*)toType, (FnSigNode*)fromType, constraint);
        return NoMatch;

    case RefTag:
        if (fromType->tag == RefTag)
            return refMatches((RefNode*)toType, (RefNode*)fromType, constraint);
        return NoMatch;

    case VirtRefTag:
        if (fromType->tag == VirtRefTag)
            return refvirtMatches((RefNode*)toType, (RefNode*)fromType, constraint);
        else if (fromType->tag == RefTag)
            return refvirtMatchesRef((RefNode*)toType, (RefNode*)fromType, constraint);
        return NoMatch;

    case ArrayRefTag:
        if (fromType->tag == ArrayRefTag)
            return arrayRefMatches((RefNode*)toType, (RefNode*)fromType, constraint);
        else if (fromType->tag == RefTag)
            return arrayRefMatchesRef((RefNode*)toType, (RefNode*)fromType, constraint);
        return NoMatch;

    case PtrTag:
        if (fromType->tag == RefTag || fromType->tag == ArrayRefTag)
            return iTypeIsSame(((RefNode *) fromType)->vtexp, ((StarNode *) toType)->vtexp) ? ConvSubtype : NoMatch;
        if (fromType->tag == PtrTag)
            return ptrMatches((StarNode*)toType, (StarNode*)fromType, constraint);
        return NoMatch;

    case VoidTag:
        return fromType->tag == VoidTag ? EqMatch : NoMatch;

    default:
        return iTypeIsSame(toType, fromType) ? EqMatch : NoMatch;
    }
}

// Return a type that is the supertype of both type nodes, or NULL if none found
INode *iTypeFindSuper(INode *type1, INode *type2) {
    INode *typ1 = iTypeGetTypeDcl(type1);
    INode *typ2 = iTypeGetTypeDcl(type2);

    if (typ1->tag != typ2->tag)
        return NULL;
    if (iTypeIsSame(typ1, typ2))
        return type1;
    switch (typ1->tag) {
    case UintNbrTag:
    case IntNbrTag:
    case FloatNbrTag:
        return nbrFindSuper(type1, type2);

    case StructTag:
        return structFindSuper(type1, type2);

    case RefTag:
    case VirtRefTag:
        return refFindSuper(type1, type2);

    default:
        return NULL;
    }
}

// Add type mangle info to buffer
char *iTypeMangle(char *bufp, INode *vtype) {
    switch (vtype->tag) {
    case NameUseTag:
    case TypeNameUseTag:
    {
        strcpy(bufp, &((INsTypeNode*)((NameUseNode *)vtype)->dclnode)->namesym->namestr);
        break;
    }
    case RefTag:
    case ArrayRefTag:
    case VirtRefTag:
    {
        RefNode *reftype = (RefNode *)vtype;
        *bufp++ = vtype->tag==VirtRefTag? '<' : ArrayRefTag? '+' : '&';
        if (permIsSame(reftype->perm, (INode*)roPerm)) {
            bufp = iTypeMangle(bufp, reftype->perm);
            *bufp++ = ' ';
        }
        bufp = iTypeMangle(bufp, reftype->vtexp);
        break;
    }
    case PtrTag:
    {
        StarNode *vtexp = (StarNode *)vtype;
        *bufp++ = '*';
        bufp = iTypeMangle(bufp, vtexp->vtexp);
        break;
    }
    case UintNbrTag:
        *bufp++ = 'u'; break;
    case IntNbrTag:
        *bufp++ = 'i'; break;
    case FloatNbrTag:
        *bufp++ = 'f'; break;

    default:
        assert(0 && "unknown type for parameter type mangling");
    }
    return bufp + strlen(bufp);
}

// Return true if type has a concrete and instantiable value. 
// Opaque structs, traits, functions will be false.
int iTypeIsConcrete(INode *type) {
    INode *dcltype = iTypeGetTypeDcl(type);
    return !(dcltype->flags & OpaqueType);
}

// Return true if type has zero size (e.g., void, empty struct)
int iTypeIsZeroSize(INode *type) {
    INode *dcltype = iTypeGetTypeDcl(type);
    return dcltype->flags & ZeroSizeType;
}

// Return true if type implements move semantics
int iTypeIsMove(INode *type) {
    return iTypeGetTypeDcl(type)->flags & MoveType;
}

// Return true if this is a generic type
int iTypeIsGenericType(INode *type) {
    if (type->tag != FnCallTag)
        return 0;
    FnCallNode *gentype = (FnCallNode*)type;
    if (gentype->objfn->tag != GenericNameTag)
        return 0;
    return gentype->args->used > 0 && (nodesGet(&gentype->args, 0));
}