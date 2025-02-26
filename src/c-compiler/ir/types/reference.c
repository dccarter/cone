/** Handling for references
 * @file
 *
 * This source file is part of the Cone Programming Language C compiler
 * See Copyright Notice in conec.h
*/

#include "../ir.h"
#include <memory.h>

// Create a new reference type whose info will be filled in afterwards
RefNode *newRefNode(uint16_t tag) {
    RefNode *refnode;
    newNode(refnode, RefNode, tag);
    refnode->region = borrowRef;          // Default values
    refnode->perm = (INode*)roPerm;
    refnode->vtype = (INode*)unknownType;
    refnode->typeinfo = NULL;
    return refnode;
}

// Allocate info for normalized reference type
void *refTypeInfoAlloc() {
    RefTypeInfo *refinfo = memAllocBlk(sizeof(RefTypeInfo));
    refinfo->llvmtyperef = NULL;
    refinfo->structype = NULL;
    refinfo->ptrstructype = NULL;
    return (void*)refinfo;
}

// Clone reference
INode *cloneRefNode(CloneState *cstate, RefNode *node) {
    RefNode *newnode = memAllocBlk(sizeof(RefNode));
    memcpy(newnode, node, sizeof(RefNode));
    newnode->region = cloneNode(cstate, node->region);
    newnode->perm = cloneNode(cstate, node->perm);
    newnode->vtexp = cloneNode(cstate, node->vtexp);
    return (INode *)newnode;
}

// Set type infection flags based on the reference's type parameters
void refAdoptInfections(RefNode *refnode) {
    if (refnode->perm == NULL || refnode->vtexp == unknownType)
        return;  // Wait until we have this info
    if (!(permGetFlags(refnode->perm) & MayAlias) || iTypeIsMove(refnode->region))
        refnode->flags |= MoveType;
    if (refnode->perm == (INode*)mutPerm || refnode->perm == (INode*)roPerm 
        || (refnode->vtexp->flags & ThreadBound))
        refnode->flags |= ThreadBound;
}

// Create a reference node based on fully-known type parameters
RefNode *newRefNodeFull(uint16_t tag, INode *lexnode, INode *region, INode *perm, INode *vtype) {
    RefNode *refnode = newRefNode(tag);
    if (lexnode)
        inodeLexCopy((INode*)refnode, lexnode);
    refnode->region = region;
    refnode->perm = perm;
    refnode->vtexp = vtype;
    refAdoptInfections(refnode);
    return refnode;
}

// Set the inferred value type of a reference
void refSetPermVtype(RefNode *refnode, INode *perm, INode *vtype) {
    refnode->perm = perm;
    refnode->vtexp = vtype;
    refAdoptInfections(refnode);
}

// Create a new ArrayDerefNode from an ArrayRefNode
RefNode *newArrayDerefNodeFrom(RefNode *refnode) {
    RefNode *dereftype = newRefNode(RefTag);
    dereftype->tag = ArrayDerefTag;
    dereftype->region = refnode->region;
    dereftype->perm = refnode->perm;
    dereftype->vtexp = refnode->vtexp;
    return dereftype;
}

// Serialize a pointer type
void refPrint(RefNode *node) {
    inodeFprint("&(");
    inodePrintNode(node->region);
    inodeFprint(" ");
    inodePrintNode((INode*)node->perm);
    inodeFprint(" ");
    inodePrintNode(node->vtexp);
    inodeFprint(")");
}

// Name resolution of a reference node
void refNameRes(NameResState *pstate, RefNode *node) {
    inodeNameRes(pstate, &node->region);
    inodeNameRes(pstate, (INode**)&node->perm);
    inodeNameRes(pstate, &node->vtexp);

    // If this is not a reference type, turn it into a borrow/allocate constructor
    if (!isTypeNode(node->vtexp)) {
        if (node->tag == RefTag)
            node->tag = node->region == (INode*)borrowRef ? BorrowTag : AllocateTag;
        else
            errorMsgNode((INode*)node, ErrorBadTerm, "May not borrow or allocate a virtual reference. Coerce from a regular ref.");
    }
}

