#ifndef LIVENESS_H
#define LIVENESS_H

typedef struct Live_move_ *Live_move;
struct Live_move_ {
	G_node src, dst;
};
Live_move Live_Move(G_node src, G_node dst);

typedef struct Live_moveList_ *Live_moveList;
struct Live_moveList_ {
	Live_move head;
	Live_moveList tail;
};
Live_moveList Live_MoveList(Live_move head, Live_moveList tail);


struct Live_graph {
	G_graph graph;
	Live_moveList moves;
};
Temp_temp Live_gtemp(G_node n);

struct Live_graph Live_liveness(G_graph flow);

/* Tool Functions*/
bool Live_moveIn(Live_moveList ml, Live_move m);
Live_moveList Live_moveComplement(Live_moveList in, Live_moveList notin);
Live_moveList Live_moveUnion(Live_moveList a, Live_moveList b);
Live_moveList Live_moveRemove(Live_moveList ml, Live_move m);
Live_moveList Live_moveAppend(Live_moveList ml, Live_move m);

#endif
