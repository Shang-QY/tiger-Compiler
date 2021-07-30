#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "util.h"
#include "errormsg.h"
#include "symbol.h"
#include "absyn.h"
#include "types.h"
#include "helper.h"
#include "env.h"
#include "semant.h"

/*Lab4: Your implementation of lab4*/


typedef void* Tr_exp;
struct expty 
{
	Tr_exp exp; 
	Ty_ty ty;
};

//In Lab4, the first argument exp should always be **NULL**.
struct expty expTy(Tr_exp exp, Ty_ty ty)
{
	struct expty e;

	e.exp = exp;
	e.ty = ty;

	return e;
}

Ty_ty actual_ty(Ty_ty ty)
{
	if (ty->kind == Ty_name) {
		return ty->u.name.ty;
	} else {
		return ty;
	}
}

struct expty transVar(S_table venv, S_table tenv, A_var v)
{
	switch (v->kind) {
		case A_simpleVar: {
			E_enventry x = S_look(venv, get_simplevar_sym(v));
			if (x && x->kind == E_varEntry) {
				return expTy(NULL, actual_ty(get_varentry_type(x)));
			} else {
				EM_error(v->pos, "undefined variable %s", S_name(get_simplevar_sym(v)));
				return expTy(NULL, Ty_Int());
			}
		}
		case A_fieldVar: {
			struct expty e = transVar(venv, tenv, get_fieldvar_var(v));
			if (get_expty_kind(e) == Ty_record) {
				Ty_fieldList f;
				for (f = get_record_fieldlist(e); f; f=f->tail) {
					if (f->head->name == get_fieldvar_sym(v)) {
						return expTy(NULL, actual_ty(f->head->ty));
					}
				}
				EM_error(get_fieldvar_var(v)->pos, "field %s doesn't exist", S_name(get_fieldvar_sym(v)));
				return expTy(NULL, Ty_Int());
			} else {
				EM_error(get_fieldvar_var(v)->pos, "not a record type");
				return expTy(NULL, Ty_Int());
			}
		}
		case A_subscriptVar: {
			struct expty ee = transExp(venv, tenv, get_subvar_exp(v));
			if (get_expty_kind(ee) != Ty_int) {
				EM_error(get_subvar_exp(v)->pos, "interger subscript required");
				return expTy(NULL, Ty_Int());
			}
			struct expty ev = transVar(venv, tenv, get_subvar_var(v));
			if (get_expty_kind(ev) == Ty_array) {
				return expTy(NULL, actual_ty(get_array(ev)));
			} else {
				EM_error(get_subvar_var(v)->pos, "array type required");
				return expTy(NULL, Ty_Int());
			}
		}
	}
}