// Type check a reference node
void refTypeCheck(TypeCheckState *pstate, RefNode *node) {
    if (node->perm == unknownType)
        node->perm = newPermUseNode(node->vtexp->tag == FnSigTag ? opaqPerm :
        (node->region == borrowRef ? roPerm : uniPerm));
    iTypeTypeCheck(pstate, &node->region);
    if (node->region != borrowRef &&
            iTypeGetTypeDcl(node->region)->tag != StructTag) {
        errorMsgNode(node->region, ErrorInvType, "Reference's region must be a struct type.");
    }
    iTypeTypeCheck(pstate, (INode **) &node->perm);
    if (iTypeTypeCheck(pstate, &node->vtexp) == 0)
        return;
    refAdoptInfections(node);
    
    // Normalize reference type and point to its metadata
    node->typeinfo = typetblFind((INode*)node, refTypeInfoAlloc);
}

// Type check a virtual reference node
void refvirtTypeCheck(TypeCheckState *pstate, RefNode *node) {
    if (node->perm == unknownType)
        node->perm = newPermUseNode(node->region == borrowRef ? roPerm : uniPerm);
    iTypeTypeCheck(pstate, &node->region);
    iTypeTypeCheck(pstate, (INode **) &node->perm);
    if (iTypeTypeCheck(pstate, &node->vtexp) == 0)
        return;
    refAdoptInfections(node);

    StructNode *trait = (StructNode*) iTypeGetTypeDcl(node->vtexp);
    if (trait->tag != StructTag) {
        errorMsgNode((INode*)node, ErrorInvType, "A virtual reference must be to a struct or trait.");
        return;
    }

    // Build the Vtable info
    structMakeVtable(trait);
}

// Compare two reference signatures to see if they are equivalent
int refIsSame(RefNode *node1, RefNode *node2) {
    return iTypeIsSame(node1->vtexp, node2->vtexp)
           && permIsSame(node1->perm, node2->perm)
           && iTypeIsSame(node1->region, node2->region);
}

// Calculate hash for a structural reference type
size_t refHash(RefNode *node) {
    size_t hash = 5381 + node->tag;
    hash = ((hash << 5) + hash) ^ iTypeHash(node->region);
    hash = ((hash << 5) + hash) ^ iTypeHash(node->perm);
    return ((hash << 5) + hash) ^ iTypeHash(node->vtype);
}

// Compare two reference signatures to see if they are equivalent at runtime
int refIsRunSame(RefNode *node1, RefNode *node2) {
    return iTypeIsSame(node1->vtexp, node2->vtexp)
           && iTypeIsRunSame(node1->perm, node2->perm)
           && iTypeIsRunSame(node1->region, node2->region);
}

// Will from region coerce to a to region
TypeCompare regionMatches(INode *to, INode *from, SubtypeConstraint constraint) {
    if (iTypeIsSame(to, from))
        return EqMatch;
    if (to == borrowRef)
        return CastSubtype;
    return NoMatch;
}

// Will from-reference coerce to a to-reference (we know they are not the same)
TypeCompare refMatches(RefNode *to, RefNode *from, SubtypeConstraint constraint) {

    // Start with matching the references' regions
    TypeCompare result = regionMatches(from->region, to->region, constraint);
    if (result == NoMatch)
        return NoMatch;

    // Now their permissions
    switch (permMatches(to->perm, from->perm)) {
    case NoMatch: return NoMatch;
    case CastSubtype: result = CastSubtype;
    default: break;
    }

    // Now we get to value-type (which might include lifetime).
    // The variance of this match depends on the mutability/read permission of the reference
    TypeCompare match;
    switch (permGetFlags(to->perm) & (MayWrite | MayRead)) {
    case 0:
    case MayRead:
        match = iTypeMatches(to->vtexp, from->vtexp, Regref); // covariant
        break;
    case MayWrite:
        match = iTypeMatches(from->vtexp, to->vtexp, Regref); // contravariant
        break;
    case MayRead | MayWrite:
        return iTypeIsSame(to->vtexp, from->vtexp) ? result : NoMatch; // invariant
    }
    switch (match) {
    case EqMatch:
        return result;
    case CastSubtype:
        return CastSubtype;
    case ConvSubtype:
        return constraint == Monomorph ? ConvSubtype : NoMatch;
    default:
        return NoMatch;
    }
}

