#include <stdio.h>
#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "tree.h"
#include "absyn.h"
#include "assem.h"
#include "frame.h"
#include "graph.h"
#include "flowgraph.h"
#include "liveness.h"
#include "table.h"

Live_moveList Live_MoveList(Live_move head, Live_moveList tail) {
	Live_moveList lm = (Live_moveList) checked_malloc(sizeof(*lm));
	lm->head = head;
	lm->tail = tail;
	return lm;
}

Live_move Live_Move(G_node src, G_node dst) {
	Live_move m = (Live_move) checked_malloc(sizeof(*m));
	m->src = src;
	m->dst = dst;
	return m;
}

Temp_temp Live_gtemp(G_node n) {
	Temp_temp t = G_nodeInfo(n);
	return t;
}

struct Live_graph Live_liveness(G_graph flow) {
	//your code here.
	struct Live_graph lg;
	lg.graph = G_Graph();
	lg.moves = NULL;
	
	G_nodeList flownodes = G_nodes(flow);
	G_table livein = G_empty();
	G_table liveout = G_empty();

	bool stable = FALSE;
	/* build up liveMap */
	while(!stable){
		stable = TRUE;
		for (G_nodeList nl = flownodes; nl; nl=nl->tail) {
			G_node node = nl->head;

			Temp_tempList in = G_look(livein, node);
			Temp_tempList out = G_look(liveout, node);

			G_nodeList succ = G_succ(node);
			Temp_tempList new_in = NULL, new_out = NULL;
			for (; succ; succ=succ->tail) {
				G_node s_node = succ->head;
				Temp_tempList s_in = G_look(livein, s_node);
				new_out = Temp_tempUnion(new_out, s_in);
			}

			// Temp_tempList out_sub = Temp_tempComplement(new_out, out);
			// if(out_sub){
			// 	stable = FALSE;
			// 	out = Temp_tempSplice(out, out_sub);
			// }

			new_in = Temp_tempUnion(FG_use(node), Temp_tempComplement(out,FG_def(node)));
			// Temp_tempList in_sub = Temp_tempComplement(new_in, in);
			// if(in_sub){
			// 	stable = FALSE;
			// 	in = Temp_tempSplice(in, in_sub);
			// }
			if(Temp_tempComplement(new_in, in) || Temp_tempComplement(new_out, out)){
				stable = FALSE;
			}

			G_enter(livein, node, new_in);
			G_enter(liveout, node, new_out);
		}
	}
	
	/* build up interference graph*/
	/* add node */
	TAB_table temp_to_node = TAB_empty();
	Temp_tempList added_temps = NULL;
	for (G_nodeList nl = flownodes; nl; nl=nl->tail) {
		G_node node = nl->head;
		Temp_tempList to_add = Temp_tempUnion(FG_def(node), FG_use(node));
		for (Temp_tempList tl = to_add; tl; tl = tl->tail) {
			Temp_temp t = tl->head;
			if (!Temp_tempIn(added_temps, t)) {
				TAB_enter(temp_to_node, t, G_Node(lg.graph, t));
				added_temps = Temp_TempList(t, added_temps);
			}
		}
	}
	/* add interference edge */
	for (G_nodeList nl = flownodes; nl; nl=nl->tail) {
		G_node node = nl->head;
		Temp_tempList def = FG_def(node);
		Temp_tempList out = G_look(liveout, node);
		Temp_tempList conflict = out;
		if (FG_isMove(node)) {
			Temp_tempList use = FG_use(node);
			G_node src = (G_node)TAB_look(temp_to_node, use->head);
			G_node dst = (G_node)TAB_look(temp_to_node, def->head);
			lg.moves = Live_MoveList(Live_Move(src, dst), lg.moves);
			
			conflict = Temp_tempComplement(out, use);
		}

		for (Temp_tempList tl = def; tl; tl=tl->tail) {
			Temp_temp td = tl->head;
			for (Temp_tempList tll = conflict; tll; tll=tll->tail) {
				Temp_temp tc = tll->head;
				if (td == tc) continue;
				G_node td_node = (G_node)TAB_look(temp_to_node, td);
				G_node tc_node = (G_node)TAB_look(temp_to_node, tc);
				G_addEdge(td_node, tc_node);
				G_addEdge(tc_node, td_node);
			}
		}
	}

	return lg;
}

bool Live_moveIn(Live_moveList ml, Live_move m) {
	for (; ml; ml=ml->tail) {
		if (ml->head->src == m->src && ml->head->dst == m->dst) {
			return TRUE;
		}
	}
	return FALSE;
}

Live_moveList Live_moveComplement(Live_moveList in, Live_moveList notin) {
	Live_moveList res = NULL;
	for (; in; in=in->tail) {
		if (!Live_moveIn(notin, in->head)) {
			res = Live_MoveList(in->head, res);
		}
	}
	return res;
}

Live_moveList Live_moveUnion(Live_moveList a, Live_moveList b) {
	Live_moveList s = Live_moveComplement(b, a);
	if (a==NULL) return s;
	while(a){
		s = Live_MoveList(a->head, s);
		a = a->tail;
	}
	return s;
}

Live_moveList Live_moveRemove(Live_moveList ml, Live_move m) {
	if(ml->head->src == m->src && ml->head->dst == m->dst) return ml->tail;
	Live_moveList origin = ml;
	Live_moveList succ = ml->tail;
	for (; succ; ml = ml->tail, succ = ml->tail) {
		if (succ->head->src == m->src && succ->head->dst == m->dst) {
			ml->tail = succ->tail;
			break;
		}
	}
	return origin;
}

Live_moveList Live_moveAppend(Live_moveList ml, Live_move m) {
	if (Live_moveIn(ml, m)) {
		return ml;
	} else {
		return Live_MoveList(m, ml);
	}
}