struct expty transExp(S_table venv, S_table tenv, A_exp a)
{
	switch (a->kind) {
		case A_varExp: {
			return transVar(venv, tenv, a->u.var);
		}
		case A_nilExp: {
			return expTy(NULL, Ty_Nil());
		}
		case A_intExp: {
			return expTy(NULL, Ty_Int());
		}
		case A_stringExp: {
			return expTy(NULL, Ty_String());
		}
		case A_callExp: {
			E_enventry x = S_look(venv, get_callexp_func(a));
			if (!(x && x->kind == E_funEntry)) {
				EM_error(a->pos, "undefined function %s", S_name(get_callexp_func(a)));
				return expTy(NULL, Ty_Int());
			}
			Ty_tyList fTys;
			A_expList pTys;
			for (fTys = get_func_tylist(x), pTys = get_callexp_args(a); fTys; fTys=fTys->tail, pTys=pTys->tail) {
				if (pTys == NULL) {
					EM_error(a->pos, "para type mismatch");
					break;
				}
				struct expty e = transExp(venv, tenv, pTys->head);
				if (e.ty != fTys->head) {
					EM_error(a->pos, "para type mismatch");
					break;
				}
			}
			if (pTys != NULL) {
				EM_error(a->pos, "too many params in function %s", S_name(get_callexp_func(a)));
			}
			return expTy(NULL, actual_ty(get_func_res(x)));
		}
		case A_opExp: {
			A_oper oper = get_opexp_oper(a);
			struct expty left = transExp(venv, tenv, get_opexp_left(a));
			struct expty right = transExp(venv, tenv, get_opexp_right(a));
			if (oper == A_plusOp || oper == A_minusOp || oper == A_timesOp || oper == A_divideOp) {
				if (left.ty->kind != Ty_int) {
					EM_error(get_opexp_leftpos(a), "integer required");
				}
				if (right.ty->kind != Ty_int) {
					EM_error(get_opexp_rightpos(a), "integer required");
				}
				return expTy(NULL, Ty_Int());
			}
			if (oper == A_eqOp || oper == A_neqOp || oper == A_ltOp || oper == A_leOp || oper == A_gtOp || oper == A_geOp) {
				if (left.ty->kind != right.ty->kind) {
					EM_error(get_opexp_leftpos(a), "same type required");
				}
				return expTy(NULL, Ty_Int());
			}
		}
		case A_recordExp: {
			Ty_ty x = S_look(tenv, get_recordexp_typ(a));
			if (x == NULL) {
				EM_error(a->pos, "undefined type %s", S_name(get_recordexp_typ(a)));
				return expTy(NULL, Ty_Record(NULL));
			}
			/* TODO: filed check */
			return expTy(NULL, x);
		}
		case A_seqExp: {
			A_expList expList;
			struct expty e;
			for (expList = get_seqexp_seq(a); expList; expList=expList->tail) {
				e = transExp(venv, tenv, expList->head);
			}
			return e;
		}
		case A_assignExp: {
			struct expty ee = transExp(venv, tenv, get_assexp_exp(a));
			struct expty ev = transVar(venv, tenv, get_assexp_var(a));
			if (ee.ty != ev.ty) {
				EM_error(a->pos, "unmatched assign exp");
			}
			if (get_assexp_var(a)->kind == A_simpleVar) {
				E_enventry x = S_look(venv, get_simplevar_sym(get_assexp_var(a)));
				if (x && x->kind == E_varEntry) {
					if (get_varentry_type(x)->kind == Ty_name) {
						EM_error(a->pos, "loop variable can't be assigned");
					}
				}
			}
			return expTy(NULL, Ty_Void());
		}
		case A_ifExp: {
			transExp(venv, tenv, get_ifexp_test(a));
			struct expty et = transExp(venv, tenv, get_ifexp_then(a));
			struct expty ee = transExp(venv, tenv, get_ifexp_else(a));
			if (get_expty_kind(ee) == Ty_nil) {
				if (get_expty_kind(et) != Ty_void) {
					EM_error(a->pos, "if-then exp's body must produce no value");
				}
				return expTy(NULL, Ty_Void());
			} else {
				if (ee.ty != et.ty) {
					EM_error(a->pos, "then exp and else exp type mismatch");
				}
				return et;
			}
		}
		case A_whileExp: {
			transExp(venv, tenv, get_whileexp_test(a));
			struct expty e = transExp(venv, tenv, get_whileexp_body(a));
			if (get_expty_kind(e) != Ty_void) {
				EM_error(a->pos, "while body must produce no value");
			}
			return expTy(NULL, Ty_Void());
		}
		case A_forExp: {
			struct expty lo = transExp(venv, tenv, get_forexp_lo(a));
			if (get_expty_kind(lo) != Ty_int) {
				EM_error(get_forexp_lo(a)->pos, "for exp's range type is not integer");
			}
			struct expty hi = transExp(venv, tenv, get_forexp_hi(a));
			if (get_expty_kind(hi) != Ty_int) {
				EM_error(get_forexp_hi(a)->pos, "for exp's range type is not integer");
			}
			S_beginScope(venv);
			S_enter(venv, get_forexp_var(a), E_VarEntry(Ty_Name(get_forexp_var(a), Ty_Int())));
			struct expty body = transExp(venv, tenv, get_forexp_body(a));
			if (get_expty_kind(body) != Ty_void) {
				;
			}
			S_endScope(venv);
			return expTy(NULL, Ty_Void());
		}
		case A_breakExp: {
			/* TODO: break check */
			return expTy(NULL, Ty_Void());
		}
		case A_letExp: {
			struct expty e;
			A_decList d;
			S_beginScope(venv);
			S_beginScope(tenv);
			for (d = get_letexp_decs(a); d; d=d->tail) {
				transDec(venv, tenv, d->head);
			}
			e = transExp(venv, tenv, get_letexp_body(a));
			S_endScope(tenv);
			S_endScope(venv);
			return e;
		}
		case A_arrayExp: {
			Ty_ty t = S_look(tenv, get_arrayexp_typ(a));
			struct expty e = transExp(venv, tenv, get_arrayexp_init(a));
			if (e.ty != actual_ty(t)->u.array) {
				EM_error(a->pos, "type mismatch");
			}
			return expTy(NULL, actual_ty(t));
		}
	}
}

Ty_tyList makeFormalTyList(S_table tenv, A_fieldList fieldList)
{
	A_fieldList f;
	Ty_tyList th = NULL;
	Ty_tyList tl = NULL;
	Ty_ty t;
	for (f = fieldList; f; f=f->tail) {
		t = S_look(tenv, f->head->typ);
		if (t == NULL) {
			EM_error(f->head->pos, "undefined type %s", f->head->typ);
			t = Ty_Int();
		}
		if (th == NULL) {
			th = tl = Ty_TyList(t, NULL);
		} else {
			tl->tail = Ty_TyList(t, NULL);
			tl = tl->tail;
		}
	}
	return th;
}

