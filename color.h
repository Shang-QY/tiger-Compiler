/*
 * color.h - Data structures and function prototypes for coloring algorithm
 *             to determine register allocation.
 */

#ifndef COLOR_H
#define COLOR_H

struct COL_result {Temp_map coloring; Temp_tempList spills;};
struct COL_result COL_color(G_graph ig, Temp_map initial, Temp_tempList regs, Live_moveList moves);

void Build();
void MakeWorklist();
void Simplify();
void Coalesce();
void Freeze();
void SelectSpill();
void AssignColors();
bool MoveRelated(G_node node);
G_nodeList Adjacent(G_node n);
Live_moveList NodeMoves(G_node n);
void DecrementDegree(G_node n);
void EnableMoves(G_nodeList nodes);

/* Coalesce functions */
G_node GetAlias(G_node t);
void AddWorkList(G_node n);
bool OK(G_node v,G_node u);
bool Conservative(G_nodeList adj);
void Combine(G_node u,G_node v);
void AddEdge(G_node u,G_node v);

void FreezeMoves(G_node n);

/* Tool Functions*/
bool Precolored(G_node n);
int* lookupDegreeMap(G_node n);
G_node popSelectStack();

/* Data Stucture*/
//Worklist
static G_nodeList simplifyWorklist;
static G_nodeList freezeWorklist;
static G_nodeList spillWorklist;

//Node sets
//static G_nodeList precolored;
static G_nodeList coloredNodes;
static G_nodeList spilledNodes;
static G_nodeList coalescedNodes;

static Temp_tempList notSpillTemps;

//Movelist
static Live_moveList coalescedMoves;
static Live_moveList constrainedMoves;
static Live_moveList frozenMoves;
static Live_moveList worklistMoves;
static Live_moveList activeMoves;

//stack 
static G_nodeList selectStack;

//Nodes information
//The binding is a pointer.
static G_table degreeTable; // binding points to an int (degree).
static G_table colorTable; //binding points to an int (color).
static G_table aliasTable; // binding points to a G_node pointer.
static G_table moveListTable;// binding points to a Live_movelist pointer.

#endif
