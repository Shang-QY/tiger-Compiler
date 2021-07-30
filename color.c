#include <stdio.h>
#include <string.h>

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
#include "table.h"

#define INFINITE 10000
static int K;
static Temp_tempList precolored;
static G_graph interferenceGraph;

int* lookupDegreeMap(G_node n) {
	return G_look(degreeTable, n);
}

G_node popSelectStack() {
	assert(selectStack!=NULL);
	G_node n = selectStack->head;
	selectStack = selectStack->tail;
	return n;
}

Live_moveList NodeMoves(G_node n) {
	Live_moveList relateMoves = G_look(moveListTable, n);
	Live_moveList notValidMoves = Live_moveComplement(relateMoves, Live_moveUnion(activeMoves, worklistMoves));
	return Live_moveComplement(relateMoves, notValidMoves);
}

bool MoveRelated(G_node n) {
	return NodeMoves(n) != NULL;
}

G_nodeList Adjacent(G_node n) {
	return G_nodeComplement(G_succ(n), G_nodeUnion(selectStack, coalescedNodes));
}

G_node GetAlias(G_node n) {
	if(G_nodeIn(coalescedNodes, n)){
		G_node alias = (G_node)G_look(aliasTable, n);
		return GetAlias(alias);
	}
	return n;
}

bool Precolored(G_node n) {
	return Temp_tempIn(precolored, Live_gtemp(n));
}

void Build(G_graph ig, Temp_tempList regs, Live_moveList moves) {

	K = F_totalRegNum;

	simplifyWorklist = NULL;
	freezeWorklist = NULL;
	spillWorklist = NULL;

	spilledNodes = NULL;
	coalescedNodes = NULL;
	coloredNodes = NULL;

	selectStack = NULL;

	precolored = regs;
	coalescedMoves = NULL;
	constrainedMoves = NULL;
	frozenMoves = NULL;
	worklistMoves = moves;
	activeMoves = NULL;

	degreeTable = G_empty();
	aliasTable = G_empty();
	colorTable = G_empty();
	moveListTable = G_empty();

	interferenceGraph = ig;

	G_nodeList nodes = G_nodes(ig);
	for(; nodes; nodes=nodes->tail){
		G_node n = nodes->head;
		/* Initial degree table*/
		int *degree = checked_malloc(sizeof(int));
		if(!Precolored(n)){
			*degree = G_degree(n)/2;
		}else{
			*degree = INFINITE;
		}
		G_enter(degreeTable, n, degree);

		/* Initial movelist table*/
		Live_moveList list = worklistMoves;
		Live_moveList movelist = NULL;
		for(;list;list = list->tail)
		{
			if(list->head->src == nodes->head || list->head->dst == nodes->head)
			{
				movelist = Live_MoveList(Live_Move(list->head->src,list->head->dst), movelist);
			}
		}
		G_enter(moveListTable,nodes->head,movelist);
	}
}

void MakeWorkList() {
	G_nodeList nodes = G_nodes(interferenceGraph);
	for (; nodes; nodes=nodes->tail) {
		G_node n = nodes->head;
		if (Precolored(n)) continue;
		if (*lookupDegreeMap(n) >= K) {
			spillWorklist = G_nodeAppend(spillWorklist, n);
		} else if (MoveRelated(n)) {
			freezeWorklist = G_nodeAppend(freezeWorklist, n);
		} else {
			simplifyWorklist = G_nodeAppend(simplifyWorklist, n);
		}
	}
}

void EnableMoves(G_nodeList nl) {
	for (; nl; nl=nl->tail) {
		G_node n = nl->head;
		Live_moveList ml = NodeMoves(n);
		for (; ml; ml=ml->tail) {
			Live_move m = ml->head;
			if (Live_moveIn(activeMoves, m)) {
				activeMoves = Live_moveRemove(activeMoves, m);
				worklistMoves = Live_moveAppend(worklistMoves, m);
			}
		}
	}
}

void DecrementDegree(G_node m) {
	int *degree = checked_malloc(sizeof(int));
	*degree = *lookupDegreeMap(m) - 1;
	G_enter(degreeTable, m, degree);
	if (*degree == K && !Precolored(m)) {
		EnableMoves(G_nodeAppend(Adjacent(m), m));
		spillWorklist = G_nodeRemove(spillWorklist, m);
		if (MoveRelated(m)) {
			freezeWorklist = G_nodeAppend(freezeWorklist, m);
		} else {
			simplifyWorklist = G_nodeAppend(simplifyWorklist, m);
		}
	}
}

