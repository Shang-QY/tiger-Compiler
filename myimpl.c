#include "prog1.h"
#include <stdio.h>

int maxargs(A_stm stm)
{
	//TODO: put your code here.
    if(stm->kind == A_compoundStm){
        int arg1 = maxargs(stm->u.compound.stm1);
		int arg2 = maxargs(stm->u.compound.stm2);
		return arg1 > arg2 ? arg1 : arg2;
	}
	if(stm->kind == A_printStm){
		int _maxExpArgs = 0;
		int _maxArgs = 0;
		A_expList list = stm->u.print.exps;
		while (list->kind != A_lastExpList)
		{
			_maxArgs++;
			if(list->u.pair.head->kind == A_eseqExp){
				int arg = maxargs(list->u.pair.head->u.eseq.stm);
				if(_maxExpArgs < arg) _maxExpArgs = arg;
			}
			list = list->u.pair.tail;
		}
		_maxArgs++;
		if(list->u.last->kind == A_eseqExp){
			int arg = maxargs(list->u.pair.head->u.eseq.stm);
			if(_maxExpArgs < arg) _maxExpArgs = arg;
		}
		return _maxExpArgs > _maxArgs ? _maxExpArgs : _maxArgs;
	}
	else{
		if(stm->u.assign.exp->kind == A_eseqExp) return maxargs(stm->u.assign.exp->u.eseq.stm);
	}
	return 0;
}

typedef struct table *Table_;
struct table
{
	string id;
	int value;
	Table_ tail;
};
Table_ Table(string id, int value, struct table *tail){
	Table_ t = checked_malloc(sizeof(*t));
	t->id = id; t->value = value; t->tail = tail;
	return t;
}

Table_ update(Table_ t, string id, int value){
	return Table(id, value, t);
}

int lookup(Table_ t, string key){
	if(t == NULL) return -1;
	if(t->id == key) return t->value;
	while (t->tail != NULL)
	{
		t = t->tail;
		if(t->id == key) return t->value;
	}
	return -1;
}

struct IntAndTable {int i; Table_ t;};
struct IntAndTable interpExp(A_exp e, Table_ t);

Table_ interpStm(A_stm s, Table_ t);

void interp(A_stm stm)
{
	//TODO: put your code here.
    interpStm(stm, NULL);
}

struct IntAndTable interpExp(A_exp e, Table_ t){
    struct IntAndTable itable;
	if(e->kind == A_numExp){
		itable.i = e->u.num;
		itable.t = t;
		return itable;
	}
	if(e->kind == A_idExp){
		itable.i = lookup(t, e->u.id);
		itable.t = t;
		return itable;
	}
	if(e->kind == A_opExp){
		struct IntAndTable exp1 = interpExp(e->u.op.left, t);
        struct IntAndTable exp2 = interpExp(e->u.op.right, exp1.t);
		switch (e->u.op.oper)
		{
		case A_plus:
			itable.i = exp1.i + exp2.i;
			break;
		case A_minus:
		    itable.i = exp1.i - exp2.i;
			break;
		case A_times:
		    itable.i = exp1.i * exp2.i;
			break;
		default:
		    itable.i = exp1.i / exp2.i;
			break;
		}
		itable.t = exp2.t;
		return itable;
	}
	if(e->kind == A_eseqExp){
		t = interpStm(e->u.eseq.stm, t);
		return interpExp(e->u.eseq.exp, t);
	}
	itable.i = 0;
	itable.t = NULL;
	return itable;
}

Table_ interpStm(A_stm s, Table_ t){
    if(s->kind == A_compoundStm){
		t = interpStm(s->u.compound.stm1, t);
        return interpStm(s->u.compound.stm2, t);
	}
	if(s->kind == A_assignStm){
		struct IntAndTable itable = interpExp(s->u.assign.exp, t);
		t = itable.t;
		return update(t, s->u.assign.id, itable.i);
	}
    A_expList list = s->u.print.exps;
    while (list->kind != A_lastExpList)
	{
		struct IntAndTable itable = interpExp(list->u.pair.head, t);
		printf("%d ", itable.i);
		t = itable.t;
        list = list->u.pair.tail;
	}
	struct IntAndTable itable = interpExp(list->u.last, t);
	printf("%d\n", itable.i);
	t = itable.t;
	return t;
}