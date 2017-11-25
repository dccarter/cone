/** Permission Types
 * @file
 *
 * This source file is part of the Cone Programming Language C compiler
 * See Copyright Notice in conec.h
*/

#include "type.h"
#include "../shared/memory.h"

// Macro for creating permission types
#define permtype(dest, typ, ptyp, flgs) {\
	dest = (AstNode*) (perm = allocTypeAstNode(PermTypeAstNode)); \
	perm->asttype = typ; \
	perm->ptype = ptyp; \
	perm->flags = flgs; \
	perm->locker = NULL; \
}

// Initialize built-in static permission type global variables
void permInit() {
	PermTypeAstNode *perm;
	permtype(mutPerm, PermType, MutPerm, MayRead | MayWrite | RaceSafe | MayIntRef | IsLockless);
	permtype(mmutPerm, PermType, MmutPerm, MayRead | MayWrite | MayAlias | MayAliasWrite | IsLockless);
	permtype(immPerm, PermType, ImmPerm, MayRead | MayAlias | RaceSafe | MayIntRef | IsLockless);
	permtype(constPerm, PermType, ConstPerm, MayRead | MayAlias | IsLockless);
	permtype(constxPerm, PermType, ConstxPerm, MayRead | MayAlias | MayIntRef | IsLockless);
	permtype(mutxPerm, PermType, MutxPerm, MayRead | MayWrite | MayAlias | MayIntRef | IsLockless);
	permtype(idPerm, PermType, IdPerm, MayAlias | RaceSafe | IsLockless);
}