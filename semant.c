#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "util.h"
#include "errormsg.h"
#include "symbol.h"
#include "absyn.h"
#include "prabsyn.h"
#include "types.h"
#include "env.h"
#include "semant.h"
#include "helper.h"
#include "translate.h"

/*Lab5: Your implementation of lab5.*/

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

Ty_ty actualType(Ty_ty ty)
{
	if (ty == NULL) return Ty_Void();
	if (ty->kind == Ty_name) {
		return actualType(ty->u.name.ty);
	} else {
		return ty;
	}
}

bool sameType(Ty_ty t1, Ty_ty t2)
{
	Ty_ty type1 = actualType(t1);
	Ty_ty type2 = actualType(t2);

	if(type1->kind == type2->kind){
		if(type1->kind == Ty_record || type1->kind == Ty_array){
			if(type1->u.record == type2->u.record) return 1;
			else return 0;
		}
		else return 1;
	}
	else{
		if((type1->kind == Ty_nil && type2->kind == Ty_record) || (type1->kind == Ty_record && type2->kind == Ty_nil)){
			return 1;
		}
		else return 0;
	}
}

struct expty transVar(S_table venv, S_table tenv, A_var v, Tr_level l, Temp_label label)
{
	switch (v->kind) {
		case A_simpleVar: {
			E_enventry x = S_look(venv, get_simplevar_sym(v));
			if (x && x->kind == E_varEntry) {
				Tr_exp simpvar=Tr_simpleVar(get_var_access(x),l);
				return expTy(simpvar, actualType(get_varentry_type(x)));
			} else {
				EM_error(v->pos, "undefined variable %s", S_name(get_simplevar_sym(v)));
				return expTy(Tr_noExp(), Ty_Int());
			}
		}
		case A_fieldVar: {
			struct expty e = transVar(venv, tenv, get_fieldvar_var(v), l, label);
			if (get_expty_kind(e) == Ty_record) {
				Ty_fieldList rec_fieldlist;
				Ty_field field;
				int i = 0;
				for (rec_fieldlist = get_record_fieldlist(e); rec_fieldlist; rec_fieldlist = rec_fieldlist->tail, i++) {
					field = rec_fieldlist->head;
					if (field->name == get_fieldvar_sym(v)) {
						return expTy(Tr_fieldVar(e.exp, i), actualType(field->ty));
					}
				}
				EM_error(get_fieldvar_var(v)->pos, "field %s doesn't exist", S_name(get_fieldvar_sym(v)));
				return expTy(Tr_noExp(), Ty_Int());
			} else {
				EM_error(get_fieldvar_var(v)->pos, "not a record type");
				return expTy(Tr_noExp(), Ty_Int());
			}
		}
		case A_subscriptVar: {
			struct expty var = transVar(venv, tenv, get_subvar_var(v), l, label);
			if (get_expty_kind(var) != Ty_array) {
				EM_error(get_subvar_var(v)->pos, "array type required");
				return expTy(Tr_noExp(), Ty_Int());
			}
			else{
				struct expty subscript = transExp(venv, tenv, get_subvar_exp(v), l, label);
				if (get_expty_kind(subscript) != Ty_int) {
					EM_error(get_subvar_exp(v)->pos, "subscript is not int");
					return expTy(Tr_noExp(), Ty_Int());
				}
				return expTy(Tr_subscriptVar(var.exp, subscript.exp), actualType(get_array(var)));
			}
		}
	}
}