// Will from reference coerce to a virtual reference (we know they are not the same)
TypeCompare refvirtMatchesRef(RefNode *to, RefNode *from, SubtypeConstraint constraint) {
    // Given this performs a runtime conversion to a completely different type, 
    // it does not make sense for monomorphization
    if (constraint == Monomorph)
        return NoMatch;

    // Start with matching the references' regions
    TypeCompare result = regionMatches(from->region, to->region, constraint);
    if (result == NoMatch)
        return NoMatch;

    // Now their permissions
    switch (permMatches(to->perm, from->perm)) {
    case NoMatch: return NoMatch;
    case CastSubtype: result = CastSubtype;
    default: break;
    }

    // Handle value type without worrying about mutability-triggered variance
    // This is because a virtual reference "supertype" can never change the value's underlying type
    // Virtual references are based on structs/traits
    StructNode *tovtypedcl = (StructNode*) iTypeGetTypeDcl(to->vtexp);
    StructNode *fromvtypedcl = (StructNode*) iTypeGetTypeDcl(from->vtexp);
    if (tovtypedcl->tag != StructTag || fromvtypedcl->tag != StructTag)
        return NoMatch;

    // When value types are equivalent, ensure it is a closed, tagged trait.
    // The tag is needed to runtime select the vtable for the created virtual reference
    if (tovtypedcl == fromvtypedcl)
        return (fromvtypedcl->flags & HasTagField) ? ConvSubtype : NoMatch;

    // Use special structural subtyping logic to not only check compatibility,
    // but also to build vtable information
    switch (structVirtRefMatches((StructNode*)tovtypedcl, (StructNode*)fromvtypedcl)) {
    case EqMatch:
    case CastSubtype:
    case ConvSubtype:
        return ConvSubtype;  // Creating a virtual reference is always a conversion
    default:
        return NoMatch;
    }
}

// Will from virtual reference coerce to a virtual reference (we know they are not the same)
TypeCompare refvirtMatches(RefNode *to, RefNode *from, SubtypeConstraint constraint) {
    // For now, there is no supported way to convert a virtual ref from one value type to another
    // This could change later, maybe for monomorphization or maybe for runtime coercion
    if (!iTypeIsSame(to->vtexp, from->vtexp))
        return NoMatch;

    // However, region, permission and lifetime can be supertyped
    // Note: mutability variance on value type should be invariant, since underlying subtype won't change
    return refMatches(to, from, constraint);
}

// Return a type that is the supertype of both type nodes, or NULL if none found
INode *refFindSuper(INode *type1, INode *type2) {
    RefNode *typ1 = (RefNode *) iTypeGetTypeDcl(type1);
    RefNode *typ2 = (RefNode *) iTypeGetTypeDcl(type2);

    if (iTypeGetTypeDcl(typ1->region) != iTypeGetTypeDcl(typ2->region)
        || iTypeGetTypeDcl(typ1->perm) != iTypeGetTypeDcl(typ2->perm))
        return NULL;

    INode *vtexp = structRefFindSuper(typ1->vtexp, typ2->vtexp);
    if (vtexp == NULL)
        return NULL;

    return (INode*)newRefNodeFull(RefTag, (INode*)typ1, typ1->region, typ1->perm, vtexp);
}
