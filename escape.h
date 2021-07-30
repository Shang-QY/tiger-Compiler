#ifndef ESCAPE_H
#define ESCAPE_H

static void traverseDec(S_table env, int depth, A_dec d);
static void traverseVar(S_table env, int depth, A_var v);
static void traverseExp(S_table env, int depth, A_exp e);

void Esc_findEscape(A_exp exp);

#endif