struct expty transExp(S_table venv, S_table tenv, A_exp a, Tr_level l, Temp_label label)
{
	switch (a->kind) {
		case A_varExp: {
			return transVar(venv, tenv, a->u.var, l, label);
		}
		case A_nilExp: {
			return expTy(Tr_nilExp(), Ty_Nil());
		}
		case A_intExp: {
			return expTy(Tr_intExp(a->u.intt), Ty_Int());
		}
		case A_stringExp: {
			return expTy(Tr_stringExp(a->u.stringg), Ty_String());
		}
		case A_callExp: {
			E_enventry x = S_look(venv, get_callexp_func(a));
			if (!(x && x->kind == E_funEntry)) {
				EM_error(a->pos, "undefined function %s", S_name(get_callexp_func(a)));
				return expTy(Tr_noExp(), Ty_Void());
			}
			Ty_tyList ftypelist;
			A_expList paras;
			Tr_expList args = NULL;
			Tr_expList tail = NULL;
			for (ftypelist = get_func_tylist(x), paras = get_callexp_args(a); ftypelist; ftypelist=ftypelist->tail, paras=paras->tail) {
				if (paras == NULL) {
					EM_error(a->pos, "para type mismatch");
					return expTy(Tr_noExp(), Ty_Void());
				}
				struct expty e = transExp(venv, tenv, paras->head, l, label);
				if (!sameType(e.ty,ftypelist->head)) {
					EM_error(a->pos, "para type mismatch");
					return expTy(Tr_noExp(), Ty_Void());
				}
				if (args == NULL) {
					args = tail = Tr_ExpList(e.exp, NULL);
				} else {
					tail->tail = Tr_ExpList(e.exp, NULL);
					tail = tail->tail;
				}
			}
			if (paras != NULL) {
				EM_error(a->pos, "too many params in function %s", S_name(get_callexp_func(a)));
				return expTy(Tr_noExp(), Ty_Void());
			}
			return expTy(Tr_callExp(get_func_label(x), get_func_level(x), l, args), actualType(get_func_res(x)));
		}
		case A_opExp: {
			A_oper oper = get_opexp_oper(a);
			struct expty left = transExp(venv, tenv, get_opexp_left(a), l, label);
			struct expty right = transExp(venv, tenv, get_opexp_right(a), l, label);
			if (oper == A_plusOp || oper == A_minusOp || oper == A_timesOp || oper == A_divideOp) {
				if (left.ty->kind != Ty_int) {
					EM_error(get_opexp_leftpos(a), "integer required");
				}
				if (right.ty->kind != Ty_int) {
					EM_error(get_opexp_rightpos(a), "integer required");
				}
				return expTy(Tr_binExp(oper, left.exp, right.exp), Ty_Int());
			}
			if (oper == A_eqOp || oper == A_neqOp || oper == A_ltOp || oper == A_leOp || oper == A_gtOp || oper == A_geOp) {
				if (!sameType(left.ty,right.ty)) {
					EM_error(get_opexp_leftpos(a), "same type required");
				}
				if (left.ty == Ty_String() && right.ty == Ty_String()) {
					return expTy(Tr_stringEqualExp(left.exp, right.exp), Ty_Int());
				}
				return expTy(Tr_relExp(oper, left.exp, right.exp), Ty_Int());
			}
		}
		case A_recordExp: {
			Ty_ty x = actualType(S_look(tenv, get_recordexp_typ(a)));
			if (x == NULL) {
				EM_error(a->pos, "undefined type %s", S_name(get_recordexp_typ(a)));
				return expTy(Tr_noExp(), Ty_Record(NULL));
			}
			Tr_expList fields = NULL;
			Tr_expList tail = NULL;
			A_efieldList el = get_recordexp_fields(a);
			int n = 0;
			for (; el; el=el->tail, n++) {
				struct expty e = transExp(venv, tenv, el->head->exp, l, label);
				if (fields == NULL) {
					fields = tail = Tr_ExpList(e.exp, NULL);
				} else {
					tail->tail = Tr_ExpList(e.exp, NULL);
					tail = tail->tail;
				}
			}
			return expTy(Tr_recordExp(n, fields), x);
		}
		case A_seqExp: {
			A_expList expList = get_seqexp_seq(a);
			if (expList == NULL) {
				return expTy(Tr_noExp(), Ty_Void());
			}
			struct expty e;
			Tr_expList seq = NULL;
			Tr_expList tail = NULL;
			for (; expList; expList=expList->tail) {
				e = transExp(venv, tenv, expList->head, l, label);
				if (seq == NULL) {
					seq = tail = Tr_ExpList(e.exp, NULL);
				} else {
					tail->tail = Tr_ExpList(e.exp, NULL);
					tail = tail->tail;
				}
			}
			return expTy(Tr_seqExp(seq), e.ty);
		}
		case A_assignExp: {
			struct expty ee = transExp(venv, tenv, get_assexp_exp(a), l, label);
			struct expty ev = transVar(venv, tenv, get_assexp_var(a), l, label);
			if (!sameType(ee.ty, ev.ty)) {
				EM_error(a->pos, "unmatched assign exp");
			}
			if (get_assexp_var(a)->kind == A_simpleVar) {
				E_enventry x = S_look(venv, get_simplevar_sym(get_assexp_var(a)));
				if (x && x->kind == E_varEntry) {
					if (x->readonly == 1) {
						EM_error(a->pos, "loop variable can't be assigned");
					}
				}
			}
			return expTy(Tr_assignExp(ev.exp, ee.exp), Ty_Void());
		}
		case A_ifExp: {
			struct expty ei = transExp(venv, tenv, get_ifexp_test(a), l, label);
			struct expty et = transExp(venv, tenv, get_ifexp_then(a), l, label);
			if (get_ifexp_else(a) == NULL) {
				if (get_expty_kind(et) != Ty_void) {
					EM_error(a->pos, "if-then exp's body must produce no value");
				}
				return expTy(Tr_ifExp(ei.exp, et.exp, NULL), Ty_Void());
			} else {
				struct expty ee = transExp(venv, tenv, get_ifexp_else(a), l, label);
				if (!sameType(ee.ty, et.ty)) {
					EM_error(a->pos, "then exp and else exp type mismatch");
				}
				return expTy(Tr_ifExp(ei.exp, et.exp, ee.exp), et.ty);
			}
		}
		case A_whileExp: {
			struct expty test = transExp(venv, tenv, get_whileexp_test(a), l, label);
			Temp_label done = Temp_newlabel();
			struct expty body = transExp(venv, tenv, get_whileexp_body(a), l, done);
			if (get_expty_kind(body) != Ty_void) {
				EM_error(a->pos, "while body must produce no value");
			}
			return expTy(Tr_whileExp(test.exp, body.exp, done), Ty_Void());
		}
		case A_forExp: {
			struct expty lo = transExp(venv, tenv, get_forexp_lo(a), l, label);
			if (get_expty_kind(lo) != Ty_int) {
				EM_error(get_forexp_lo(a)->pos, "for exp's range type is not integer");
			}
			struct expty hi = transExp(venv, tenv, get_forexp_hi(a), l, label);
			if (get_expty_kind(hi) != Ty_int) {
				EM_error(get_forexp_hi(a)->pos, "for exp's range type is not integer");
			}
			Tr_access ta = Tr_allocLocal(l, TRUE);
			S_beginScope(venv);
			S_enter(venv, get_forexp_var(a), E_ROVarEntry(ta, Ty_Int()));
			Temp_label done = Temp_newlabel();
			struct expty body = transExp(venv, tenv, get_forexp_body(a), l, done);
			S_endScope(venv);
			return expTy(Tr_forExp(ta, lo.exp, hi.exp, body.exp, done), Ty_Void());
		}
		case A_breakExp: {
			if (!label) {
				return expTy(Tr_noExp(), Ty_Void());
			}
			return expTy(Tr_breakExp(label), Ty_Void());
		}
		case A_letExp: {
			A_decList d;
			Tr_expList exps = NULL;
			Tr_expList tail = NULL;
			S_beginScope(venv);
			S_beginScope(tenv);
			for (d = get_letexp_decs(a); d; d=d->tail) {
				Tr_exp exp = transDec(venv, tenv, d->head, l, label);
				if (exps == NULL) {
					exps = tail = Tr_ExpList(exp, NULL);
				} else {
					tail->tail = Tr_ExpList(exp, NULL);
					tail = tail->tail;
				}
			}
			struct expty e = transExp(venv, tenv, get_letexp_body(a), l, label);
			tail->tail = Tr_ExpList(e.exp, NULL);
			S_endScope(tenv);
			S_endScope(venv);
			return expTy(Tr_seqExp(exps), e.ty);
		}
		case A_arrayExp: {
			Ty_ty t = actualType(S_look(tenv, get_arrayexp_typ(a)));
			struct expty size = transExp(venv, tenv, get_arrayexp_size(a), l, label);
			struct expty init = transExp(venv, tenv, get_arrayexp_init(a), l, label);
			if (!sameType(init.ty, actualType(t)->u.array)) {
				EM_error(a->pos, "type mismatch");
				return expTy(Tr_noExp(), Ty_Int());
			}
			return expTy(Tr_arrayExp(size.exp, init.exp), actualType(t));
		}
	}
}