void transDec(S_table venv, S_table tenv, A_dec d)
{
	switch (d->kind) {
		case A_functionDec: {
			A_fundecList fl, ffl;
			A_fundec f;
			Ty_ty resultTy;
			Ty_tyList formalTys;
			for (fl = get_funcdec_list(d); fl; fl=fl->tail) {
				f = fl->head;
				for (ffl = get_funcdec_list(d); ffl != fl; ffl=ffl->tail) {
					if (ffl->head->name == f->name) {
						EM_error(f->pos, "two functions have the same name");
						break;
					}
				}
				if (f->result) {
					resultTy = S_look(tenv, f->result);
				} else {
					resultTy = Ty_Void();
				}
				formalTys = makeFormalTyList(tenv, f->params);
				S_enter(venv, f->name, E_FunEntry(formalTys, resultTy));
			}

			for (fl = get_funcdec_list(d); fl; fl=fl->tail) {
				f = fl->head;
				E_enventry x = S_look(venv, f->name);
				S_beginScope(venv);
				A_fieldList l;
				Ty_tyList t;
				for (l = f->params, t = get_func_tylist(x); l; l=l->tail, t=t->tail) {
					S_enter(venv, l->head->name, E_VarEntry(t->head));
				}
				struct expty e = transExp(venv, tenv, f->body);
				if (get_func_res(x)->kind == Ty_void && get_expty_kind(e) != Ty_void) {
					EM_error(f->pos, "procedure returns value");
				}
				/* TODO: other return type check */
				S_endScope(venv);
			}
			break;
		}
		case A_varDec: {
			struct expty e = transExp(venv, tenv, get_vardec_init(d));
			if (get_vardec_typ(d) != NULL) {
				Ty_ty t = S_look(tenv, get_vardec_typ(d));
				if (t == NULL) {
					EM_error(d->pos, "undefined type %s", S_name(get_vardec_typ(d)));
				}
				if (t != e.ty) {
					EM_error(d->pos, "type mismatch");
				}
			} else {
				if (get_expty_kind(e) == Ty_nil) {
					EM_error(d->pos, "init should not be nil without type specified");
				}
			}
			S_enter(venv, get_vardec_var(d), E_VarEntry(e.ty));
			break;
		}
		case A_typeDec: {
			A_nametyList t, tt;
			for (t = get_typedec_list(d); t; t=t->tail) {
				for (tt = get_typedec_list(d); tt != t; tt=tt->tail) {
					if (tt->head->name == t->head->name) {
						EM_error(d->pos, "two types have the same name");
						break;
					}
				}
				S_enter(tenv, t->head->name, Ty_Name(t->head->name, NULL));
			}
			for (t = get_typedec_list(d); t; t=t->tail) {
				Ty_ty ty = S_look(tenv, t->head->name);
				ty->u.name.ty = transTy(tenv, t->head->ty);
			}
			int flag = 0;
			for (t = get_typedec_list(d); t; t=t->tail) {
				Ty_ty ty = S_look(tenv, t->head->name);
				Ty_ty tmp = ty;
				while (tmp->kind == Ty_name) {
					tmp = tmp->u.name.ty;
					if (tmp == ty) {
						EM_error(d->pos, "illegal type cycle");
						flag = 1;
						break;
					}
				}
				if (flag) break;
			}
			break;
		}
	}
}

Ty_ty transTy (S_table tenv, A_ty a)
{
	switch (a->kind) {
		case A_nameTy: {
			Ty_ty t = S_look(tenv, get_ty_name(a));
			if (t != NULL) {
				return t;
			} else {
				EM_error(a->pos, "undefined type %s", S_name(get_ty_name(a)));
				return Ty_Int();
			}
		}
		case A_recordTy: {
			A_fieldList f;
			Ty_fieldList tf = NULL;
			Ty_ty t;
			for (f = get_ty_record(a); f; f=f->tail) {
				t = S_look(tenv, f->head->typ);
				if (t == NULL) {
					EM_error(f->head->pos, "undefined type %s", S_name(f->head->typ));
					t = Ty_Int();
				}
				tf = Ty_FieldList(Ty_Field(f->head->name, t), tf);
			}
			return Ty_Record(tf);
		}
		case A_arrayTy: {
			Ty_ty t = S_look(tenv, get_ty_array(a));
			if (t != NULL) {
				return Ty_Array(t);
			} else {
				EM_error(a->pos, "undefined type %s", S_name(get_ty_array(a)));
				return Ty_Array(Ty_Int());
			}
		}
	}
}

void SEM_transProg(A_exp exp)
{
	S_table tenv = E_base_tenv();
	S_table venv = E_base_venv();
	transExp(venv, tenv, exp);
}