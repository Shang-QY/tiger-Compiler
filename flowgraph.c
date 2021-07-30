#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "tree.h"
#include "absyn.h"
#include "assem.h"
#include "frame.h"
#include "graph.h"
#include "flowgraph.h"
#include "errormsg.h"
#include "table.h"

Temp_tempList FG_def(G_node n) {
	AS_instr instr = G_nodeInfo(n);
	switch(instr->kind)
	{
		case I_OPER:
			return instr->u.OPER.dst;
		case I_LABEL:
			return NULL;
		case I_MOVE:
			return instr->u.MOVE.dst;
	}
}

Temp_tempList FG_use(G_node n) {
	AS_instr instr = G_nodeInfo(n);
	switch(instr->kind)
	{
		case I_OPER:
			return instr->u.OPER.src;
		case I_LABEL:
			return NULL;
		case I_MOVE:
			return instr->u.MOVE.src;
	}
}

bool FG_isMove(G_node n) {
	AS_instr inst = (AS_instr)G_nodeInfo(n);
	return inst->kind == I_MOVE;
}

G_graph FG_AssemFlowGraph(AS_instrList il, F_frame f) {
	G_graph g = G_Graph();
	TAB_table t = TAB_empty();
	G_node prev = NULL;

	for (AS_instrList i = il; i; i=i->tail) {
		AS_instr inst = i->head;
		G_node node = G_Node(g, (void *)inst);
		if (prev) {
			G_addEdge(prev, node);
		}
		if (inst->kind == I_OPER && strncmp("\tjmp", inst->u.OPER.assem, 4) == 0) {
			prev = NULL;
		} else {
			prev = node;
		}
		if (inst->kind == I_LABEL) {
			TAB_enter(t, inst->u.LABEL.label, node);
		}
	}

	//add jump targets
	G_nodeList nodes = G_nodes(g);

	G_node target;
	for(;nodes;nodes = nodes->tail)
	{
		G_node node = nodes->head;
		AS_instr instr = G_nodeInfo(node);
		if(instr->kind == I_OPER && instr->u.OPER.jumps)
		{
			Temp_labelList labels = instr->u.OPER.jumps->labels;
			for(;labels;labels = labels->tail)
			{
				target = TAB_look(t,labels->head);
				if(target)
					G_addEdge(node,target);
				else
				{
					printf("Cannot find label %s\nSee in runtime.s or undefined label\n",Temp_labelstring(labels->head));//For debugging
				}
			}
		}
	}

	return g;
}