Ty_tyList makeFormalTyList(S_table tenv, A_fieldList fieldList)
{
	Ty_tyList tlhead = NULL;
	Ty_tyList tltail = NULL;
	for (A_fieldList f = fieldList; f; f = f->tail) {
		Ty_ty t = S_look(tenv, f->head->typ);
		if (t == NULL) {
			EM_error(f->head->pos, "undefined type %s", f->head->typ);
			t = Ty_Int();
		}
		if (tlhead == NULL) {
			tlhead = tltail = Ty_TyList(t, NULL);
		} else {
			tltail->tail = Ty_TyList(t, NULL);
			tltail = tltail->tail;
		}
	}
	return tlhead;
}

U_boolList makeFormalBoolList(A_fieldList fieldList)
{
	U_boolList head = NULL;
	U_boolList tail = NULL;
	for (A_fieldList f = fieldList; f; f = f->tail) {
		bool escape = f->head->escape;
		if (head == NULL) {
			head = tail = U_BoolList(escape, NULL);
		} else {
			tail->tail = U_BoolList(escape, NULL);
			tail = tail->tail;
		}
	}
	return head;
}

Tr_exp transDec(S_table venv, S_table tenv, A_dec d, Tr_level l, Temp_label label)
{
	switch (d->kind) {
		case A_functionDec: {
			for (A_fundecList fl = get_funcdec_list(d); fl; fl=fl->tail) {
				A_fundec f = fl->head;
				for (A_fundecList ffl = get_funcdec_list(d); ffl != fl; ffl=ffl->tail) {
					if (ffl->head->name == f->name) {
						EM_error(f->pos, "two functions have the same name");
						break;
					}
				}
				Ty_ty resultTy;
				if (f->result) {
					resultTy = S_look(tenv, f->result);
				} else {
					resultTy = Ty_Void();
				}
				Ty_tyList formalTys = makeFormalTyList(tenv, f->params);
				U_boolList formalEscapes = makeFormalBoolList(f->params);
				Temp_label newLabel = Temp_namedlabel(S_name(f->name));
				Tr_level newLevel = Tr_newLevel(l, newLabel, formalEscapes);
				S_enter(venv, f->name, E_FunEntry(newLevel, newLabel, formalTys, resultTy));
			}
			for (A_fundecList fl = get_funcdec_list(d); fl; fl=fl->tail) {
				A_fundec f = fl->head;
				E_enventry x = S_look(venv, f->name);
				S_beginScope(venv);
				A_fieldList l;
				Ty_tyList t;
				Tr_accessList al = Tr_formals(get_func_level(x));
				for (l = f->params, t = get_func_tylist(x); l; l=l->tail, t=t->tail, al=al->tail) {
					S_enter(venv, l->head->name, E_VarEntry(al->head, t->head));
				}

				struct expty e = transExp(venv, tenv, f->body, get_func_level(x), label);

				if (get_func_res(x)->kind == Ty_void && get_expty_kind(e) != Ty_void) {
					EM_error(f->pos, "procedure returns value");
				}
				Tr_procEntryExit(get_func_level(x), e.exp, al);
				S_endScope(venv);
			}
			return Tr_noExp();
		}
		case A_varDec: {
			struct expty e = transExp(venv, tenv, get_vardec_init(d), l, label);
			Tr_access a = Tr_allocLocal(l, d->u.var.escape);
			if (get_vardec_typ(d) != NULL) {
				Ty_ty t = S_look(tenv, get_vardec_typ(d));
				if (t == NULL) {
					EM_error(d->pos, "undefined type %s", S_name(get_vardec_typ(d)));
				} else {
					if (!sameType(t, e.ty)) {
						EM_error(d->pos, "type mismatch");
					}
					S_enter(venv, get_vardec_var(d), E_VarEntry(a, t));
				}
			} else {
				if (get_expty_kind(e) == Ty_nil) {
					EM_error(d->pos, "init should not be nil without type specified");
					S_enter(venv, get_vardec_var(d), E_VarEntry(a, Ty_Int()));
				} else {
					S_enter(venv, get_vardec_var(d), E_VarEntry(a, e.ty));
				}
			}
			return Tr_assignExp(Tr_simpleVar(a, l), e.exp);
		}
		case A_typeDec: {
			for (A_nametyList t = get_typedec_list(d); t; t=t->tail) {
				for (A_nametyList tt = get_typedec_list(d); tt != t; tt=tt->tail) {
					if (tt->head->name == t->head->name) {
						EM_error(d->pos, "two types have the same name");
						break;
					}
				}
				S_enter(tenv, t->head->name, Ty_Name(t->head->name, NULL));
			}
			for (A_nametyList t = get_typedec_list(d); t; t=t->tail) {
				Ty_ty ty = S_look(tenv, t->head->name);
				ty->u.name.ty = transTy(tenv, t->head->ty);
			}
			int cycle = 0;
			int count = 0;
			for (A_nametyList t = get_typedec_list(d); t; t=t->tail) {
				Ty_ty ty = S_look(tenv, t->head->name);
				Ty_ty tmp = ty;
				while (tmp->kind == Ty_name) {
					count++;
					assert(count < 10000);
					tmp = tmp->u.name.ty;
					if (tmp == ty) {
						EM_error(d->pos, "illegal type cycle");
						cycle = 1;
						break;
					}
				}
				if (cycle) break;
			}
			return Tr_noExp();
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
			Ty_fieldList flhead = NULL;
			Ty_fieldList fltail = NULL;
			for (A_fieldList f = get_ty_record(a); f; f=f->tail) {
				Ty_ty t = S_look(tenv, f->head->typ);
				if (t == NULL) {
					EM_error(f->head->pos, "undefined type %s", S_name(f->head->typ));
					t = Ty_Int();
				}
				if (flhead == NULL) {
					flhead = fltail = Ty_FieldList(Ty_Field(f->head->name, t), NULL);
				} else {
					fltail->tail = Ty_FieldList(Ty_Field(f->head->name, t), NULL);
					fltail = fltail->tail;
				}
				
			}
			return Ty_Record(flhead);
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

F_fragList SEM_transProg(A_exp exp)
{
	S_table t = E_base_tenv();
	S_table v = E_base_venv();
	Temp_label mainLabel = Temp_namedlabel("tigermain");
	Tr_level mainLevel = Tr_newLevel(Tr_outermost(), mainLabel, NULL);
	struct expty main = transExp(v, t, exp, mainLevel, NULL);
	Tr_procEntryExit(mainLevel, main.exp, NULL);
	return Tr_getResult();
}

