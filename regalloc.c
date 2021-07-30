#include <stdio.h>
#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "tree.h"
#include "absyn.h"
#include "assem.h"
#include "frame.h"
#include "graph.h"
#include "liveness.h"
#include "color.h"
#include "regalloc.h"
#include "table.h"
#include "flowgraph.h"

struct RA_result RA_regAlloc(F_frame f, AS_instrList il) {
	struct RA_result ret;
	struct COL_result cr;

	bool noSpill = TRUE;
	do{
		noSpill = TRUE;
		G_graph fg = FG_AssemFlowGraph(il, f);
		struct Live_graph lg = Live_liveness(fg);
		cr = COL_color(lg.graph, F_regTempMap(), F_registers(), lg.moves);
		if (cr.spills) {
			il = AS_rewriteSpill(f, il, cr.spills);
			noSpill = FALSE;
		}
	}while(!noSpill);

	AS_rewrite(&il, cr.coloring);

	ret.coloring = cr.coloring;
	ret.il = il;

	return ret;
}