void Simplify() {
	G_node n = simplifyWorklist->head;
	simplifyWorklist = simplifyWorklist->tail;
	selectStack = G_nodeAppend(selectStack, n);
	G_nodeList nl = Adjacent(n);
	for (; nl; nl=nl->tail) {
		G_node m = nl->head;
		DecrementDegree(m);
	}
}

void AddWorkList(G_node u) {
	if (!Precolored(u) && !MoveRelated(u) && *lookupDegreeMap(u) < K) {
		freezeWorklist = G_nodeRemove(freezeWorklist, u);
		simplifyWorklist = G_nodeAppend(simplifyWorklist, u);
	}
}

bool OK(G_node t, G_node v) {
	return *lookupDegreeMap(t) < K || Precolored(t) || G_goesTo(t, v);
}

bool Conservative(G_nodeList nl) {
	int k = 0;
	for (; nl; nl=nl->tail) {
		G_node node = nl->head;
		if (*lookupDegreeMap(node) >= K) {
			k = k + 1;
		}
	}
	return k < K;
}

void Combine(G_node u, G_node v) {
	if (G_nodeIn(freezeWorklist, v)) freezeWorklist = G_nodeRemove(freezeWorklist, v);
	if (G_nodeIn(spillWorklist, v)) spillWorklist = G_nodeRemove(spillWorklist, v);
	coalescedNodes = G_nodeAppend(coalescedNodes, v);
	G_enter(aliasTable, v, u);
	Live_moveList uml = G_look(moveListTable, u);
	Live_moveList vml = G_look(moveListTable, v);
	G_enter(moveListTable, u, Live_moveUnion(uml, vml));
	EnableMoves(G_NodeList(v, NULL));
	G_nodeList nl = Adjacent(v);
	for (; nl; nl=nl->tail) {
		G_node t = nl->head;
		if(!G_goesTo(t, u)){  //add edge between t,u
			G_addEdge(t, u);
			G_addEdge(u, t);
			int *u_degree = lookupDegreeMap(u);
			int *t_degree = lookupDegreeMap(t);
			(*u_degree)++;
			(*t_degree)++;
			G_enter(degreeTable, u, u_degree);
			G_enter(degreeTable, v, t_degree);
		}
		DecrementDegree(t);
	}
	if (*lookupDegreeMap(u) >= K && G_nodeIn(freezeWorklist, u)) {
		freezeWorklist = G_nodeRemove(freezeWorklist, u);
		spillWorklist = G_nodeAppend(spillWorklist, u);
	}
}

void Coalesce() {
	assert(worklistMoves != NULL);
	Live_move m = worklistMoves->head;

	G_node x = GetAlias(m->src);
	G_node y = GetAlias(m->dst);
	G_node u, v;
	if (Precolored(y)) {
		u = y;
		v = x;
	} else {
		u = x;
		v = y;
	}
	worklistMoves = Live_moveRemove(worklistMoves, m);
	if (u == v) {
		coalescedMoves = Live_moveAppend(coalescedMoves, m);
		AddWorkList(u);
	} else if (Precolored(v) || G_goesTo(u, v)) {
		constrainedMoves = Live_moveAppend(constrainedMoves, m);
		AddWorkList(u);
		AddWorkList(v);
	} else {
		G_nodeList nl = Adjacent(v);
		bool flag = TRUE;
		for (; nl; nl=nl->tail) {
			G_node t = nl->head;
			if (!OK(t, u)) {
				flag = FALSE;
				break;
			}
		}
		if (Precolored(u) && flag || !Precolored(u) && Conservative(G_nodeUnion(Adjacent(u), Adjacent(v)))) {
			coalescedMoves = Live_moveAppend(coalescedMoves, m);
			Combine(u, v);
			AddWorkList(u);
		} else {
			activeMoves = Live_moveAppend(activeMoves, m);
		}
	}
}

