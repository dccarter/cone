/** AST handling for pointers
 * @file
 *
 * This source file is part of the Cone Programming Language C compiler
 * See Copyright Notice in conec.h
*/

#include "../ast/ast.h"
#include "../shared/memory.h"
#include "../parser/lexer.h"
#include "../shared/symbol.h"

// Create a new pointer type whose info will be filled in afterwards
PtrTypeAstNode *newPtrTypeNode() {
	PtrTypeAstNode *ptrnode;
	newAstNode(ptrnode, PtrTypeAstNode, PtrType);
	return ptrnode;
}

// Serialize a pointer type
void ptrTypePrint(PtrTypeAstNode *node) {
	astFprint("&(");
	astPrintNode(node->alloc);
	astFprint(", ");
	astPrintNode((AstNode*)node->perm);
	astFprint(", ");
	astPrintNode(node->pvtype);
	astFprint(")");
}

// Semantically analyze a pointer type
void ptrTypePass(AstPass *pstate, PtrTypeAstNode *node) {
	astPass(pstate, node->alloc);
	astPass(pstate, (AstNode*)node->perm);
	astPass(pstate, node->pvtype);
}