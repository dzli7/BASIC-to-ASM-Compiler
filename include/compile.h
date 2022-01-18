#ifndef COMPILE_H
#define COMPILE_H

#include <stdbool.h>

#include "ast.h"


bool check_constant(node_t *node);

size_t shift_left(int64_t val);

int64_t arith(node_t *node);
/**
 * Prints x86-64 assembly code that implements the given TeenyBASIC AST.
 *
 * @param node the statement to compile.
 *   The root node will be a SEQUENCE, PRINT, LET, IF, or WHILE.
 * @return true iff compilation succeeds
 */
bool compile_ast(node_t *node);

#endif /* COMPILE_H */