void FreezeMoves(G_node u) {
	Live_moveList ml = NodeMoves(u);
	for (; ml; ml=ml->tail) {
		Live_move m = ml->head;
		G_node x = m->src;
		G_node y = m->dst;
		G_node v;
		if (GetAlias(y) == GetAlias(u)) {
			v = GetAlias(x);
		} else {
			v = GetAlias(y);
		}
		activeMoves = Live_moveRemove(activeMoves, m);
		frozenMoves = Live_moveAppend(frozenMoves, m);
		if (!NodeMoves(v) && *lookupDegreeMap(v) < K) {
			freezeWorklist = G_nodeRemove(freezeWorklist, v);
			simplifyWorklist = G_nodeAppend(simplifyWorklist, v);
		}
	}
}

void Freeze() {
	G_node u = freezeWorklist->head;
	freezeWorklist = G_nodeRemove(freezeWorklist, u);
	simplifyWorklist = G_nodeAppend(simplifyWorklist, u);
	FreezeMoves(u);
}

void SelectSpill() {
	G_node maxDegreeNode = NULL; 
	int maxDegree = -1;
	for (G_nodeList nl = spillWorklist; nl; nl=nl->tail) {
		G_node n = nl->head;
		assert(!Precolored(n));
		int degree = *lookupDegreeMap(n);
		if(degree > maxDegree){
			maxDegree = degree;
			maxDegreeNode = n;
		}
	}
	spillWorklist = G_nodeRemove(spillWorklist, maxDegreeNode);
	simplifyWorklist = G_nodeAppend(simplifyWorklist, maxDegreeNode);
	FreezeMoves(maxDegreeNode);
}

void AssignColors() {
	for (G_nodeList nl = G_nodes(interferenceGraph); nl; nl=nl->tail) {
		G_node n = nl->head;
		if (Precolored(n)) {
			G_enter(colorTable, n, Live_gtemp(n));
			coloredNodes = G_nodeAppend(coloredNodes, n);
		}
	}
	while (selectStack) {
		G_node n = popSelectStack();
		if (G_nodeIn(coloredNodes, n)) continue;
		Temp_tempList okColors = precolored;
		G_nodeList adj = G_adj(n);
		for (; adj; adj=adj->tail) {
			G_node w = adj->head;
			if (G_nodeIn(coloredNodes, GetAlias(w)) || Precolored(GetAlias(w))) {
				Temp_temp color = (Temp_temp)G_look(colorTable, GetAlias(w));
				okColors = Temp_tempComplement(okColors, Temp_TempList(color, NULL));
			}
		}
		if (!okColors) {
			spilledNodes = G_nodeAppend(spilledNodes, n);
		} else {
			coloredNodes = G_nodeAppend(coloredNodes, n);
			G_enter(colorTable, n, okColors->head);
		}
	}
	G_nodeList nl = coalescedNodes;
	for (; nl; nl=nl->tail) {
		G_node n = nl->head;
		Temp_temp color = (Temp_temp)G_look(colorTable, GetAlias(n));
		G_enter(colorTable, n, color);
	}
}

struct COL_result COL_color(G_graph ig, Temp_map initial, Temp_tempList regs, Live_moveList moves) {
	struct COL_result ret;
	Build(ig, regs, moves);
	MakeWorkList();

	do{
		if(simplifyWorklist){
			Simplify();
		}else if(worklistMoves){
			Coalesce();
		}else if(freezeWorklist){
			Freeze();
		}else if (spillWorklist){
			SelectSpill();
		}
	}while(simplifyWorklist || worklistMoves || freezeWorklist || spillWorklist);
	AssignColors();

	Temp_tempList spills = NULL;
	for(G_nodeList nl = spilledNodes; nl; nl=nl->tail){
		G_node n = nl->head;
		spills = Temp_TempList(Live_gtemp(n), spills);
	}
	ret.spills = spills;
	if(spills) return ret;

	Temp_map coloring = Temp_empty();
	G_nodeList nl = G_nodes(interferenceGraph);
	for(; nl; nl=nl->tail){
		G_node n = nl->head;
		Temp_temp color = (Temp_temp)G_look(colorTable, n);
		if(color){
			Temp_enter(coloring, Live_gtemp(n), Temp_look(initial, color));
		}
	}
	ret.coloring = Temp_layerMap(coloring, initial);
	return ret;
}
