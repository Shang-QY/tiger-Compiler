
/*Lab5: This header file is not complete. Please finish it with more definition.*/

#ifndef FRAME_H
#define FRAME_H

#include "tree.h"
#include "assem.h"

extern const int F_wordSize;
extern const int F_tempRegNum;
extern const int F_totalRegNum;

typedef struct F_accessList_ *F_accessList;

typedef struct F_frame_ *F_frame;
struct F_frame_ {
	T_stm shift;
	int size;
	F_accessList fmls;
	Temp_label name;
};

typedef struct F_access_ *F_access;
struct F_access_ {
	enum {inFrame, inReg} kind;
	union {
		int offset; //inFrame
		Temp_temp reg; //inReg
	} u;
};


struct F_accessList_ {F_access head; F_accessList tail;};
F_accessList F_AccessList(F_access head, F_accessList tail);

F_frame F_newFrame(Temp_label name, U_boolList formals);
Temp_label F_name(F_frame f);
F_accessList F_formals(F_frame f);
F_access F_allocLocal(F_frame f, bool escape);

/* IR Tree */
T_exp F_Exp(F_access acc, T_exp framePtr);

T_exp F_externalCall(string s, T_expList args);

/* declaration for fragments */
typedef struct F_frag_ *F_frag;
struct F_frag_ {enum {F_stringFrag, F_procFrag} kind;
			union {
				struct {Temp_label label; string str;} stringg;
				struct {T_stm body; F_frame frame;} proc;
			} u;
};

F_frag F_StringFrag(Temp_label label, string str);
F_frag F_ProcFrag(T_stm body, F_frame frame);

typedef struct F_fragList_ *F_fragList;
struct F_fragList_ 
{
	F_frag head; 
	F_fragList tail;
};

F_fragList F_FragList(F_frag head, F_fragList tail);

/* lab 6 */
Temp_map F_tempMap;
Temp_tempList F_registers(void);
Temp_tempList F_calleeSavedReg(void);
Temp_tempList F_callerSavedReg(void);
Temp_tempList F_paramReg(void);
Temp_map F_regTempMap();
Temp_temp F_FP(void);
Temp_temp F_SP(void);
Temp_temp F_RV(void);
Temp_temp F_rax();
Temp_temp F_rbx();
Temp_temp F_rcx();
Temp_temp F_rdx();
Temp_temp F_rsi();
Temp_temp F_rdi();
Temp_temp F_rbp();
Temp_temp F_rsp();
Temp_temp F_r8();
Temp_temp F_r9();
Temp_temp F_r10();
Temp_temp F_r11();
Temp_temp F_r12();
Temp_temp F_r13();
Temp_temp F_r14();
Temp_temp F_r15();

T_stm F_procEntryExit1(F_frame frame, T_stm stm);
AS_instrList F_procEntryExit2(AS_instrList body);
AS_proc F_procEntryExit3(F_frame frame, AS_instrList body);

Temp_labelList F_preDefineFuncs();

#endif
