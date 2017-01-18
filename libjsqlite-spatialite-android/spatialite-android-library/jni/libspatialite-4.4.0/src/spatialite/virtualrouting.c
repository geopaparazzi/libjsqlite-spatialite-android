/*

 virtualrouting.c -- SQLite3 extension [VIRTUAL TABLE Routing - shortest path]

 version 4.4, 2016 October 7

 Author: Sandro Furieri a.furieri@lqt.it

 -----------------------------------------------------------------------------
 
 Version: MPL 1.1/GPL 2.0/LGPL 2.1
 
 The contents of this file are subject to the Mozilla Public License Version
 1.1 (the "License"); you may not use this file except in compliance with
 the License. You may obtain a copy of the License at
 http://www.mozilla.org/MPL/
 
Software distributed under the License is distributed on an "AS IS" basis,
WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
for the specific language governing rights and limitations under the
License.

The Original Code is the SpatiaLite library

The Initial Developer of the Original Code is Alessandro Furieri
 
Portions created by the Initial Developer are Copyright (C) 2016
the Initial Developer. All Rights Reserved.

Contributor(s):
Luigi Costalli luigi.costalli@gmail.com

Alternatively, the contents of this file may be used under the terms of
either the GNU General Public License Version 2 or later (the "GPL"), or
the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
in which case the provisions of the GPL or the LGPL are applicable instead
of those above. If you wish to allow use of your version of this file only
under the terms of either the GPL or the LGPL, and not to allow others to
use your version of this file under the terms of the MPL, indicate your
decision by deleting the provisions above and replace them with the notice
and other provisions required by the GPL or the LGPL. If you do not delete
the provisions above, a recipient may use your version of this file under
the terms of any one of the MPL, the GPL or the LGPL.
 
*/

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <ctype.h>

#if defined(_WIN32) && !defined(__MINGW32__)
#include "config-msvc.h"
#else
#include "config.h"
#endif

#include <spatialite/sqlite.h>

#include <spatialite/spatialite.h>
#include <spatialite/gaiaaux.h>
#include <spatialite/gaiageo.h>

static struct sqlite3_module my_route_module;

#define VROUTE_DIJKSTRA_ALGORITHM	1
#define VROUTE_A_STAR_ALGORITHM	2

#define VROUTE_ROUTING_SOLUTION	0xdd
#define VROUTE_RANGE_SOLUTION	0xbb
#define VROUTE_TSP_SOLUTION		0xee

#define VROUTE_SHORTEST_PATH_FULL		0x70
#define VROUTE_SHORTEST_PATH_NO_ARCS	0x71
#define VROUTE_SHORTEST_PATH_NO_GEOMS	0x72
#define VROUTE_SHORTEST_PATH_SIMPLE		0x73

#define VROUTE_SHORTEST_PATH			0x91
#define VROUTE_TSP_NN					0x92
#define VROUTE_TSP_GA					0x93

#define VROUTE_INVALID_SRID	-1234

#define	VROUTE_TSP_GA_MAX_ITERATIONS	512

#ifdef _WIN32
#define strcasecmp	_stricmp
#endif /* not WIN32 */

/******************************************************************************
/
/ helper structs for destintation candidates
/
******************************************************************************/

typedef struct DestinationCandidateStruct
{
/* a candidate destination */
    char *Code;
    sqlite3_int64 Id;
    char Valid;
    struct DestinationCandidateStruct *Next;
} DestinationCandidate;
typedef DestinationCandidate *DestinationCandidatePtr;

typedef struct DestinationCandidatesListStruct
{
/* a list of candidate destination */
    int NodeCode;
    DestinationCandidatePtr First;
    DestinationCandidatePtr Last;
    int ValidItems;
} DestinationCandidatesList;
typedef DestinationCandidatesList *DestinationCandidatesListPtr;

/******************************************************************************
/
/ virtualrouting structs
/
******************************************************************************/

typedef struct RouteArcStruct
{
/* an ARC */
    const struct RouteNodeStruct *NodeFrom;
    const struct RouteNodeStruct *NodeTo;
    sqlite3_int64 ArcRowid;
    double Cost;
} RouteArc;
typedef RouteArc *RouteArcPtr;

typedef struct RouteNodeStruct
{
/* a NODE */
    int InternalIndex;
    sqlite3_int64 Id;
    char *Code;
    double CoordX;
    double CoordY;
    int NumArcs;
    RouteArcPtr Arcs;
} RouteNode;
typedef RouteNode *RouteNodePtr;

typedef struct RoutingStruct
{
/* the main NETWORK structure */
    int Net64;
    int AStar;
    int EndianArch;
    int MaxCodeLength;
    int CurrentIndex;
    int NodeCode;
    int NumNodes;
    char *TableName;
    char *FromColumn;
    char *ToColumn;
    char *GeometryColumn;
    char *NameColumn;
    double AStarHeuristicCoeff;
    RouteNodePtr Nodes;
} Routing;
typedef Routing *RoutingPtr;

typedef struct ArcSolutionStruct
{
/* Geometry corresponding to an Arc used by Dijkstra shortest path solution */
    sqlite3_int64 ArcRowid;
    char *FromCode;
    char *ToCode;
    sqlite3_int64 FromId;
    sqlite3_int64 ToId;
    int Points;
    double *Coords;
    int Srid;
    char *Name;
    struct ArcSolutionStruct *Next;

} ArcSolution;
typedef ArcSolution *ArcSolutionPtr;

typedef struct RowSolutionStruct
{
/* a row into the shortest path solution */
    RouteArcPtr Arc;
    char *Name;
    struct RowSolutionStruct *Next;

} RowSolution;
typedef RowSolution *RowSolutionPtr;

typedef struct RowNodeSolutionStruct
{
/* a row into the "within Cost range" solution */
    int RouteNum;
    int RouteRow;
    RouteNodePtr Node;
    double Cost;
    int Srid;
    struct RowNodeSolutionStruct *Next;

} RowNodeSolution;
typedef RowNodeSolution *RowNodeSolutionPtr;

typedef struct ShortestPathSolutionStruct
{
/* a shortest path solution */
    ArcSolutionPtr FirstArc;
    ArcSolutionPtr LastArc;
    RouteNodePtr From;
    RouteNodePtr To;
    char *Undefined;
    RowSolutionPtr First;
    RowSolutionPtr Last;
    RowNodeSolutionPtr FirstNode;
    RowNodeSolutionPtr LastNode;
    RowNodeSolutionPtr CurrentNodeRow;
    double TotalCost;
    gaiaGeomCollPtr Geometry;
    struct ShortestPathSolutionStruct *Next;
} ShortestPathSolution;
typedef ShortestPathSolution *ShortestPathSolutionPtr;

typedef struct ResultsetRowStruct
{
/* a row into the resultset */
    int RouteNum;
    int RouteRow;
    RouteNodePtr From;
    RouteNodePtr To;
    char *Undefined;
    RowSolutionPtr linkRef;
    double TotalCost;
    gaiaGeomCollPtr Geometry;
    struct ResultsetRowStruct *Next;

} ResultsetRow;
typedef ResultsetRow *ResultsetRowPtr;

typedef struct RoutingMultiDestStruct
{
/* helper struct supporting multiple destinations */
    int CodeNode;
    int Items;
    int Next;
    RouteNodePtr *To;
    char *Found;
    sqlite3_int64 *Ids;
    char **Codes;
} RoutingMultiDest;
typedef RoutingMultiDest *RoutingMultiDestPtr;

typedef struct MultiSolutionStruct
{
/* multiple shortest path solutions */
    unsigned char Mode;
    RouteNodePtr From;
    double MaxCost;
    RoutingMultiDestPtr MultiTo;
    ResultsetRowPtr FirstRow;
    ResultsetRowPtr LastRow;
    ResultsetRowPtr CurrentRow;
    ShortestPathSolutionPtr First;
    ShortestPathSolutionPtr Last;
    RowNodeSolutionPtr FirstNode;
    RowNodeSolutionPtr LastNode;
    RowSolutionPtr FirstArc;
    RowSolutionPtr LastArc;
    gaiaGeomCollPtr FirstGeom;
    gaiaGeomCollPtr LastGeom;
    RowNodeSolutionPtr CurrentNodeRow;
    sqlite3_int64 CurrentRowId;
} MultiSolution;
typedef MultiSolution *MultiSolutionPtr;

/******************************************************************************
/
/ TSP structs
/
******************************************************************************/

typedef struct TspTargetsStruct
{
/* TSP helper struct */
    unsigned char Mode;
    double TotalCost;
    RouteNodePtr From;
    int Count;
    RouteNodePtr *To;
    char *Found;
    double *Costs;
    ShortestPathSolutionPtr *Solutions;
    ShortestPathSolutionPtr LastSolution;
} TspTargets;
typedef TspTargets *TspTargetsPtr;

typedef struct TspGaSubDistanceStruct
{
/* a cache sub-item for storing TSP GA costs */
    RouteNodePtr CityTo;
    double Cost;
} TspGaSubDistance;
typedef TspGaSubDistance *TspGaSubDistancePtr;

typedef struct TspGaDistanceStruct
{
/* a cache item for storing TSP GA costs */
    RouteNodePtr CityFrom;
    int Cities;
    TspGaSubDistancePtr *Distances;
    int NearestIndex;
} TspGaDistance;
typedef TspGaDistance *TspGaDistancePtr;

typedef struct TspGaSolutionStruct
{
/* TSP GA solution struct */
    int Cities;
    RouteNodePtr *CitiesFrom;
    RouteNodePtr *CitiesTo;
    double *Costs;
    double TotalCost;
} TspGaSolution;
typedef TspGaSolution *TspGaSolutionPtr;

typedef struct TspGaPopulationStruct
{
/* TSP GA helper struct */
    int Count;
    int Cities;
    TspGaSolutionPtr *Solutions;
    TspGaSolutionPtr *Offsprings;
    TspGaDistancePtr *Distances;
    char *RandomSolutionsSql;
    char *RandomIntervalSql;
} TspGaPopulation;
typedef TspGaPopulation *TspGaPopulationPtr;

/******************************************************************************
/
/ Dijkstra and A* common structs
/
******************************************************************************/

typedef struct RoutingNode
{
    int Id;
    struct RoutingNode **To;
    RouteArcPtr *Link;
    int DimTo;
    struct RoutingNode *PreviousNode;
    RouteNodePtr Node;
    RouteArcPtr Arc;
    double Distance;
    double HeuristicDistance;
    int Inspected;
} RoutingNode;
typedef RoutingNode *RoutingNodePtr;

typedef struct RoutingNodes
{
    RoutingNodePtr Nodes;
    RouteArcPtr *ArcsBuffer;
    RoutingNodePtr *NodesBuffer;
    int Dim;
    int DimLink;
} RoutingNodes;
typedef RoutingNodes *RoutingNodesPtr;

typedef struct HeapNode
{
    RoutingNodePtr Node;
    double Distance;
} HeapNode;
typedef HeapNode *HeapNodePtr;

typedef struct RoutingHeapStruct
{
    HeapNodePtr Nodes;
    int Count;
} RoutingHeap;
typedef RoutingHeap *RoutingHeapPtr;

/******************************************************************************
/
/ VirtualTable structs
/
******************************************************************************/

typedef struct virtualroutingStruct
{
/* extends the sqlite3_vtab struct */
    const sqlite3_module *pModule;	/* ptr to sqlite module: USED INTERNALLY BY SQLITE */
    int nRef;			/* # references: USED INTERNALLY BY SQLITE */
    char *zErrMsg;		/* error message: USE INTERNALLY BY SQLITE */
    sqlite3 *db;		/* the sqlite db holding the virtual table */
    RoutingPtr graph;		/* the NETWORK structure */
    RoutingNodesPtr routing;	/* the ROUTING structure */
    int currentAlgorithm;	/* the currently selected Shortest Path Algorithm */
    int currentRequest;		/* the currently selected Shortest Path Request */
    int currentOptions;		/* the currently selected Shortest Path Options */
    char currentDelimiter;	/* the currently set delimiter char */
    MultiSolutionPtr multiSolution;	/* the current multiple solution */
    int eof;			/* the EOF marker */
} virtualrouting;
typedef virtualrouting *virtualroutingPtr;

typedef struct virtualroutingCursortStruct
{
/* extends the sqlite3_vtab_cursor struct */
    virtualroutingPtr pVtab;	/* Virtual table of this cursor */
} virtualroutingCursor;
typedef virtualroutingCursor *virtualroutingCursorPtr;

/*
/
/  implementation of the Dijkstra Shortest Path algorithm
/
////////////////////////////////////////////////////////////
/
/ Author: Luigi Costalli luigi.costalli@gmail.com
/ version 1.0. 2008 October 21
/
*/

static RoutingNodesPtr
routing_init (RoutingPtr graph)
{
/* allocating and initializing the ROUTING struct */
    int i;
    int j;
    int cnt = 0;
    RoutingNodesPtr nd;
    RoutingNodePtr ndn;
    RouteNodePtr nn;
/* allocating the main Nodes struct */
    nd = malloc (sizeof (RoutingNodes));
/* allocating and initializing  Nodes array */
    nd->Nodes = malloc (sizeof (RoutingNode) * graph->NumNodes);
    nd->Dim = graph->NumNodes;
    nd->DimLink = 0;

/* pre-alloc buffer strategy - GENSCHER 2010-01-05 */
    for (i = 0; i < graph->NumNodes; cnt += graph->Nodes[i].NumArcs, i++);
    nd->NodesBuffer = malloc (sizeof (RoutingNodePtr) * cnt);
    nd->ArcsBuffer = malloc (sizeof (RouteArcPtr) * cnt);

    cnt = 0;
    for (i = 0; i < graph->NumNodes; i++)
      {
	  /* initializing the Nodes array */
	  nn = graph->Nodes + i;
	  ndn = nd->Nodes + i;
	  ndn->Id = nn->InternalIndex;
	  ndn->DimTo = nn->NumArcs;
	  ndn->Node = nn;
	  ndn->To = &(nd->NodesBuffer[cnt]);
	  ndn->Link = &(nd->ArcsBuffer[cnt]);
	  cnt += nn->NumArcs;

	  for (j = 0; j < nn->NumArcs; j++)
	    {
		/*  setting the outcoming Arcs for the current Node */
		nd->DimLink++;
		ndn->To[j] = nd->Nodes + nn->Arcs[j].NodeTo->InternalIndex;
		ndn->Link[j] = nn->Arcs + j;
	    }
      }
    return (nd);
}

static void
routing_free (RoutingNodes * e)
{
/* memory cleanup; freeing the ROUTING struct */
    free (e->ArcsBuffer);
    free (e->NodesBuffer);
    free (e->Nodes);
    free (e);
}

static RoutingHeapPtr
routing_heap_init (int n)
{
/* allocating and initializing the Heap (min-priority queue) */
    RoutingHeapPtr heap = malloc (sizeof (RoutingHeap));
    heap->Count = 0;
    heap->Nodes = malloc (sizeof (HeapNode) * (n + 1));
    return heap;
}

static void
routing_heap_reset (RoutingHeapPtr heap)
{
/* resetting the Heap (min-priority queue) */
    if (heap == NULL)
	return;
    heap->Count = 0;
}

static void
routing_heap_free (RoutingHeapPtr heap)
{
/* freeing the Heap (min-priority queue) */
    if (heap->Nodes != NULL)
	free (heap->Nodes);
    free (heap);
}

static void
dijkstra_insert (RoutingNodePtr node, HeapNodePtr heap, int size)
{
/* inserting a new Node and rearranging the heap */
    int i;
    HeapNode tmp;
    i = size + 1;
    heap[i].Node = node;
    heap[i].Distance = node->Distance;
    if (i / 2 < 1)
	return;
    while (heap[i].Distance < heap[i / 2].Distance)
      {
	  tmp = heap[i];
	  heap[i] = heap[i / 2];
	  heap[i / 2] = tmp;
	  i /= 2;
	  if (i / 2 < 1)
	      break;
      }
}

static void
dijkstra_enqueue (RoutingHeapPtr heap, RoutingNodePtr node)
{
/* enqueuing a Node into the heap */
    dijkstra_insert (node, heap->Nodes, heap->Count);
    heap->Count += 1;
}

static void
dijkstra_shiftdown (HeapNodePtr heap, int size, int i)
{
/* rearranging the heap after removing */
    int c;
    HeapNode tmp;
    for (;;)
      {
	  c = i * 2;
	  if (c > size)
	      break;
	  if (c < size)
	    {
		if (heap[c].Distance > heap[c + 1].Distance)
		    ++c;
	    }
	  if (heap[c].Distance < heap[i].Distance)
	    {
		/* swapping two Nodes */
		tmp = heap[c];
		heap[c] = heap[i];
		heap[i] = tmp;
		i = c;
	    }
	  else
	      break;
      }
}

static RoutingNodePtr
dijkstra_remove_min (HeapNodePtr heap, int size)
{
/* removing the min-priority Node from the heap */
    RoutingNodePtr node = heap[1].Node;
    heap[1] = heap[size];
    --size;
    dijkstra_shiftdown (heap, size, 1);
    return node;
}

static RoutingNodePtr
routing_dequeue (RoutingHeapPtr heap)
{
/* dequeuing a Node from the heap */
    RoutingNodePtr node = dijkstra_remove_min (heap->Nodes, heap->Count);
    heap->Count -= 1;
    return node;
}

/* END of Luigi Costalli Dijkstra Shortest Path implementation */

static void
delete_solution (ShortestPathSolutionPtr solution)
{
/* deleting the current solution */
    ArcSolutionPtr pA;
    ArcSolutionPtr pAn;
    RowSolutionPtr pR;
    RowSolutionPtr pRn;
    RowNodeSolutionPtr pN;
    RowNodeSolutionPtr pNn;
    if (!solution)
	return;
    pA = solution->FirstArc;
    while (pA)
      {
	  pAn = pA->Next;
	  if (pA->FromCode)
	      free (pA->FromCode);
	  if (pA->ToCode)
	      free (pA->ToCode);
	  if (pA->Coords)
	      free (pA->Coords);
	  if (pA->Name)
	      free (pA->Name);
	  free (pA);
	  pA = pAn;
      }
    pR = solution->First;
    while (pR)
      {
	  pRn = pR->Next;
	  if (pR->Name)
	      free (pR->Name);
	  free (pR);
	  pR = pRn;
      }
    pN = solution->FirstNode;
    while (pN)
      {
	  pNn = pN->Next;
	  free (pN);
	  pN = pNn;
      }
    if (solution->Undefined != NULL)
	free (solution->Undefined);
    if (solution->Geometry)
	gaiaFreeGeomColl (solution->Geometry);
    free (solution);
}

static ShortestPathSolutionPtr
alloc_solution (void)
{
/* allocates and initializes the current solution */
    ShortestPathSolutionPtr p = malloc (sizeof (ShortestPathSolution));
    p->FirstArc = NULL;
    p->LastArc = NULL;
    p->From = NULL;
    p->To = NULL;
    p->Undefined = NULL;
    p->First = NULL;
    p->Last = NULL;
    p->FirstNode = NULL;
    p->LastNode = NULL;
    p->CurrentNodeRow = NULL;
    p->TotalCost = 0.0;
    p->Geometry = NULL;
    p->Next = NULL;
    return p;
}

static void
add_arc_to_solution (ShortestPathSolutionPtr solution, RouteArcPtr arc)
{
/* inserts an Arc into the Shortest Path solution */
    RowSolutionPtr p = malloc (sizeof (RowSolution));
    p->Arc = arc;
    p->Name = NULL;
    p->Next = NULL;
    solution->TotalCost += arc->Cost;
    if (!(solution->First))
	solution->First = p;
    if (solution->Last)
	solution->Last->Next = p;
    solution->Last = p;
}

static void
add_node_to_solution (MultiSolutionPtr multiSolution, RoutingNodePtr node,
		      int srid, int index)
{
/* inserts a Node into the "within Cost range" solution */
    RowNodeSolutionPtr p = malloc (sizeof (RowNodeSolution));
    p->RouteNum = 0;
    p->RouteRow = index;
    p->Node = node->Node;
    p->Cost = node->Distance;
    p->Srid = srid;
    p->Next = NULL;
    if (!(multiSolution->FirstNode))
	multiSolution->FirstNode = p;
    if (multiSolution->LastNode)
	multiSolution->LastNode->Next = p;
    multiSolution->LastNode = p;
}

static void
set_arc_name_into_solution (ShortestPathSolutionPtr solution,
			    sqlite3_int64 arc_id, const char *name)
{
/* sets the Name identifyin an Arc into the Solution */
    RowSolutionPtr row = solution->First;
    while (row != NULL)
      {
	  if (row->Arc->ArcRowid == arc_id)
	    {
		int len = strlen (name);
		if (row->Name != NULL)
		    free (row->Name);
		row->Name = malloc (len + 1);
		strcpy (row->Name, name);
		return;
	    }
	  row = row->Next;
      }
}

static void
add_arc_geometry_to_solution (ShortestPathSolutionPtr solution,
			      sqlite3_int64 arc_id, const char *from_code,
			      const char *to_code, sqlite3_int64 from_id,
			      sqlite3_int64 to_id, int points, double *coords,
			      int srid, const char *name)
{
/* inserts an Arc Geometry into the Shortest Path solution */
    int len;
    ArcSolutionPtr p = malloc (sizeof (ArcSolution));
    p->ArcRowid = arc_id;
    p->FromCode = NULL;
    len = strlen (from_code);
    if (len > 0)
      {
	  p->FromCode = malloc (len + 1);
	  strcpy (p->FromCode, from_code);
      }
    p->ToCode = NULL;
    len = strlen (to_code);
    if (len > 0)
      {
	  p->ToCode = malloc (len + 1);
	  strcpy (p->ToCode, to_code);
      }
    p->FromId = from_id;
    p->ToId = to_id;
    p->Points = points;
    p->Coords = coords;
    p->Srid = srid;
    if (name == NULL)
	p->Name = NULL;
    else
      {
	  len = strlen (name);
	  p->Name = malloc (len + 1);
	  strcpy (p->Name, name);
      }
    p->Next = NULL;
    if (!(solution->FirstArc))
	solution->FirstArc = p;
    if (solution->LastArc)
	solution->LastArc->Next = p;
    solution->LastArc = p;
}

static void
build_solution (sqlite3 * handle, int options, RoutingPtr graph,
		ShortestPathSolutionPtr solution, RouteArcPtr * shortest_path,
		int cnt)
{
/* formatting the Shortest Path solution */
    int i;
    char *sql;
    int err;
    int error = 0;
    int ret;
    sqlite3_int64 arc_id;
    const unsigned char *blob;
    int size;
    sqlite3_int64 from_id;
    sqlite3_int64 to_id;
    char *from_code;
    char *to_code;
    char *name;
    int tbd;
    int ind;
    int base = 0;
    int block = 128;
    int how_many;
    sqlite3_stmt *stmt = NULL;
    char *xfrom;
    char *xto;
    char *xgeom;
    char *xname;
    char *xtable;
    gaiaOutBuffer sql_statement;
    if (options == VROUTE_SHORTEST_PATH_SIMPLE)
      {
	  /* building the solution */
	  for (i = 0; i < cnt; i++)
	      solution->TotalCost += shortest_path[i]->Cost;
      }
    else
      {
	  /* building the solution */
	  for (i = 0; i < cnt; i++)
	      add_arc_to_solution (solution, shortest_path[i]);
      }
    if (options == VROUTE_SHORTEST_PATH_SIMPLE)
      {
	  if (shortest_path)
	      free (shortest_path);
	  return;
      }
    if (graph->GeometryColumn == NULL && graph->NameColumn == NULL)
      {
	  if (shortest_path)
	      free (shortest_path);
	  return;
      }
    if (options == VROUTE_SHORTEST_PATH_NO_GEOMS && graph->NameColumn == NULL)
      {
	  if (shortest_path)
	      free (shortest_path);
	  return;
      }

    tbd = cnt;
    while (tbd > 0)
      {
	  /* requesting max 128 arcs at each time */
	  if (tbd < block)
	      how_many = tbd;
	  else
	      how_many = block;
/* preparing the Geometry representing this solution [reading arcs] */
	  gaiaOutBufferInitialize (&sql_statement);
	  if (graph->NameColumn)
	    {
		/* a Name column is defined */
		if (graph->GeometryColumn == NULL
		    || options == VROUTE_SHORTEST_PATH_NO_GEOMS)
		  {
		      xfrom = gaiaDoubleQuotedSql (graph->FromColumn);
		      xto = gaiaDoubleQuotedSql (graph->ToColumn);
		      xname = gaiaDoubleQuotedSql (graph->NameColumn);
		      xtable = gaiaDoubleQuotedSql (graph->TableName);
		      sql =
			  sqlite3_mprintf
			  ("SELECT ROWID, \"%s\", \"%s\", NULL, \"%s\" FROM \"%s\" WHERE ROWID IN (",
			   xfrom, xto, xname, xtable);
		      free (xfrom);
		      free (xto);
		      free (xname);
		      free (xtable);
		  }
		else
		  {
		      xfrom = gaiaDoubleQuotedSql (graph->FromColumn);
		      xto = gaiaDoubleQuotedSql (graph->ToColumn);
		      xgeom = gaiaDoubleQuotedSql (graph->GeometryColumn);
		      xname = gaiaDoubleQuotedSql (graph->NameColumn);
		      xtable = gaiaDoubleQuotedSql (graph->TableName);
		      sql =
			  sqlite3_mprintf
			  ("SELECT ROWID, \"%s\", \"%s\", \"%s\", \"%s\" FROM \"%s\" WHERE ROWID IN (",
			   xfrom, xto, xgeom, xname, xtable);
		      free (xfrom);
		      free (xto);
		      free (xgeom);
		      free (xname);
		      free (xtable);
		  }
		gaiaAppendToOutBuffer (&sql_statement, sql);
		sqlite3_free (sql);
	    }
	  else
	    {
		/* no Name column is defined */
		xfrom = gaiaDoubleQuotedSql (graph->FromColumn);
		xto = gaiaDoubleQuotedSql (graph->ToColumn);
		xgeom = gaiaDoubleQuotedSql (graph->GeometryColumn);
		xtable = gaiaDoubleQuotedSql (graph->TableName);
		sql =
		    sqlite3_mprintf
		    ("SELECT ROWID, \"%s\", \"%s\", \"%s\" FROM \"%s\" WHERE ROWID IN (",
		     xfrom, xto, xgeom, xtable);
		free (xfrom);
		free (xto);
		free (xgeom);
		free (xtable);
		gaiaAppendToOutBuffer (&sql_statement, sql);
		sqlite3_free (sql);
	    }
	  for (i = 0; i < how_many; i++)
	    {
		if (i == 0)
		    gaiaAppendToOutBuffer (&sql_statement, "?");
		else
		    gaiaAppendToOutBuffer (&sql_statement, ",?");
	    }
	  gaiaAppendToOutBuffer (&sql_statement, ")");
	  if (sql_statement.Error == 0 && sql_statement.Buffer != NULL)
	      ret =
		  sqlite3_prepare_v2 (handle, sql_statement.Buffer,
				      strlen (sql_statement.Buffer), &stmt,
				      NULL);
	  else
	      ret = SQLITE_ERROR;
	  gaiaOutBufferReset (&sql_statement);
	  if (ret != SQLITE_OK)
	    {
		error = 1;
		goto abort;
	    }
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  ind = 1;
	  for (i = 0; i < cnt; i++)
	    {
		if (i < base)
		    continue;
		if (i >= (base + how_many))
		    break;
		sqlite3_bind_int64 (stmt, ind, shortest_path[i]->ArcRowid);
		ind++;
	    }
	  while (1)
	    {
		ret = sqlite3_step (stmt);
		if (ret == SQLITE_DONE)
		    break;
		if (ret == SQLITE_ROW)
		  {
		      arc_id = -1;
		      from_id = -1;
		      to_id = -1;
		      from_code = NULL;
		      to_code = NULL;
		      blob = NULL;
		      size = 0;
		      name = NULL;
		      err = 0;
		      if (sqlite3_column_type (stmt, 0) == SQLITE_INTEGER)
			  arc_id = sqlite3_column_int64 (stmt, 0);
		      else
			  err = 1;
		      if (graph->NodeCode)
			{
			    /* nodes are identified by TEXT codes */
			    if (sqlite3_column_type (stmt, 1) == SQLITE_TEXT)
				from_code =
				    (char *) sqlite3_column_text (stmt, 1);
			    else
				err = 1;
			    if (sqlite3_column_type (stmt, 2) == SQLITE_TEXT)
				to_code =
				    (char *) sqlite3_column_text (stmt, 2);
			    else
				err = 1;
			}
		      else
			{
			    /* nodes are identified by INTEGER ids */
			    if (sqlite3_column_type (stmt, 1) == SQLITE_INTEGER)
				from_id = sqlite3_column_int64 (stmt, 1);
			    else
				err = 1;
			    if (sqlite3_column_type (stmt, 2) == SQLITE_INTEGER)
				to_id = sqlite3_column_int64 (stmt, 2);
			    else
				err = 1;
			}
		      if (graph->GeometryColumn != NULL
			  && options != VROUTE_SHORTEST_PATH_NO_GEOMS)
			{
			    if (sqlite3_column_type (stmt, 3) == SQLITE_BLOB)
			      {
				  blob =
				      (const unsigned char *)
				      sqlite3_column_blob (stmt, 3);
				  size = sqlite3_column_bytes (stmt, 3);
			      }
			    else
				err = 1;
			}
		      if (graph->NameColumn)
			{
			    if (sqlite3_column_type (stmt, 4) == SQLITE_TEXT)
				name = (char *) sqlite3_column_text (stmt, 4);
			}
		      if (err)
			  error = 1;
		      else if (graph->GeometryColumn != NULL
			       && options != VROUTE_SHORTEST_PATH_NO_GEOMS)
			{
			    /* saving the Arc geometry into the temporary struct */
			    gaiaGeomCollPtr geom =
				gaiaFromSpatiaLiteBlobWkb (blob, size);
			    if (geom)
			      {
				  /* OK, we have fetched a valid Geometry */
				  if (geom->FirstPoint == NULL
				      && geom->FirstPolygon == NULL
				      && geom->FirstLinestring != NULL
				      && geom->FirstLinestring ==
				      geom->LastLinestring)
				    {
					/* Geometry is LINESTRING as expected */
					int iv;
					int points =
					    geom->FirstLinestring->Points;
					double *coords =
					    malloc (sizeof (double) *
						    (points * 2));
					for (iv = 0; iv < points; iv++)
					  {
					      double x;
					      double y;
					      gaiaGetPoint
						  (geom->FirstLinestring->
						   Coords, iv, &x, &y);
					      *(coords + ((iv * 2) + 0)) = x;
					      *(coords + ((iv * 2) + 1)) = y;
					  }
					if (from_code == NULL)
					    from_code = "";
					if (to_code == NULL)
					    to_code = "";
					add_arc_geometry_to_solution (solution,
								      arc_id,
								      from_code,
								      to_code,
								      from_id,
								      to_id,
								      points,
								      coords,
								      geom->
								      Srid,
								      name);
				    }
				  else
				      error = 1;
				  gaiaFreeGeomColl (geom);
			      }
			    else
				error = 1;
			}
		      else if (name != NULL)
			  set_arc_name_into_solution (solution, arc_id, name);
		  }
	    }
	  sqlite3_finalize (stmt);
	  tbd -= how_many;
	  base += how_many;
      }
  abort:
    if (shortest_path)
	free (shortest_path);
    if (!error && graph->GeometryColumn != NULL
	&& options != VROUTE_SHORTEST_PATH_NO_GEOMS)
      {
	  /* building the Geometry representing the Shortest Path Solution */
	  gaiaLinestringPtr ln;
	  int tot_pts = 0;
	  RowSolutionPtr pR;
	  ArcSolutionPtr pA;
	  int srid = -1;
	  if (solution->FirstArc)
	      srid = (solution->FirstArc)->Srid;
	  pR = solution->First;
	  while (pR)
	    {
		pA = solution->FirstArc;
		while (pA)
		  {
		      /* computing how many vertices do we need to build the LINESTRING */
		      if (pR->Arc->ArcRowid == pA->ArcRowid)
			{
			    if (pR == solution->First)
				tot_pts += pA->Points;
			    else
				tot_pts += (pA->Points - 1);
			    if (pA->Srid != srid)
				srid = -1;
			}
		      pA = pA->Next;
		  }
		pR = pR->Next;
	    }
	  /* creating the Shortest Path Geometry - LINESTRING */
	  if (tot_pts <= 2)
	      return;
	  ln = gaiaAllocLinestring (tot_pts);
	  solution->Geometry = gaiaAllocGeomColl ();
	  solution->Geometry->Srid = srid;
	  gaiaInsertLinestringInGeomColl (solution->Geometry, ln);
	  tot_pts = 0;
	  pR = solution->First;
	  while (pR)
	    {
		/* building the LINESTRING */
		int skip;
		if (pR == solution->First)
		    skip = 0;	/* for first arc we must copy any vertex */
		else
		    skip = 1;	/* for subsequent arcs we must skip first vertex [already inserted from previous arc] */
		pA = solution->FirstArc;
		while (pA)
		  {
		      if (pR->Arc->ArcRowid == pA->ArcRowid)
			{
			    /* copying vertices from correspoinding Arc Geometry */
			    int ini;
			    int iv;
			    int rev;
			    double x;
			    double y;
			    if (graph->NodeCode)
			      {
				  /* nodes are identified by TEXT codes */
				  if (strcmp
				      (pR->Arc->NodeFrom->Code,
				       pA->ToCode) == 0)
				      rev = 1;
				  else
				      rev = 0;
			      }
			    else
			      {
				  /* nodes are identified by INTEGER ids */
				  if (pR->Arc->NodeFrom->Id == pA->ToId)
				      rev = 1;
				  else
				      rev = 0;
			      }
			    if (rev)
			      {
				  /* copying Arc vertices in reverse order */
				  if (skip)
				      ini = pA->Points - 2;
				  else
				      ini = pA->Points - 1;
				  for (iv = ini; iv >= 0; iv--)
				    {
					x = *(pA->Coords + ((iv * 2) + 0));
					y = *(pA->Coords + ((iv * 2) + 1));
					gaiaSetPoint (ln->Coords, tot_pts, x,
						      y);
					tot_pts++;
				    }
			      }
			    else
			      {
				  /* copying Arc vertices in normal order */
				  if (skip)
				      ini = 1;
				  else
				      ini = 0;
				  for (iv = ini; iv < pA->Points; iv++)
				    {
					x = *(pA->Coords + ((iv * 2) + 0));
					y = *(pA->Coords + ((iv * 2) + 1));
					gaiaSetPoint (ln->Coords, tot_pts, x,
						      y);
					tot_pts++;
				    }
			      }
			    if (pA->Name)
			      {
				  int len = strlen (pA->Name);
				  pR->Name = malloc (len + 1);
				  strcpy (pR->Name, pA->Name);
			      }
			    break;
			}
		      pA = pA->Next;
		  }
		pR = pR->Next;
	    }
      }
    else
      {
	  RowSolutionPtr pR;
	  ArcSolutionPtr pA;
	  if (graph->NameColumn != NULL)
	    {
		pR = solution->First;
		while (pR)
		  {
		      pA = solution->FirstArc;
		      while (pA)
			{
			    if (pA->Name)
			      {
				  int len = strlen (pA->Name);
				  pR->Name = malloc (len + 1);
				  strcpy (pR->Name, pA->Name);
			      }
			    pA = pA->Next;
			}
		      pR = pR->Next;
		  }
	    }
      }

    if (options == VROUTE_SHORTEST_PATH_NO_ARCS)
      {
	  /* deleting arcs */
	  RowSolutionPtr pR;
	  RowSolutionPtr pRn;
	  pR = solution->First;
	  while (pR)
	    {
		pRn = pR->Next;
		if (pR->Name)
		    free (pR->Name);
		free (pR);
		pR = pRn;
	    }
	  solution->First = NULL;
	  solution->Last = NULL;
      }
}

static void
build_multi_solution (MultiSolutionPtr multiSolution)
{
/* formatting the Shortest Path resultset */
    ShortestPathSolutionPtr pS;
    int route_num = 0;
    pS = multiSolution->First;
    while (pS != NULL)
      {
	  /* inserting the Route Header */
	  int route_row = 0;
	  RowSolutionPtr pA;
	  ResultsetRowPtr row = malloc (sizeof (ResultsetRow));
	  row->RouteNum = route_num;
	  row->RouteRow = route_row++;
	  row->From = pS->From;
	  row->To = pS->To;
	  row->Undefined = pS->Undefined;
	  pS->Undefined = NULL;
	  row->linkRef = NULL;
	  row->TotalCost = pS->TotalCost;
	  row->Geometry = pS->Geometry;
	  row->Next = NULL;
	  if (multiSolution->FirstRow == NULL)
	      multiSolution->FirstRow = row;
	  if (multiSolution->LastRow != NULL)
	      multiSolution->LastRow->Next = row;
	  multiSolution->LastRow = row;

	  pA = pS->First;
	  while (pA != NULL)
	    {
		/* inserting Route's traversed Arcs */
		row = malloc (sizeof (ResultsetRow));
		row->RouteNum = route_num;
		row->RouteRow = route_row++;
		row->From = NULL;
		row->To = NULL;
		row->Undefined = NULL;
		row->linkRef = pA;
		row->TotalCost = 0.0;
		row->Geometry = NULL;
		row->Next = NULL;
		if (multiSolution->FirstRow == NULL)
		    multiSolution->FirstRow = row;
		if (multiSolution->LastRow != NULL)
		    multiSolution->LastRow->Next = row;
		multiSolution->LastRow = row;
		pA = pA->Next;
	    }
	  route_num++;
	  pS = pS->Next;
      }
}

static void
aux_tsp_add_solution (MultiSolutionPtr multiSolution,
		      ShortestPathSolutionPtr pS, int *route_num,
		      gaiaLinestringPtr tsp_path, int *point_index, int mode)
{
/* helper function: adding a solutiont into the TSP resultset */
    RowSolutionPtr pA;
    ResultsetRowPtr row;
    int route_row = 0;

/* inserting the Route Header */
    row = malloc (sizeof (ResultsetRow));
    row->RouteNum = *route_num;
    *route_num += 1;
    row->RouteRow = route_row;
    route_row += 1;
    row->From = pS->From;
    row->To = pS->To;
    row->Undefined = NULL;
    row->linkRef = NULL;
    row->TotalCost = pS->TotalCost;
    row->Geometry = pS->Geometry;
    row->Next = NULL;
    if (multiSolution->FirstRow == NULL)
	multiSolution->FirstRow = row;
    if (multiSolution->LastRow != NULL)
	multiSolution->LastRow->Next = row;
    multiSolution->LastRow = row;
    /* adding the Geometry to the multiSolution list */
    if (multiSolution->FirstGeom == NULL)
	multiSolution->FirstGeom = pS->Geometry;
    if (multiSolution->LastGeom != NULL)
	multiSolution->LastGeom->Next = pS->Geometry;
    multiSolution->LastGeom = pS->Geometry;
    /* removing Geometry ownership form Solution */
    pS->Geometry = NULL;

    /* copying points into the TSP geometry */
    if (tsp_path != NULL && row->Geometry != NULL)
      {
	  gaiaLinestringPtr ln = row->Geometry->FirstLinestring;
	  if (ln != NULL)
	    {
		int base;
		int iv;
		if (!mode)
		    base = 0;
		else
		    base = 1;
		for (iv = base; iv < ln->Points; iv++)
		  {
		      double x;
		      double y;
		      int pt = *point_index;
		      *point_index += 1;
		      gaiaGetPoint (ln->Coords, iv, &x, &y);
		      gaiaSetPoint (tsp_path->Coords, pt, x, y);
		  }
	    }
      }

    pA = pS->First;
    while (pA != NULL)
      {
	  /* inserting Route's traversed Arcs */
	  row = malloc (sizeof (ResultsetRow));
	  row->RouteNum = *route_num;
	  row->RouteRow = route_row;
	  route_row += 1;
	  row->From = NULL;
	  row->To = NULL;
	  row->Undefined = NULL;
	  row->linkRef = pA;
	  row->TotalCost = 0.0;
	  row->Geometry = NULL;
	  row->Next = NULL;
	  if (multiSolution->FirstRow == NULL)
	      multiSolution->FirstRow = row;
	  if (multiSolution->LastRow != NULL)
	      multiSolution->LastRow->Next = row;
	  multiSolution->LastRow = row;
	  /* adding the Arc to the multiSolution list */
	  if (multiSolution->FirstArc == NULL)
	      multiSolution->FirstArc = pA;
	  if (multiSolution->LastArc != NULL)
	      multiSolution->LastArc->Next = pA;
	  multiSolution->LastArc = pA;
	  pA = pA->Next;
      }
    route_num += 1;
    /* removing Arcs ownership form Solution */
    pS->First = NULL;
    pS->Last = NULL;
}

static void
aux_build_tsp (MultiSolutionPtr multiSolution, TspTargetsPtr targets,
	       int route_num, gaiaLinestringPtr tsp_path)
{
/* helper function: formatting the TSP resultset */
    int point_index = 0;
    int i;
    for (i = 0; i < targets->Count; i++)
      {
	  /* adding all City to City solutions */
	  ShortestPathSolutionPtr pS = *(targets->Solutions + i);
	  aux_tsp_add_solution (multiSolution, pS, &route_num, tsp_path,
				&point_index, i);
      }
    /* adding the final trip closing the circular path */
    aux_tsp_add_solution (multiSolution, targets->LastSolution, &route_num,
			  tsp_path, &point_index, targets->Count);

}

static void
build_tsp_solution (MultiSolutionPtr multiSolution, TspTargetsPtr targets)
{
/* formatting the TSP resultset */
    ShortestPathSolutionPtr *oldS;
    ShortestPathSolutionPtr pS;
    ResultsetRowPtr row;
    RouteNodePtr from;
    int i;
    int k;
    int unreachable = 0;
    int route_row = 0;
    int route_num = 0;
    int points = 0;
    gaiaLinestringPtr tsp_path = NULL;
    int srid = -1;
    int found;

/* checking for undefined or unreacheable targets */
    for (i = 0; i < targets->Count; i++)
      {
	  RouteNodePtr to = *(targets->To + i);
	  if (to == NULL)
	      continue;
	  if (*(targets->Found + i) != 'Y')
	      unreachable = 1;
      }

/* inserting the TSP Header */
    row = malloc (sizeof (ResultsetRow));
    row->RouteNum = route_num++;
    row->RouteRow = route_row++;
    row->From = multiSolution->From;
    row->To = multiSolution->From;
    row->Undefined = NULL;
    row->linkRef = NULL;
    if (unreachable)
	row->TotalCost = 0.0;
    else
	row->TotalCost = targets->TotalCost;
    row->Geometry = NULL;
    row->Next = NULL;
    if (multiSolution->FirstRow == NULL)
	multiSolution->FirstRow = row;
    if (multiSolution->LastRow != NULL)
	multiSolution->LastRow->Next = row;
    multiSolution->LastRow = row;

    if (unreachable)
      {
	  /* inserting unreacheable targets */
	  for (i = 0; i < targets->Count; i++)
	    {
		RouteNodePtr to = *(targets->To + i);
		if (to == NULL)
		    continue;
		if (*(targets->Found + i) != 'Y')
		  {
		      /* unreacheable target */
		      row = malloc (sizeof (ResultsetRow));
		      row->RouteNum = route_num++;
		      row->RouteRow = 0;
		      row->From = to;
		      row->To = to;
		      row->Undefined = NULL;
		      row->linkRef = NULL;
		      row->TotalCost = 0.0;
		      row->Geometry = NULL;
		      row->Next = NULL;
		      if (multiSolution->FirstRow == NULL)
			  multiSolution->FirstRow = row;
		      if (multiSolution->LastRow != NULL)
			  multiSolution->LastRow->Next = row;
		      multiSolution->LastRow = row;
		  }
	    }
	  return;
      }

/* creating the TSP geometry */
    for (i = 0; i < targets->Count; i++)
      {
	  pS = *(targets->Solutions + i);
	  gaiaGeomCollPtr geom = pS->Geometry;
	  if (geom != NULL)
	    {
		srid = geom->Srid;
		gaiaLinestringPtr ln = geom->FirstLinestring;
		if (ln != NULL)
		  {
		      if (i == 0)
			  points += ln->Points;
		      else
			  points += ln->Points - 1;
		  }
	    }
      }
    if (targets->LastSolution != NULL)
      {
	  gaiaGeomCollPtr geom = targets->LastSolution->Geometry;
	  if (geom != NULL)
	    {
		gaiaLinestringPtr ln = geom->FirstLinestring;
		if (ln != NULL)
		  {
		      points += ln->Points - 1;
		  }
	    }
      }
    if (points >= 2)
      {
	  row->Geometry = gaiaAllocGeomColl ();
	  row->Geometry->Srid = srid;
	  tsp_path = gaiaAddLinestringToGeomColl (row->Geometry, points);
	  /* adding the Geometry to the multiSolution list */
	  if (multiSolution->FirstGeom == NULL)
	      multiSolution->FirstGeom = row->Geometry;
	  if (multiSolution->LastGeom != NULL)
	      multiSolution->LastGeom->Next = row->Geometry;
	  multiSolution->LastGeom = row->Geometry;
      }

    /* reordering the TSP solution */
    oldS = targets->Solutions;
    targets->Solutions =
	malloc (sizeof (ShortestPathSolutionPtr) * targets->Count);
    from = multiSolution->From;
    for (k = 0; k < targets->Count; k++)
      {
	  /* building the right sequence */
	  found = 0;
	  for (i = 0; i < targets->Count; i++)
	    {
		pS = *(oldS + i);
		if (pS->From == from)
		  {
		      *(targets->Solutions + k) = pS;
		      from = pS->To;
		      found = 1;
		      break;
		  }
	    }
	  if (!found)
	    {
		if (targets->LastSolution->From == from)
		  {
		      *(targets->Solutions + k) = targets->LastSolution;
		      from = targets->LastSolution->To;
		  }
	    }
      }
    /* adjusting the last route so to close a circular path */
    for (i = 0; i < targets->Count; i++)
      {
	  pS = *(oldS + i);
	  if (pS->From == from)
	    {
		targets->LastSolution = pS;
		break;
	    }
      }
    free (oldS);
    aux_build_tsp (multiSolution, targets, route_num, tsp_path);
}

static void
build_tsp_illegal_solution (MultiSolutionPtr multiSolution,
			    TspTargetsPtr targets)
{
/* formatting the TSP resultset - undefined targets */
    ResultsetRowPtr row;
    int i;
    int route_num = 0;

/* inserting the TSP Header */
    row = malloc (sizeof (ResultsetRow));
    row->RouteNum = route_num++;
    row->RouteRow = 0;
    row->From = multiSolution->From;
    row->To = multiSolution->From;
    row->Undefined = NULL;
    row->linkRef = NULL;
    row->TotalCost = 0.0;
    row->Geometry = NULL;
    row->Next = NULL;
    if (multiSolution->FirstRow == NULL)
	multiSolution->FirstRow = row;
    if (multiSolution->LastRow != NULL)
	multiSolution->LastRow->Next = row;
    multiSolution->LastRow = row;

    for (i = 0; i < targets->Count; i++)
      {
	  RouteNodePtr to = *(targets->To + i);
	  const char *code = *(multiSolution->MultiTo->Codes + i);
	  if (to == NULL)
	    {
		/* unknown target */
		int len;
		row = malloc (sizeof (ResultsetRow));
		row->RouteNum = route_num++;
		row->RouteRow = 0;
		row->From = to;
		row->To = to;
		len = strlen (code);
		row->Undefined = malloc (len + 1);
		strcpy (row->Undefined, code);
		row->linkRef = NULL;
		row->TotalCost = 0.0;
		row->Geometry = NULL;
		row->Next = NULL;
		if (multiSolution->FirstRow == NULL)
		    multiSolution->FirstRow = row;
		if (multiSolution->LastRow != NULL)
		    multiSolution->LastRow->Next = row;
		multiSolution->LastRow = row;
	    }
	  if (*(targets->Found + i) != 'Y')
	    {
		/* unreachable target */
		row = malloc (sizeof (ResultsetRow));
		row->RouteNum = route_num++;
		row->RouteRow = 0;
		row->From = to;
		row->To = to;
		row->Undefined = NULL;
		row->linkRef = NULL;
		row->TotalCost = 0.0;
		row->Geometry = NULL;
		row->Next = NULL;
		if (multiSolution->FirstRow == NULL)
		    multiSolution->FirstRow = row;
		if (multiSolution->LastRow != NULL)
		    multiSolution->LastRow->Next = row;
		multiSolution->LastRow = row;
	    }
      }
}

static ShortestPathSolutionPtr
add2multiSolution (MultiSolutionPtr multiSolution, RouteNodePtr pfrom,
		   RouteNodePtr pto)
{
/* adding a route solution to a multiple destinations request */
    ShortestPathSolutionPtr solution = alloc_solution ();
    solution->From = pfrom;
    solution->To = pto;
    if (multiSolution->First == NULL)
	multiSolution->First = solution;
    if (multiSolution->Last != NULL)
	multiSolution->Last->Next = solution;
    multiSolution->Last = solution;
    return solution;
}

static ShortestPathSolutionPtr
add2tspLastSolution (TspTargetsPtr targets, RouteNodePtr from, RouteNodePtr to)
{
/* adding the last route solution to a TSP request */
    ShortestPathSolutionPtr solution = alloc_solution ();
    solution->From = from;
    solution->To = to;
    targets->LastSolution = solution;
    return solution;
}

static ShortestPathSolutionPtr
add2tspSolution (TspTargetsPtr targets, RouteNodePtr from, RouteNodePtr to)
{
/* adding a route solution to a TSP request */
    int i;
    ShortestPathSolutionPtr solution = alloc_solution ();
    solution->From = from;
    solution->To = to;
    for (i = 0; i < targets->Count; i++)
      {
	  if (*(targets->To + i) == to)
	    {
		*(targets->Solutions + i) = solution;
		break;
	    }
      }
    return solution;
}

static RouteNodePtr
check_multiTo (RoutingNodePtr node, RoutingMultiDestPtr multiple)
{
/* testing destinations */
    int i;
    for (i = 0; i < multiple->Items; i++)
      {
	  RouteNodePtr to = *(multiple->To + i);
	  if (to == NULL)
	      continue;
	  if (*(multiple->Found + i) == 'Y')
	      continue;
	  if (node->Id == to->InternalIndex)
	    {
		*(multiple->Found + i) = 'Y';
		return to;
	    }
      }
    return NULL;
}

static int
end_multiTo (RoutingMultiDestPtr multiple)
{
/* testing if there are unresolved destinations */
    int i;
    for (i = 0; i < multiple->Items; i++)
      {
	  RouteNodePtr to = *(multiple->To + i);
	  if (to == NULL)
	      continue;
	  if (*(multiple->Found + i) == 'Y')
	      continue;
	  return 0;
      }
    return 1;
}

static void
dijkstra_multi_shortest_path (sqlite3 * handle, int options, RoutingPtr graph,
			      RoutingNodesPtr e, MultiSolutionPtr multiSolution)
{
/* Shortest Path (multiple destinations) - Dijkstra's algorithm */
    int from;
    int i;
    int k;
    ShortestPathSolutionPtr solution;
    RouteNodePtr destination;
    RoutingNodePtr n;
    RoutingNodePtr p_to;
    RouteArcPtr p_link;
    RoutingHeapPtr heap;
/* setting From */
    from = multiSolution->From->InternalIndex;
/* initializing the heap */
    heap = routing_heap_init (e->DimLink);
/* initializing the graph */
    for (i = 0; i < e->Dim; i++)
      {
	  n = e->Nodes + i;
	  n->PreviousNode = NULL;
	  n->Arc = NULL;
	  n->Inspected = 0;
	  n->Distance = DBL_MAX;
      }
/* queuing the From node into the heap */
    e->Nodes[from].Distance = 0.0;
    dijkstra_enqueue (heap, e->Nodes + from);
    while (heap->Count > 0)
      {
	  /* Dijsktra loop */
	  n = routing_dequeue (heap);
	  destination = check_multiTo (n, multiSolution->MultiTo);
	  if (destination != NULL)
	    {
		/* reached one of the multiple destinations */
		RouteArcPtr *result;
		int cnt = 0;
		int to = destination->InternalIndex;
		n = e->Nodes + to;
		while (n->PreviousNode != NULL)
		  {
		      /* counting how many Arcs are into the Shortest Path solution */
		      cnt++;
		      n = n->PreviousNode;
		  }
		/* allocating the solution */
		result = malloc (sizeof (RouteArcPtr) * cnt);
		k = cnt - 1;
		n = e->Nodes + to;
		while (n->PreviousNode != NULL)
		  {
		      /* inserting an Arc into the solution */
		      result[k] = n->Arc;
		      n = n->PreviousNode;
		      k--;
		  }
		solution =
		    add2multiSolution (multiSolution, multiSolution->From,
				       destination);
		build_solution (handle, options, graph, solution, result, cnt);
		/* testing for end (all destinations already reached) */
		if (end_multiTo (multiSolution->MultiTo))
		    break;
	    }
	  n->Inspected = 1;
	  for (i = 0; i < n->DimTo; i++)
	    {
		p_to = *(n->To + i);
		p_link = *(n->Link + i);
		if (p_to->Inspected == 0)
		  {
		      if (p_to->Distance == DBL_MAX)
			{
			    /* queuing a new node into the heap */
			    p_to->Distance = n->Distance + p_link->Cost;
			    p_to->PreviousNode = n;
			    p_to->Arc = p_link;
			    dijkstra_enqueue (heap, p_to);
			}
		      else if (p_to->Distance > n->Distance + p_link->Cost)
			{
			    /* updating an already inserted node */
			    p_to->Distance = n->Distance + p_link->Cost;
			    p_to->PreviousNode = n;
			    p_to->Arc = p_link;
			}
		  }
	    }
      }
    routing_heap_free (heap);
}

static RouteNodePtr
check_targets (RoutingNodePtr node, TspTargetsPtr targets)
{
/* testing targets */
    int i;
    for (i = 0; i < targets->Count; i++)
      {
	  RouteNodePtr to = *(targets->To + i);
	  if (to == NULL)
	      continue;
	  if (*(targets->Found + i) == 'Y')
	      continue;
	  if (node->Id == to->InternalIndex)
	    {
		*(targets->Found + i) = 'Y';
		return to;
	    }
      }
    return NULL;
}

static void
update_targets (TspTargetsPtr targets, RouteNodePtr destination, double cost,
		int *stop)
{
/* tupdating targets */
    int i;
    *stop = 1;
    for (i = 0; i < targets->Count; i++)
      {
	  RouteNodePtr to = *(targets->To + i);
	  if (to == NULL)
	      continue;
	  if (to == destination)
	      *(targets->Costs + i) = cost;
	  if (*(targets->Found + i) == 'Y')
	      continue;
	  *stop = 0;
      }
}


static void
dijkstra_targets_solve (RoutingNodesPtr e, TspTargetsPtr targets)
{
/* Shortest Path (multiple destinations) for TSP GA */
    int from;
    int i;
    RouteNodePtr destination;
    RoutingNodePtr n;
    RoutingNodePtr p_to;
    RouteArcPtr p_link;
    RoutingHeapPtr heap;
/* setting From */
    from = targets->From->InternalIndex;
/* initializing the heap */
    heap = routing_heap_init (e->DimLink);
/* initializing the graph */
    for (i = 0; i < e->Dim; i++)
      {
	  n = e->Nodes + i;
	  n->PreviousNode = NULL;
	  n->Arc = NULL;
	  n->Inspected = 0;
	  n->Distance = DBL_MAX;
      }
/* queuing the From node into the heap */
    e->Nodes[from].Distance = 0.0;
    dijkstra_enqueue (heap, e->Nodes + from);
    while (heap->Count > 0)
      {
	  /* Dijsktra loop */
	  n = routing_dequeue (heap);
	  destination = check_targets (n, targets);
	  if (destination != NULL)
	    {
		/* reached one of the targets */
		int stop = 0;
		double totalCost = 0.0;
		int to = destination->InternalIndex;
		n = e->Nodes + to;
		while (n->PreviousNode != NULL)
		  {
		      /* computing the total Cost */
		      totalCost += n->Arc->Cost;
		      n = n->PreviousNode;
		  }
		/* updating targets */
		update_targets (targets, destination, totalCost, &stop);
		if (stop)
		    break;
	    }
	  n->Inspected = 1;
	  for (i = 0; i < n->DimTo; i++)
	    {
		p_to = *(n->To + i);
		p_link = *(n->Link + i);
		if (p_to->Inspected == 0)
		  {
		      if (p_to->Distance == DBL_MAX)
			{
			    /* queuing a new node into the heap */
			    p_to->Distance = n->Distance + p_link->Cost;
			    p_to->PreviousNode = n;
			    p_to->Arc = p_link;
			    dijkstra_enqueue (heap, p_to);
			}
		      else if (p_to->Distance > n->Distance + p_link->Cost)
			{
			    /* updating an already inserted node */
			    p_to->Distance = n->Distance + p_link->Cost;
			    p_to->PreviousNode = n;
			    p_to->Arc = p_link;
			}
		  }
	    }
      }
    routing_heap_free (heap);
}

static void
destroy_tsp_targets (TspTargetsPtr targets)
{
/* memory cleanup; destroying a TSP helper struct */
    if (targets == NULL)
	return;
    if (targets->To != NULL)
	free (targets->To);
    if (targets->Found != NULL)
	free (targets->Found);
    if (targets->Costs != NULL)
	free (targets->Costs);
    if (targets->Solutions != NULL)
      {
	  int i;
	  for (i = 0; i < targets->Count; i++)
	    {
		ShortestPathSolutionPtr pS = *(targets->Solutions + i);
		if (pS != NULL)
		    delete_solution (pS);
	    }
	  free (targets->Solutions);
      }
    if (targets->LastSolution != NULL)
	delete_solution (targets->LastSolution);
    free (targets);
}

static TspTargetsPtr
randomize_targets (sqlite3 * handle, RoutingPtr graph,
		   MultiSolutionPtr multiSolution)
{
/* creating and initializing the TSP helper struct */
    char *sql;
    char *prev_sql;
    int ret;
    int i;
    int n_rows;
    int n_columns;
    const char *value = NULL;
    char **results;
    int from;
    TspTargetsPtr targets = malloc (sizeof (TspTargets));
    targets->Mode = multiSolution->Mode;
    targets->TotalCost = 0.0;
    targets->Count = multiSolution->MultiTo->Items;
    targets->To = malloc (sizeof (RouteNodePtr *) * targets->Count);
    targets->Found = malloc (sizeof (char) * targets->Count);
    targets->Costs = malloc (sizeof (double) * targets->Count);
    targets->Solutions =
	malloc (sizeof (ShortestPathSolutionPtr) * targets->Count);
    targets->LastSolution = NULL;
    for (i = 0; i < targets->Count; i++)
      {
	  *(targets->To + i) = NULL;
	  *(targets->Found + i) = 'N';
	  *(targets->Costs + i) = DBL_MAX;
	  *(targets->Solutions + i) = NULL;
      }

    sql =
	sqlite3_mprintf ("SELECT %d, Random() AS rnd\n",
			 multiSolution->From->InternalIndex);
    for (i = 0; i < multiSolution->MultiTo->Items; i++)
      {
	  RouteNodePtr p_to = *(multiSolution->MultiTo->To + i);
	  if (p_to == NULL)
	    {
		sqlite3_free (sql);
		goto illegal;
	    }
	  prev_sql = sql;
	  sql =
	      sqlite3_mprintf ("%sUNION\nSELECT %d, Random() AS rnd\n",
			       prev_sql, p_to->InternalIndex);
	  sqlite3_free (prev_sql);
      }
    prev_sql = sql;
    sql = sqlite3_mprintf ("%sORDER BY rnd LIMIT 1", prev_sql);
    sqlite3_free (prev_sql);
    ret = sqlite3_get_table (handle, sql, &results, &n_rows, &n_columns, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	goto illegal;
    if (n_rows >= 1)
      {
	  for (i = 1; i <= n_rows; i++)
	    {
		value = results[(i * n_columns) + 0];
		from = atoi (value);
	    }
      }
    sqlite3_free_table (results);

    if (from == multiSolution->From->InternalIndex)
      {
	  targets->From = multiSolution->From;
	  for (i = 0; i < multiSolution->MultiTo->Items; i++)
	    {
		RouteNodePtr p_to = *(multiSolution->MultiTo->To + i);
		*(targets->To + i) = p_to;
		*(targets->Found + i) = 'N';
		*(targets->Costs + i) = DBL_MAX;
		*(targets->Solutions + i) = NULL;
	    }
      }
    else
      {
	  int j = 0;
	  targets->From = graph->Nodes + from;
	  *(targets->To + j++) = multiSolution->From;
	  for (i = 0; i < multiSolution->MultiTo->Items; i++)
	    {
		RouteNodePtr p_to = *(multiSolution->MultiTo->To + i);
		*(targets->Found + i) = 'N';
		*(targets->Costs + i) = DBL_MAX;
		*(targets->Solutions + i) = NULL;
		if (p_to == targets->From)
		    continue;
		*(targets->To + j++) = p_to;
	    }
      }
    return targets;

  illegal:
    for (i = 0; i < targets->Count; i++)
      {
	  *(targets->To + i) = NULL;
	  *(targets->Found + i) = 'N';
	  *(targets->Costs + i) = DBL_MAX;
	  *(targets->Solutions + i) = NULL;
      }
    for (i = 0; i < multiSolution->MultiTo->Items; i++)
      {
	  RouteNodePtr p_to = *(multiSolution->MultiTo->To + i);
	  if (p_to == NULL)
	      continue;
	  *(targets->To + i) = p_to;
      }
    return targets;
}

static RouteNodePtr
check_TspTo (RoutingNodePtr node, TspTargetsPtr targets)
{
/* testing TSP destinations */
    int i;
    for (i = 0; i < targets->Count; i++)
      {
	  RouteNodePtr to = *(targets->To + i);
	  if (to == NULL)
	      continue;
	  if (*(targets->Found + i) == 'Y')
	      continue;
	  if (node->Id == to->InternalIndex)
	    {
		*(targets->Found + i) = 'Y';
		return to;
	    }
      }
    return NULL;
}

static RouteNodePtr
check_TspFinal (RoutingNodePtr node, TspTargetsPtr targets)
{
/* testing TSP final destination (= FROM) */
    RouteNodePtr from = targets->From;
    if (node->Id == from->InternalIndex)
	return from;
    return NULL;
}

static int
end_TspTo (TspTargetsPtr targets)
{
/* testing if there are unresolved TSP targets */
    int i;
    for (i = 0; i < targets->Count; i++)
      {
	  RouteNodePtr to = *(targets->To + i);
	  if (to == NULL)
	      continue;
	  if (*(targets->Found + i) == 'Y')
	      continue;
	  return 0;
      }
    return 1;
}

static void
dijkstra_tsp_nn (sqlite3 * handle, int options, RoutingPtr graph,
		 RoutingNodesPtr e, TspTargetsPtr targets)
{
/* TSP NN - Dijkstra's algorithm */
    int from;
    int i;
    int k;
    int last_route = 0;
    ShortestPathSolutionPtr solution;
    RouteNodePtr origin;
    RouteNodePtr destination;
    RoutingNodePtr n;
    RoutingNodePtr p_to;
    RouteArcPtr p_link;
    RoutingHeapPtr heap;

/* setting From */
    from = targets->From->InternalIndex;
    origin = targets->From;
/* initializing the heap */
    heap = routing_heap_init (e->DimLink);
/* initializing the graph */
    for (i = 0; i < e->Dim; i++)
      {
	  n = e->Nodes + i;
	  n->PreviousNode = NULL;
	  n->Arc = NULL;
	  n->Inspected = 0;
	  n->Distance = DBL_MAX;
      }
/* queuing the From node into the heap */
    e->Nodes[from].Distance = 0.0;
    dijkstra_enqueue (heap, e->Nodes + from);
    while (heap->Count > 0)
      {
	  /* Dijsktra loop */
	  n = routing_dequeue (heap);
	  if (last_route)
	      destination = check_TspFinal (n, targets);
	  else
	      destination = check_TspTo (n, targets);
	  if (destination != NULL)
	    {
		/* reached one of the target destinations */
		RouteArcPtr *result;
		int cnt = 0;
		int to = destination->InternalIndex;
		n = e->Nodes + to;
		while (n->PreviousNode != NULL)
		  {
		      /* counting how many Arcs are into the Shortest Path solution */
		      cnt++;
		      n = n->PreviousNode;
		  }
		/* allocating the solution */
		result = malloc (sizeof (RouteArcPtr) * cnt);
		k = cnt - 1;
		n = e->Nodes + to;
		while (n->PreviousNode != NULL)
		  {
		      /* inserting an Arc into the solution */
		      result[k] = n->Arc;
		      n = n->PreviousNode;
		      k--;
		  }
		if (last_route)
		    solution =
			add2tspLastSolution (targets, origin, destination);
		else
		    solution = add2tspSolution (targets, origin, destination);
		build_solution (handle, options, graph, solution, result, cnt);
		targets->TotalCost += solution->TotalCost;

		/* testing for end (all destinations already reached) */
		if (last_route)
		    break;
		if (end_TspTo (targets))
		  {
		      /* determining the final route so to close the circular path */
		      last_route = 1;
		  }

		/* restarting from the current target */
		from = to;
		for (i = 0; i < e->Dim; i++)
		  {
		      n = e->Nodes + i;
		      n->PreviousNode = NULL;
		      n->Arc = NULL;
		      n->Inspected = 0;
		      n->Distance = DBL_MAX;
		  }
		e->Nodes[from].Distance = 0.0;
		routing_heap_reset (heap);
		dijkstra_enqueue (heap, e->Nodes + from);
		origin = destination;
		continue;
	    }
	  n->Inspected = 1;
	  for (i = 0; i < n->DimTo; i++)
	    {
		p_to = *(n->To + i);
		p_link = *(n->Link + i);
		if (p_to->Inspected == 0)
		  {
		      if (p_to->Distance == DBL_MAX)
			{
			    /* queuing a new node into the heap */
			    p_to->Distance = n->Distance + p_link->Cost;
			    p_to->PreviousNode = n;
			    p_to->Arc = p_link;
			    dijkstra_enqueue (heap, p_to);
			}
		      else if (p_to->Distance > n->Distance + p_link->Cost)
			{
			    /* updating an already inserted node */
			    p_to->Distance = n->Distance + p_link->Cost;
			    p_to->PreviousNode = n;
			    p_to->Arc = p_link;
			}
		  }
	    }
      }
    routing_heap_free (heap);
}

static RoutingNodePtr *
dijkstra_range_analysis (RoutingNodesPtr e, RouteNodePtr pfrom,
			 double max_cost, int *ll)
{
/* identifying all Nodes within a given Cost range - Dijkstra's algorithm */
    int from;
    int i;
    RoutingNodePtr p_to;
    RoutingNodePtr n;
    RouteArcPtr p_link;
    int cnt;
    RoutingNodePtr *result;
    RoutingHeapPtr heap;
/* setting From */
    from = pfrom->InternalIndex;
/* initializing the heap */
    heap = routing_heap_init (e->DimLink);
/* initializing the graph */
    for (i = 0; i < e->Dim; i++)
      {
	  n = e->Nodes + i;
	  n->PreviousNode = NULL;
	  n->Arc = NULL;
	  n->Inspected = 0;
	  n->Distance = DBL_MAX;
      }
/* queuing the From node into the heap */
    e->Nodes[from].Distance = 0.0;
    dijkstra_enqueue (heap, e->Nodes + from);
    while (heap->Count > 0)
      {
	  /* Dijsktra loop */
	  n = routing_dequeue (heap);
	  n->Inspected = 1;
	  for (i = 0; i < n->DimTo; i++)
	    {
		p_to = *(n->To + i);
		p_link = *(n->Link + i);
		if (p_to->Inspected == 0)
		  {
		      if (p_to->Distance == DBL_MAX)
			{
			    /* queuing a new node into the heap */
			    if (n->Distance + p_link->Cost <= max_cost)
			      {
				  p_to->Distance = n->Distance + p_link->Cost;
				  p_to->PreviousNode = n;
				  p_to->Arc = p_link;
				  dijkstra_enqueue (heap, p_to);
			      }
			}
		      else if (p_to->Distance > n->Distance + p_link->Cost)
			{
			    /* updating an already inserted node */
			    p_to->Distance = n->Distance + p_link->Cost;
			    p_to->PreviousNode = n;
			    p_to->Arc = p_link;
			}
		  }
	    }
      }
    routing_heap_free (heap);
    cnt = 0;
    for (i = 0; i < e->Dim; i++)
      {
	  /* counting how many traversed Nodes */
	  n = e->Nodes + i;
	  if (n->Inspected)
	      cnt++;
      }
/* allocating the solution */
    result = malloc (sizeof (RouteNodePtr) * cnt);
    cnt = 0;
    for (i = 0; i < e->Dim; i++)
      {
	  /* populating the resultset */
	  n = e->Nodes + i;
	  if (n->Inspected)
	      result[cnt++] = n;
      }
    *ll = cnt;
    return (result);
}

/*
/
/  implementation of the A* Shortest Path algorithm
/
*/

static void
astar_insert (RoutingNodePtr node, HeapNodePtr heap, int size)
{
/* inserting a new Node and rearranging the heap */
    int i;
    HeapNode tmp;
    i = size + 1;
    heap[i].Node = node;
    heap[i].Distance = node->HeuristicDistance;
    if (i / 2 < 1)
	return;
    while (heap[i].Distance < heap[i / 2].Distance)
      {
	  tmp = heap[i];
	  heap[i] = heap[i / 2];
	  heap[i / 2] = tmp;
	  i /= 2;
	  if (i / 2 < 1)
	      break;
      }
}

static void
astar_enqueue (RoutingHeapPtr heap, RoutingNodePtr node)
{
/* enqueuing a Node into the heap */
    astar_insert (node, heap->Nodes, heap->Count);
    heap->Count += 1;
}

static double
astar_heuristic_distance (RouteNodePtr n1, RouteNodePtr n2, double coeff)
{
/* computing the euclidean distance intercurring between two nodes */
    double dx = n1->CoordX - n2->CoordX;
    double dy = n1->CoordY - n2->CoordY;
    double dist = sqrt ((dx * dx) + (dy * dy)) * coeff;
    return dist;
}

static RouteArcPtr *
astar_shortest_path (RoutingNodesPtr e, RouteNodePtr nodes,
		     RouteNodePtr pfrom, RouteNodePtr pto,
		     double heuristic_coeff, int *ll)
{
/* identifying the Shortest Path - A* algorithm */
    int from;
    int to;
    int i;
    int k;
    RoutingNodePtr pAux;
    RoutingNodePtr n;
    RoutingNodePtr p_to;
    RouteNodePtr pOrg;
    RouteNodePtr pDest;
    RouteArcPtr p_link;
    int cnt;
    RouteArcPtr *result;
    RoutingHeapPtr heap;
/* setting From/To */
    from = pfrom->InternalIndex;
    to = pto->InternalIndex;
    pAux = e->Nodes + from;
    pOrg = nodes + pAux->Id;
    pAux = e->Nodes + to;
    pDest = nodes + pAux->Id;
/* initializing the heap */
    heap = routing_heap_init (e->DimLink);
/* initializing the graph */
    for (i = 0; i < e->Dim; i++)
      {
	  n = e->Nodes + i;
	  n->PreviousNode = NULL;
	  n->Arc = NULL;
	  n->Inspected = 0;
	  n->Distance = DBL_MAX;
	  n->HeuristicDistance = DBL_MAX;
      }
/* queuing the From node into the heap */
    e->Nodes[from].Distance = 0.0;
    e->Nodes[from].HeuristicDistance =
	astar_heuristic_distance (pOrg, pDest, heuristic_coeff);
    astar_enqueue (heap, e->Nodes + from);
    while (heap->Count > 0)
      {
	  /* A* loop */
	  n = routing_dequeue (heap);
	  if (n->Id == to)
	    {
		/* destination reached */
		break;
	    }
	  n->Inspected = 1;
	  for (i = 0; i < n->DimTo; i++)
	    {
		p_to = *(n->To + i);
		p_link = *(n->Link + i);
		if (p_to->Inspected == 0)
		  {
		      if (p_to->Distance == DBL_MAX)
			{
			    /* queuing a new node into the heap */
			    p_to->Distance = n->Distance + p_link->Cost;
			    pOrg = nodes + p_to->Id;
			    p_to->HeuristicDistance =
				p_to->Distance +
				astar_heuristic_distance (pOrg, pDest,
							  heuristic_coeff);
			    p_to->PreviousNode = n;
			    p_to->Arc = p_link;
			    astar_enqueue (heap, p_to);
			}
		      else if (p_to->Distance > n->Distance + p_link->Cost)
			{
			    /* updating an already inserted node */
			    p_to->Distance = n->Distance + p_link->Cost;
			    pOrg = nodes + p_to->Id;
			    p_to->HeuristicDistance =
				p_to->Distance +
				astar_heuristic_distance (pOrg, pDest,
							  heuristic_coeff);
			    p_to->PreviousNode = n;
			    p_to->Arc = p_link;
			}
		  }
	    }
      }
    routing_heap_free (heap);
    cnt = 0;
    n = e->Nodes + to;
    while (n->PreviousNode != NULL)
      {
	  /* counting how many Arcs are into the Shortest Path solution */
	  cnt++;
	  n = n->PreviousNode;
      }
/* allocating the solution */
    result = malloc (sizeof (RouteArcPtr) * cnt);
    k = cnt - 1;
    n = e->Nodes + to;
    while (n->PreviousNode != NULL)
      {
	  /* inserting an Arc  into the solution */
	  result[k] = n->Arc;
	  n = n->PreviousNode;
	  k--;
      }
    *ll = cnt;
    return (result);
}

/* END of A* Shortest Path implementation */

static int
cmp_nodes_code (const void *p1, const void *p2)
{
/* compares two nodes  by CODE [for BSEARCH] */
    RouteNodePtr pN1 = (RouteNodePtr) p1;
    RouteNodePtr pN2 = (RouteNodePtr) p2;
    return strcmp (pN1->Code, pN2->Code);
}

static int
cmp_nodes_id (const void *p1, const void *p2)
{
/* compares two nodes  by ID [for BSEARCH] */
    RouteNodePtr pN1 = (RouteNodePtr) p1;
    RouteNodePtr pN2 = (RouteNodePtr) p2;
    if (pN1->Id == pN2->Id)
	return 0;
    if (pN1->Id > pN2->Id)
	return 1;
    return -1;
}

static RouteNodePtr
find_node_by_code (RoutingPtr graph, const char *code)
{
/* searching a Node (by Code) into the sorted list */
    RouteNodePtr ret;
    RouteNode pN;
    pN.Code = (char *) code;
    ret =
	bsearch (&pN, graph->Nodes, graph->NumNodes, sizeof (RouteNode),
		 cmp_nodes_code);
    return ret;
}

static RouteNodePtr
find_node_by_id (RoutingPtr graph, const sqlite3_int64 id)
{
/* searching a Node (by Id) into the sorted list */
    RouteNodePtr ret;
    RouteNode pN;
    pN.Id = id;
    ret =
	bsearch (&pN, graph->Nodes, graph->NumNodes, sizeof (RouteNode),
		 cmp_nodes_id);
    return ret;
}

static void
vroute_delete_multiple_destinations (RoutingMultiDestPtr multiple)
{
/* deleting a multiple destinations request */
    if (multiple == NULL)
	return;
    if (multiple->Found != NULL)
	free (multiple->Found);
    if (multiple->To != NULL)
	free (multiple->To);
    if (multiple->Ids != NULL)
	free (multiple->Ids);
    if (multiple->Codes != NULL)
      {
	  int i;
	  for (i = 0; i < multiple->Items; i++)
	    {
		char *p = *(multiple->Codes + i);
		if (p != NULL)
		    free (p);
	    }
	  free (multiple->Codes);
      }
    free (multiple);
}

static void
vroute_add_multiple_code (RoutingMultiDestPtr multiple, char *code)
{
/* adding an item to an helper struct for Multiple Destinations (by Code) */
    *(multiple->Codes + multiple->Next) = code;
    multiple->Next += 1;
}

static void
vroute_add_multiple_id (RoutingMultiDestPtr multiple, int id)
{
/* adding an item to an helper struct for Multiple Destinations (by Code) */
    *(multiple->Ids + multiple->Next) = id;
    multiple->Next += 1;
}

static char *
vroute_parse_multiple_item (const char *start, const char *last)
{
/* attempting to parse an item form the multiple destinations list */
    char *item;
    const char *p_in = start;
    char *p_out;
    int len = last - start;
    if (len <= 0)
	return NULL;

    item = malloc (len + 1);
    p_in = start;
    p_out = item;
    while (p_in < last)
	*p_out++ = *p_in++;
    *p_out = '\0';
    return item;
}

static RoutingMultiDestPtr
vroute_as_multiple_destinations (sqlite3_int64 value)
{
/* allocatint a multiple destinations containign a single target */
    RoutingMultiDestPtr multiple = NULL;

/* allocating the helper struct */
    multiple = malloc (sizeof (RoutingMultiDest));
    multiple->CodeNode = 0;
    multiple->Found = malloc (sizeof (char));
    multiple->To = malloc (sizeof (RouteNodePtr));
    *(multiple->Found + 0) = 'N';
    *(multiple->To + 0) = NULL;
    multiple->Items = 1;
    multiple->Next = 0;
    multiple->Ids = malloc (sizeof (sqlite3_int64));
    multiple->Codes = NULL;
    *(multiple->Ids + 0) = value;
    return multiple;
}

static DestinationCandidatesListPtr
alloc_candidates (int code_node)
{
/* allocating a list of multiple destination candidates */
    DestinationCandidatesListPtr list =
	malloc (sizeof (DestinationCandidatesList));
    list->NodeCode = code_node;
    list->First = NULL;
    list->Last = NULL;
    list->ValidItems = 0;
    return list;
}

static void
delete_candidates (DestinationCandidatesListPtr list)
{
/* memory clean-up: destroying a list of multiple destination candidates */
    DestinationCandidatePtr pC;
    DestinationCandidatePtr pCn;
    if (list == NULL)
	return;

    pC = list->First;
    while (pC != NULL)
      {
	  pCn = pC->Next;
	  free (pC);
	  pC = pCn;
      }
    free (list);
}

static int
checkNumber (const char *str)
{
/* testing for an unsignd integer */
    int len;
    int i;
    len = strlen (str);
    for (i = 0; i < len; i++)
      {
	  if (*(str + i) < '0' || *(str + i) > '9')
	      return 0;
      }
    return 1;
}

static void
addMultiCandidate (DestinationCandidatesListPtr list, char *item)
{
/* adding a candidate to the list */
    DestinationCandidatePtr pC;
    if (list == NULL)
	return;
    if (item == NULL)
	return;
    if (list->NodeCode == 0)
      {
	  if (!checkNumber (item))
	    {
		/* invalid: not a number */
		free (item);
		return;
	    }
      }

    pC = malloc (sizeof (DestinationCandidate));
    if (list->NodeCode)
      {
	  pC->Code = item;
	  pC->Id = -1;
      }
    else
      {
	  pC->Code = NULL;
	  pC->Id = atoll (item);
	  free (item);
      }
    pC->Valid = 'Y';
    pC->Next = NULL;
    if (list->First == NULL)
	list->First = pC;
    if (list->Last != NULL)
	list->Last->Next = pC;
    list->Last = pC;
}

static void
validateMultiCandidates (DestinationCandidatesListPtr list)
{
/* validating candidates */
    DestinationCandidatePtr pC = list->First;
    while (pC != NULL)
      {
	  DestinationCandidatePtr pC2;
	  if (pC->Valid == 'N')
	    {
		pC = pC->Next;
		continue;
	    }
	  pC2 = pC->Next;
	  while (pC2 != NULL)
	    {
		/* testing for duplicates */
		if (pC2->Valid == 'N')
		  {
		      pC2 = pC2->Next;
		      continue;
		  }
		if (list->NodeCode)
		  {
		      if (strcmp (pC->Code, pC2->Code) == 0)
			{
			    free (pC2->Code);
			    pC2->Code = NULL;
			    pC2->Valid = 'N';
			}
		  }
		else
		  {
		      if (pC->Id == pC2->Id)
			  pC2->Valid = 'N';
		  }
		pC2 = pC2->Next;
	    }
	  pC = pC->Next;
      }

    list->ValidItems = 0;
    pC = list->First;
    while (pC != NULL)
      {
	  if (pC->Valid == 'Y')
	      list->ValidItems += 1;
	  pC = pC->Next;
      }
}

static RoutingMultiDestPtr
vroute_get_multiple_destinations (int code_node, char delimiter,
				  const char *str)
{
/* parsing a multiple destinations request string */
    RoutingMultiDestPtr multiple = NULL;
    DestinationCandidatesListPtr list = alloc_candidates (code_node);
    DestinationCandidatePtr pC;
    char *item;
    int i;
    const char *prev = str;
    const char *ptr = str;

/* populating a linked list */
    while (1)
      {
	  int whitespace = 0;
	  if (*ptr == '\0')
	    {
		item = vroute_parse_multiple_item (prev, ptr);
		addMultiCandidate (list, item);
		break;
	    }
	  if (*ptr == ' ' || *ptr == '\t' || *ptr == '\n' || *ptr == '\r')
	      whitespace = 1;
	  if (*ptr == delimiter || whitespace)
	    {
		item = vroute_parse_multiple_item (prev, ptr);
		addMultiCandidate (list, item);
		ptr++;
		prev = ptr;
		continue;
	    }
	  ptr++;
      }
    validateMultiCandidates (list);
    if (list->ValidItems <= 0)
      {
	  delete_candidates (list);
	  return NULL;
      }

/* allocating the helper struct */
    multiple = malloc (sizeof (RoutingMultiDest));
    multiple->CodeNode = code_node;
    multiple->Found = malloc (sizeof (char) * list->ValidItems);
    multiple->To = malloc (sizeof (RouteNodePtr) * list->ValidItems);
    for (i = 0; i < list->ValidItems; i++)
      {
	  *(multiple->Found + i) = 'N';
	  *(multiple->To + i) = NULL;
      }
    multiple->Items = list->ValidItems;
    multiple->Next = 0;
    if (code_node)
      {
	  multiple->Ids = NULL;
	  multiple->Codes = malloc (sizeof (char *) * list->ValidItems);
      }
    else
      {
	  multiple->Ids = malloc (sizeof (sqlite3_int64) * list->ValidItems);
	  multiple->Codes = NULL;
      }

/* populating the array */
    pC = list->First;
    while (pC != NULL)
      {
	  if (pC->Valid == 'Y')
	    {
		if (code_node)
		    vroute_add_multiple_code (multiple, pC->Code);
		else
		    vroute_add_multiple_id (multiple, pC->Id);
	    }
	  pC = pC->Next;
      }
    delete_candidates (list);
    return multiple;
}

static void
delete_multiSolution (MultiSolutionPtr multiSolution)
{
/* deleting the current solution */
    ResultsetRowPtr pR;
    ResultsetRowPtr pRn;
    ShortestPathSolutionPtr pS;
    ShortestPathSolutionPtr pSn;
    RowNodeSolutionPtr pN;
    RowNodeSolutionPtr pNn;
    RowSolutionPtr pA;
    RowSolutionPtr pAn;
    gaiaGeomCollPtr pG;
    gaiaGeomCollPtr pGn;
    if (!multiSolution)
	return;
    if (multiSolution->MultiTo != NULL)
	vroute_delete_multiple_destinations (multiSolution->MultiTo);
    pS = multiSolution->First;
    while (pS != NULL)
      {
	  pSn = pS->Next;
	  delete_solution (pS);
	  pS = pSn;
      }
    pN = multiSolution->FirstNode;
    while (pN != NULL)
      {
	  pNn = pN->Next;
	  free (pN);
	  pN = pNn;
      }
    pR = multiSolution->FirstRow;
    while (pR != NULL)
      {
	  pRn = pR->Next;
	  if (pR->Undefined != NULL)
	      free (pR->Undefined);
	  free (pR);
	  pR = pRn;
      }
    pA = multiSolution->FirstArc;
    while (pA != NULL)
      {
	  pAn = pA->Next;
	  if (pA->Name)
	      free (pA->Name);
	  free (pA);
	  pA = pAn;
      }
    pG = multiSolution->FirstGeom;
    while (pG != NULL)
      {
	  pGn = pG->Next;
	  gaiaFreeGeomColl (pG);
	  pG = pGn;
      }
    free (multiSolution);
}

static void
reset_multiSolution (MultiSolutionPtr multiSolution)
{
/* resetting the current solution */
    ResultsetRowPtr pR;
    ResultsetRowPtr pRn;
    ShortestPathSolutionPtr pS;
    ShortestPathSolutionPtr pSn;
    RowNodeSolutionPtr pN;
    RowNodeSolutionPtr pNn;
    RowSolutionPtr pA;
    RowSolutionPtr pAn;
    gaiaGeomCollPtr pG;
    gaiaGeomCollPtr pGn;
    if (!multiSolution)
	return;
    if (multiSolution->MultiTo != NULL)
	vroute_delete_multiple_destinations (multiSolution->MultiTo);
    pS = multiSolution->First;
    while (pS != NULL)
      {
	  pSn = pS->Next;
	  delete_solution (pS);
	  pS = pSn;
      }
    pN = multiSolution->FirstNode;
    while (pN != NULL)
      {
	  pNn = pN->Next;
	  free (pN);
	  pN = pNn;
      }
    pR = multiSolution->FirstRow;
    while (pR != NULL)
      {
	  pRn = pR->Next;
	  free (pR);
	  pR = pRn;
      }
    pA = multiSolution->FirstArc;
    while (pA != NULL)
      {
	  pAn = pA->Next;
	  if (pA->Name)
	      free (pA->Name);
	  free (pA);
	  pA = pAn;
      }
    pG = multiSolution->FirstGeom;
    while (pG != NULL)
      {
	  pGn = pG->Next;
	  gaiaFreeGeomColl (pG);
	  pG = pGn;
      }
    multiSolution->From = NULL;
    multiSolution->MultiTo = NULL;
    multiSolution->First = NULL;
    multiSolution->Last = NULL;
    multiSolution->FirstRow = NULL;
    multiSolution->LastRow = NULL;
    multiSolution->FirstNode = NULL;
    multiSolution->LastNode = NULL;
    multiSolution->CurrentRow = NULL;
    multiSolution->CurrentNodeRow = NULL;
    multiSolution->CurrentRowId = 0;
    multiSolution->FirstArc = NULL;
    multiSolution->LastArc = NULL;
    multiSolution->FirstGeom = NULL;
    multiSolution->LastGeom = NULL;
}

static MultiSolutionPtr
alloc_multiSolution (void)
{
/* allocates and initializes the current multiple-destinations solution */
    MultiSolutionPtr p = malloc (sizeof (MultiSolution));
    p->From = NULL;
    p->MultiTo = NULL;
    p->First = NULL;
    p->Last = NULL;
    p->FirstRow = NULL;
    p->LastRow = NULL;
    p->CurrentRow = NULL;
    p->FirstNode = NULL;
    p->LastNode = NULL;
    p->CurrentNodeRow = NULL;
    p->CurrentRowId = 0;
    p->FirstArc = NULL;
    p->LastArc = NULL;
    p->FirstGeom = NULL;
    p->LastGeom = NULL;
    return p;
}

static int
find_srid (sqlite3 * handle, RoutingPtr graph)
{
/* attempting to retrieve the appropriate Srid */
    sqlite3_stmt *stmt;
    int ret;
    int srid = VROUTE_INVALID_SRID;
    char *sql;

    if (graph->GeometryColumn == NULL)
	return srid;

    sql = sqlite3_mprintf ("SELECT srid FROM geometry_columns WHERE "
			   "Lower(f_table_name) = Lower(%Q) AND Lower(f_geometry_column) = Lower(%Q)",
			   graph->TableName, graph->GeometryColumn);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return srid;
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	      srid = sqlite3_column_int (stmt, 0);
      }
    sqlite3_finalize (stmt);
    return srid;
}

static void
build_range_solution (MultiSolutionPtr multiSolution,
		      RoutingNodePtr * range_nodes, int cnt, int srid)
{
/* formatting the "within Cost range" solution */
    int i;
    if (cnt > 0)
      {
	  /* building the solution */
	  for (i = 0; i < cnt; i++)
	    {
		add_node_to_solution (multiSolution, range_nodes[i], srid, i);
	    }
      }
    if (range_nodes)
	free (range_nodes);
}

static RouteNodePtr
findSingleTo (RoutingMultiDestPtr multiple)
{
/* testing for a single destination */
    int i;
    RouteNodePtr to = NULL;
    int count = 0;
    for (i = 0; i < multiple->Items; i++)
      {
	  RouteNodePtr p_to = *(multiple->To + i);
	  if (p_to == NULL)
	      continue;
	  to = p_to;
	  count++;
      }
    if (count == 1)
	return to;
    return NULL;
}

static void
astar_solve (sqlite3 * handle, int options, RoutingPtr graph,
	     RoutingNodesPtr routing, MultiSolutionPtr multiSolution)
{
/* computing an A* Shortest Path solution */
    int cnt;
    RouteArcPtr *shortest_path;
    ShortestPathSolutionPtr solution;
    RouteNodePtr to = findSingleTo (multiSolution->MultiTo);
    if (to == NULL)
	return;
    shortest_path =
	astar_shortest_path (routing, graph->Nodes, multiSolution->From,
			     to, graph->AStarHeuristicCoeff, &cnt);
    solution = add2multiSolution (multiSolution, multiSolution->From, to);
    build_solution (handle, options, graph, solution, shortest_path, cnt);
    build_multi_solution (multiSolution);
}

static void
dijkstra_multi_solve (sqlite3 * handle, int options, RoutingPtr graph,
		      RoutingNodesPtr routing, MultiSolutionPtr multiSolution)
{
/* computing a Dijkstra Shortest Path multiSolution */
    int i;
    RoutingMultiDestPtr multiple = multiSolution->MultiTo;

    dijkstra_multi_shortest_path (handle, options, graph, routing,
				  multiSolution);
/* testing if there are undefined or unresolved destinations */
    for (i = 0; i < multiple->Items; i++)
      {
	  int len;
	  ShortestPathSolutionPtr row;
	  RouteNodePtr to = *(multiple->To + i);
	  const char *code = *(multiple->Codes + i);
	  if (to == NULL)
	    {
		row =
		    add2multiSolution (multiSolution, multiSolution->From,
				       NULL);
		len = strlen (code);
		row->Undefined = malloc (len + 1);
		strcpy (row->Undefined, code);
		continue;
	    }
	  if (*(multiple->Found + i) != 'Y')
	    {
		row =
		    add2multiSolution (multiSolution, multiSolution->From, to);
		len = strlen (code);
		row->Undefined = malloc (len + 1);
		strcpy (row->Undefined, code);
	    }
      }
    build_multi_solution (multiSolution);
}

static void
dijkstra_within_cost_range (RoutingNodesPtr routing,
			    MultiSolutionPtr multiSolution, int srid)
{
/* computing a Dijkstra "within cost range" solution */
    int cnt;
    RoutingNodePtr *range_nodes =
	dijkstra_range_analysis (routing, multiSolution->From,
				 multiSolution->MaxCost,
				 &cnt);
    build_range_solution (multiSolution, range_nodes, cnt, srid);
}

static void
tsp_nn_solve (sqlite3 * handle, int options, RoutingPtr graph,
	      RoutingNodesPtr routing, MultiSolutionPtr multiSolution)
{
/* computing a Dijkstra TSP NN Solution */
    int i;
    TspTargetsPtr targets = randomize_targets (handle, graph, multiSolution);
    for (i = 0; i < targets->Count; i++)
      {
	  /* checking for undefined targets */
	  if (*(targets->To + i) == NULL)
	      goto illegal;
      }
    dijkstra_tsp_nn (handle, options, graph, routing, targets);
    build_tsp_solution (multiSolution, targets);
    destroy_tsp_targets (targets);
    return;

  illegal:
    build_tsp_illegal_solution (multiSolution, targets);
    destroy_tsp_targets (targets);
}

static void
tsp_ga_random_solutions_sql (TspGaPopulationPtr ga)
{
/* building the Solutions randomizing SQL query */
    int i;
    char *sql;
    char *prev_sql;

    for (i = 0; i < ga->Count; i++)
      {
	  if (i == 0)
	      sql = sqlite3_mprintf ("SELECT %d, Random() AS rnd\n", i);
	  else
	    {
		prev_sql = sql;
		sql =
		    sqlite3_mprintf ("%sUNION\nSELECT %d, Random() AS rnd\n",
				     prev_sql, i);
		sqlite3_free (prev_sql);
	    }
      }
    prev_sql = sql;
    sql = sqlite3_mprintf ("%sORDER BY rnd LIMIT 2", prev_sql);
    sqlite3_free (prev_sql);
    ga->RandomSolutionsSql = sql;
}

static void
tsp_ga_random_interval_sql (TspGaPopulationPtr ga)
{
/* building the Interval randomizing SQL query */
    int i;
    char *sql;
    char *prev_sql;

    for (i = 0; i < ga->Cities; i++)
      {
	  if (i == 0)
	      sql = sqlite3_mprintf ("SELECT %d, Random() AS rnd\n", i);
	  else
	    {
		prev_sql = sql;
		sql =
		    sqlite3_mprintf ("%sUNION\nSELECT %d, Random() AS rnd\n",
				     prev_sql, i);
		sqlite3_free (prev_sql);
	    }
      }
    prev_sql = sql;
    sql = sqlite3_mprintf ("%sORDER BY rnd LIMIT 2", prev_sql);
    sqlite3_free (prev_sql);
    ga->RandomIntervalSql = sql;
}

static TspGaDistancePtr
alloc_tsp_ga_distances (TspTargetsPtr targets)
{
/* allocating a TSP GA distances struct */
    int i;
    TspGaDistancePtr dist = malloc (sizeof (TspGaDistance));
    dist->Cities = targets->Count;
    dist->CityFrom = targets->From;
    dist->Distances = malloc (sizeof (TspGaSubDistancePtr) * dist->Cities);
    for (i = 0; i < dist->Cities; i++)
      {
	  TspGaSubDistancePtr sub = malloc (sizeof (TspGaSubDistance));
	  double cost = *(targets->Costs + i);
	  sub->CityTo = *(targets->To + i);
	  sub->Cost = cost;
	  *(dist->Distances + i) = sub;
      }
    dist->NearestIndex = -1;
    return dist;
}

static void
destroy_tsp_ga_distances (TspGaDistancePtr dist)
{
/* freeing a TSP GA distance struct */
    if (dist == NULL)
	return;
    if (dist->Distances != NULL)
      {
	  int i;
	  for (i = 0; i < dist->Cities; i++)
	    {
		TspGaSubDistancePtr sub = *(dist->Distances + i);
		if (sub != NULL)
		    free (sub);
	    }
	  free (dist->Distances);
      }
    free (dist);
}

static TspGaPopulationPtr
build_tsp_ga_population (int count)
{
/* creating a TSP GA Population */
    int i;
    TspGaPopulationPtr ga = malloc (sizeof (TspGaPopulation));
    ga->Count = count;
    ga->Cities = count;
    ga->Solutions = malloc (sizeof (TspGaSolutionPtr) * count);
    ga->Offsprings = malloc (sizeof (TspGaSolutionPtr) * count);
    for (i = 0; i < count; i++)
      {
	  *(ga->Offsprings + i) = NULL;
	  *(ga->Solutions + i) = NULL;
      }
    ga->Distances = malloc (sizeof (TspGaDistancePtr) * count);
    for (i = 0; i < count; i++)
	*(ga->Distances + i) = NULL;
    ga->RandomSolutionsSql = NULL;
    tsp_ga_random_solutions_sql (ga);
    ga->RandomIntervalSql = NULL;
    tsp_ga_random_interval_sql (ga);
    return ga;
}

static void
destroy_tsp_ga_solution (TspGaSolutionPtr solution)
{
/* memory cleanup: destroyng a GA Solution */
    if (solution == NULL)
	return;
    if (solution->CitiesFrom != NULL)
	free (solution->CitiesFrom);
    if (solution->CitiesTo != NULL)
	free (solution->CitiesTo);
    if (solution->Costs != NULL)
	free (solution->Costs);
    free (solution);
}

static void
free_tsp_ga_offsprings (TspGaPopulationPtr ga)
{
/* memory cleanup; freeing GA Offsprings */
    int i;
    if (ga == NULL)
	return;

    for (i = 0; i < ga->Count; i++)
      {
	  if (*(ga->Offsprings + i) != NULL)
	      destroy_tsp_ga_solution (*(ga->Offsprings + i));
	  *(ga->Offsprings + i) = NULL;
      }
}

static void
destroy_tsp_ga_population (TspGaPopulationPtr ga)
{
/* memory cleanup; destroyng a GA Population */
    int i;
    if (ga == NULL)
	return;

    for (i = 0; i < ga->Count; i++)
	destroy_tsp_ga_solution (*(ga->Solutions + i));
    free (ga->Solutions);
    free_tsp_ga_offsprings (ga);
    free (ga->Offsprings);
    if (ga->Distances != NULL)
      {
	  for (i = 0; i < ga->Cities; i++)
	    {
		TspGaDistancePtr dist = *(ga->Distances + i);
		if (dist != NULL)
		    destroy_tsp_ga_distances (dist);
	    }
      }
    free (ga->Distances);
    if (ga->RandomSolutionsSql != NULL)
	sqlite3_free (ga->RandomSolutionsSql);
    if (ga->RandomIntervalSql != NULL)
	sqlite3_free (ga->RandomIntervalSql);
    free (ga);
}

static int
cmp_dist_from (const void *p1, const void *p2)
{
/* compares FROM distances [for BSEARCH] */
    TspGaDistancePtr pD1 = (TspGaDistancePtr) p1;
    TspGaDistancePtr pD2 = *((TspGaDistancePtr *) p2);
    RouteNodePtr pN1 = pD1->CityFrom;
    RouteNodePtr pN2 = pD2->CityFrom;
    if (pN1 == pN2)
	return 0;
    if (pN1 > pN2)
	return 1;
    return -1;
}

static TspGaDistancePtr
tsp_ga_find_from_distance (TspGaPopulationPtr ga, RouteNodePtr from)
{
/* searching the main FROM distance */
    TspGaDistancePtr *ret;
    TspGaDistance dist;
    dist.CityFrom = from;
    ret =
	bsearch (&dist, ga->Distances, ga->Cities, sizeof (TspGaDistancePtr),
		 cmp_dist_from);
    if (ret == NULL)
	return NULL;
    return *ret;
}

static int
cmp_dist_to (const void *p1, const void *p2)
{
/* compares TO distances [for BSEARCH] */
    TspGaSubDistancePtr pD1 = (TspGaSubDistancePtr) p1;
    TspGaSubDistancePtr pD2 = *((TspGaSubDistancePtr *) p2);
    RouteNodePtr pN1 = pD1->CityTo;
    RouteNodePtr pN2 = pD2->CityTo;
    if (pN1 == pN2)
	return 0;
    if (pN1 > pN2)
	return 1;
    return -1;
}

static TspGaSubDistancePtr
tsp_ga_find_to_distance (TspGaDistancePtr dist, RouteNodePtr to)
{
/* searching the main FROM distance */
    TspGaSubDistancePtr *ret;
    TspGaSubDistance sub;
    sub.CityTo = to;
    ret =
	bsearch (&sub, dist->Distances, dist->Cities,
		 sizeof (TspGaSubDistancePtr), cmp_dist_to);
    if (ret == NULL)
	return NULL;
    return *ret;
}

static double
tsp_ga_find_distance (TspGaPopulationPtr ga, RouteNodePtr from, RouteNodePtr to)
{
/* searching a cached distance */
    TspGaSubDistancePtr sub;
    TspGaDistancePtr dist = tsp_ga_find_from_distance (ga, from);
    if (dist == NULL)
	return DBL_MAX;

    sub = tsp_ga_find_to_distance (dist, to);
    if (sub != NULL)
	return sub->Cost;
    return DBL_MAX;
}

static void
tsp_ga_random_solutions (sqlite3 * handle, TspGaPopulationPtr ga, int *index1,
			 int *index2)
{
/* fetching two random TSP GA solutions */
    int i;
    int ret;
    int n_rows;
    int n_columns;
    const char *value = NULL;
    char **results;

    *index1 = -1;
    *index2 = -1;

    ret =
	sqlite3_get_table (handle, ga->RandomSolutionsSql, &results, &n_rows,
			   &n_columns, NULL);
    if (ret != SQLITE_OK)
	return;
    if (n_rows >= 1)
      {
	  for (i = 1; i <= n_rows; i++)
	    {
		value = results[(i * n_columns) + 0];
		if (i == 1)
		    *index1 = atoi (value);
		else
		    *index2 = atoi (value);
	    }
      }
    sqlite3_free_table (results);
}

static void
tsp_ga_random_interval (sqlite3 * handle, TspGaPopulationPtr ga, int *index1,
			int *index2)
{
/* fetching a random TSP GA interval */
    int i;
    int ret;
    int n_rows;
    int n_columns;
    const char *value = NULL;
    char **results;

    *index1 = -1;
    *index2 = -1;

    ret =
	sqlite3_get_table (handle, ga->RandomIntervalSql, &results, &n_rows,
			   &n_columns, NULL);
    if (ret != SQLITE_OK)
	return;
    if (n_rows >= 1)
      {
	  for (i = 1; i <= n_rows; i++)
	    {
		value = results[(i * n_columns) + 0];
		if (i == 1)
		    *index1 = atoi (value);
		else
		    *index2 = atoi (value);
	    }
      }
    sqlite3_free_table (results);
}

static void
tps_ga_chromosome_update (TspGaSolutionPtr chromosome, RouteNodePtr from,
			  RouteNodePtr to, double cost)
{
/* updating a chromosome (missing distance) */
    int j;
    for (j = 0; j < chromosome->Cities; j++)
      {
	  RouteNodePtr n1 = *(chromosome->CitiesFrom + j);
	  RouteNodePtr n2 = *(chromosome->CitiesTo + j);
	  if (n1 == from && n2 == to)
	      *(chromosome->Costs + j) = cost;
      }
}

static void
tsp_ga_random_mutation (sqlite3 * handle, TspGaPopulationPtr ga,
			TspGaSolutionPtr mutant)
{
/* introducing a random mutation */
    RouteNodePtr mutation;
    int j;
    int idx1;
    int idx2;

/* applying a random mutation */
    tsp_ga_random_interval (handle, ga, &idx1, &idx2);
    mutation = *(mutant->CitiesFrom + idx1);
    *(mutant->CitiesFrom + idx1) = *(mutant->CitiesFrom + idx2);
    *(mutant->CitiesFrom + idx2) = mutation;

/* adjusting From/To */
    for (j = 1; j < mutant->Cities; j++)
      {
	  RouteNodePtr pFrom = *(mutant->CitiesFrom + j);
	  *(mutant->CitiesTo + j - 1) = pFrom;
      }
    *(mutant->CitiesTo + mutant->Cities - 1) = *(mutant->CitiesFrom + 0);

/* adjusting Costs */
    mutant->TotalCost = 0.0;
    for (j = 0; j < mutant->Cities; j++)
      {
	  RouteNodePtr pFrom = *(mutant->CitiesFrom + j);
	  RouteNodePtr pTo = *(mutant->CitiesTo + j);
	  double cost = tsp_ga_find_distance (ga, pFrom, pTo);
	  tps_ga_chromosome_update (mutant, pFrom, pTo, cost);
	  *(mutant->Costs + j) = cost;
	  mutant->TotalCost += cost;
      }
}

static TspGaSolutionPtr
tsp_ga_clone_solution (TspGaPopulationPtr ga, TspGaSolutionPtr original)
{
/* cloning a TSP GA solution */
    int j;
    TspGaSolutionPtr clone;
    if (original == NULL)
	return NULL;

    clone = malloc (sizeof (TspGaSolution));
    clone->Cities = original->Cities;
    clone->CitiesFrom = malloc (sizeof (RouteNodePtr) * ga->Cities);
    clone->CitiesTo = malloc (sizeof (RouteNodePtr) * ga->Cities);
    clone->Costs = malloc (sizeof (double) * ga->Cities);
    for (j = 0; j < ga->Cities; j++)
      {
	  *(clone->CitiesFrom + j) = *(original->CitiesFrom + j);
	  *(clone->CitiesTo + j) = *(original->CitiesTo + j);
	  *(clone->Costs + j) = *(original->Costs + j);
      }
    clone->TotalCost = 0.0;
    return clone;
}

static TspGaSolutionPtr
tsp_ga_crossover (sqlite3 * handle, TspGaPopulationPtr ga, int mutation1,
		  int mutation2)
{
/* creating a Crossover solution */
    int j;
    int idx1;
    int idx2;
    TspGaSolutionPtr hybrid;
    TspGaSolutionPtr parent1 = NULL;
    TspGaSolutionPtr parent2 = NULL;

/* randomly choosing two parents */
    tsp_ga_random_solutions (handle, ga, &idx1, &idx2);
    if (idx1 >= 0 && idx1 < ga->Count)
	parent1 = tsp_ga_clone_solution (ga, *(ga->Solutions + idx1));
    if (idx2 >= 0 && idx2 < ga->Count)
	parent2 = tsp_ga_clone_solution (ga, *(ga->Solutions + idx2));
    if (parent1 == NULL || parent2 == NULL)
	goto stop;

    if (mutation1)
	tsp_ga_random_mutation (handle, ga, parent1);
    if (mutation2)
	tsp_ga_random_mutation (handle, ga, parent2);

/* creating an empty hybrid */
    hybrid = malloc (sizeof (TspGaSolution));
    hybrid->Cities = ga->Cities;
    hybrid->CitiesFrom = malloc (sizeof (RouteNodePtr) * ga->Cities);
    hybrid->CitiesTo = malloc (sizeof (RouteNodePtr) * ga->Cities);
    hybrid->Costs = malloc (sizeof (double) * ga->Cities);
    for (j = 0; j < ga->Cities; j++)
      {
	  *(hybrid->CitiesFrom + j) = NULL;
	  *(hybrid->CitiesTo + j) = NULL;
	  *(hybrid->Costs + j) = DBL_MAX;
      }
    hybrid->TotalCost = 0.0;

/* step #1: inheritance from the fist parent */
    tsp_ga_random_interval (handle, ga, &idx1, &idx2);
    if (idx1 < idx2)
      {
	  for (j = idx1; j <= idx2; j++)
	      *(hybrid->CitiesFrom + j) = *(parent1->CitiesFrom + j);
      }
    else
      {
	  for (j = idx2; j <= idx1; j++)
	      *(hybrid->CitiesFrom + j) = *(parent1->CitiesFrom + j);
      }

/* step #2: inheritance from the second parent */
    for (j = 0; j < parent2->Cities; j++)
      {
	  RouteNodePtr p2From = *(parent2->CitiesFrom + j);
	  int k;
	  int found = 0;
	  if (p2From == NULL)
	      continue;
	  for (k = 0; k < hybrid->Cities; k++)
	    {
		RouteNodePtr p1From = *(hybrid->CitiesFrom + k);
		if (p1From == NULL)
		    continue;
		if (p1From == p2From)
		  {
		      /* already present: skipping */
		      found = 1;
		      break;
		  }
	    }
	  if (found)
	      continue;
	  for (k = 0; k < hybrid->Cities; k++)
	    {
		if (*(hybrid->CitiesFrom + k) == NULL
		    && *(hybrid->CitiesTo + k) == NULL
		    && *(hybrid->Costs + k) == DBL_MAX)
		  {
		      /* found an empty slot: inserting */
		      *(hybrid->CitiesFrom + k) = p2From;
		      break;
		  }
	    }
      }
    destroy_tsp_ga_solution (parent1);
    destroy_tsp_ga_solution (parent2);

/* adjusting From/To */
    for (j = 1; j < hybrid->Cities; j++)
      {
	  RouteNodePtr p1From = *(hybrid->CitiesFrom + j);
	  *(hybrid->CitiesTo + j - 1) = p1From;
      }
    *(hybrid->CitiesTo + hybrid->Cities - 1) = *(hybrid->CitiesFrom + 0);

/* retrieving cached costs */
    for (j = 0; j < hybrid->Cities; j++)
      {
	  RouteNodePtr pFrom = *(hybrid->CitiesFrom + j);
	  RouteNodePtr pTo = *(hybrid->CitiesTo + j);
	  double cost = tsp_ga_find_distance (ga, pFrom, pTo);
	  tps_ga_chromosome_update (hybrid, pFrom, pTo, cost);
	  hybrid->TotalCost += cost;
      }
    return hybrid;

  stop:
    if (parent1 != NULL)
	destroy_tsp_ga_solution (parent1);
    if (parent2 != NULL)
	destroy_tsp_ga_solution (parent2);
    return NULL;
}

static void
evalTspGaFitness (TspGaPopulationPtr ga)
{
/* evaluating the comparative fitness of parents and offsprings */
    int j;
    int i;
    int index;
    int already_defined = 0;

    for (j = 0; j < ga->Count; j++)
      {
	  /* evaluating an offsprings */
	  double max_cost = 0.0;
	  TspGaSolutionPtr hybrid = *(ga->Offsprings + j);

	  for (i = 0; i < ga->Count; i++)
	    {
		/* searching the worst parent */
		TspGaSolutionPtr old = *(ga->Solutions + i);
		if (old->TotalCost > max_cost)
		  {
		      max_cost = old->TotalCost;
		      index = i;
		  }
		if (old->TotalCost == hybrid->TotalCost)
		    already_defined = 1;
	    }
	  if (max_cost > hybrid->TotalCost && !already_defined)
	    {
		/* inserting the new hybrid by replacing the worst parent */
		TspGaSolutionPtr kill = *(ga->Solutions + index);
		*(ga->Solutions + index) = hybrid;
		*(ga->Offsprings + j) = NULL;
		destroy_tsp_ga_solution (kill);
	    }
      }
}

static MultiSolutionPtr
tsp_ga_compute_route (sqlite3 * handle, int options, RouteNodePtr origin,
		      RouteNodePtr destination, RoutingPtr graph,
		      RoutingNodesPtr routing)
{
/* computing a route from City to City */
    RoutingMultiDestPtr to = NULL;
    MultiSolutionPtr ms = alloc_multiSolution ();
    ms->From = origin;
    to = malloc (sizeof (RoutingMultiDest));
    ms->MultiTo = to;
    to->CodeNode = graph->NodeCode;
    to->Found = malloc (sizeof (char));
    to->To = malloc (sizeof (RouteNodePtr));
    *(to->Found + 0) = 'N';
    *(to->To + 0) = destination;
    to->Items = 1;
    to->Next = 0;
    if (graph->NodeCode)
      {
	  int len = strlen (destination->Code);
	  to->Ids = NULL;
	  to->Codes = malloc (sizeof (char *));
	  *(to->Codes + 0) = malloc (len + 1);
	  strcpy (*(to->Codes + 0), destination->Code);
      }
    else
      {
	  to->Ids = malloc (sizeof (sqlite3_int64));
	  to->Codes = NULL;
	  *(to->Ids + 0) = destination->Id;
      }

    dijkstra_multi_shortest_path (handle, options, graph, routing, ms);
    return (ms);
}

static void
completing_tsp_ga_solution (sqlite3 * handle, int options, RouteNodePtr origin,
			    RouteNodePtr destination, RoutingPtr graph,
			    RoutingNodesPtr routing, TspTargetsPtr targets,
			    int j)
{
/* completing a TSP GA solution */
    ShortestPathSolutionPtr solution;
    MultiSolutionPtr result =
	tsp_ga_compute_route (handle, options, origin, destination, graph,
			      routing);

    solution = result->First;
    while (solution != NULL)
      {
	  RowSolutionPtr old;
	  ShortestPathSolutionPtr newSolution = alloc_solution ();
	  newSolution->From = origin;
	  newSolution->To = destination;
	  newSolution->TotalCost += solution->TotalCost;
	  targets->TotalCost += solution->TotalCost;
	  newSolution->Geometry = solution->Geometry;
	  solution->Geometry = NULL;
	  if (j < 0)
	      targets->LastSolution = newSolution;
	  else
	      *(targets->Solutions + j) = newSolution;
	  old = solution->First;
	  while (old != NULL)
	    {
		/* inserts an Arc into the Shortest Path solution */
		RowSolutionPtr p = malloc (sizeof (RowSolution));
		p->Arc = old->Arc;
		p->Name = old->Name;
		old->Name = NULL;
		p->Next = NULL;
		if (!(newSolution->First))
		    newSolution->First = p;
		if (newSolution->Last)
		    newSolution->Last->Next = p;
		newSolution->Last = p;
		old = old->Next;
	    }
	  solution = solution->Next;
      }
    delete_multiSolution (result);
}

static void
set_tsp_ga_targets (sqlite3 * handle, int options, RoutingPtr graph,
		    RoutingNodesPtr routing, TspGaSolutionPtr bestSolution,
		    TspTargetsPtr targets)
{
/* preparing TSP GA targets (best solution found) */
    int j;
    RouteNodePtr from;
    RouteNodePtr to;

    for (j = 0; j < targets->Count; j++)
      {
	  from = *(bestSolution->CitiesFrom + j);
	  to = *(bestSolution->CitiesTo + j);
	  completing_tsp_ga_solution (handle, options, from, to,
				      graph, routing, targets, j);
	  *(targets->To + j) = to;
	  *(targets->Found + j) = 'Y';
      }
    /* this is the final City closing the circular path */
    from = *(bestSolution->CitiesFrom + targets->Count);
    to = *(bestSolution->CitiesTo + targets->Count);
    completing_tsp_ga_solution (handle, options, from, to, graph,
				routing, targets, -1);
}

static TspTargetsPtr
build_tsp_ga_solution_targets (int count, RouteNodePtr from)
{
/* creating and initializing the TSP helper struct - final solution */
    int i;
    TspTargetsPtr targets = malloc (sizeof (TspTargets));
    targets->Mode = VROUTE_TSP_SOLUTION;
    targets->TotalCost = 0.0;
    targets->Count = count;
    targets->To = malloc (sizeof (RouteNodePtr *) * targets->Count);
    targets->Found = malloc (sizeof (char) * targets->Count);
    targets->Costs = malloc (sizeof (double) * targets->Count);
    targets->Solutions =
	malloc (sizeof (ShortestPathSolutionPtr) * targets->Count);
    targets->LastSolution = NULL;
    targets->From = from;
    for (i = 0; i < targets->Count; i++)
      {
	  *(targets->To + i) = NULL;
	  *(targets->Found + i) = 'N';
	  *(targets->Costs + i) = DBL_MAX;
	  *(targets->Solutions + i) = NULL;
      }
    return targets;
}

static TspTargetsPtr
tsp_ga_permuted_targets (RouteNodePtr from, RoutingMultiDestPtr multi,
			 int index)
{
/* initializing the TSP helper struct - permuted */
    int i;
    TspTargetsPtr targets = malloc (sizeof (TspTargets));
    targets->Mode = VROUTE_ROUTING_SOLUTION;
    targets->TotalCost = 0.0;
    targets->Count = multi->Items;
    targets->To = malloc (sizeof (RouteNodePtr *) * targets->Count);
    targets->Found = malloc (sizeof (char) * targets->Count);
    targets->Costs = malloc (sizeof (double) * targets->Count);
    targets->Solutions =
	malloc (sizeof (ShortestPathSolutionPtr) * targets->Count);
    targets->LastSolution = NULL;
    if (index < 0)
      {
	  /* no permutation */
	  targets->From = from;
	  for (i = 0; i < targets->Count; i++)
	    {
		*(targets->To + i) = *(multi->To + i);
		*(targets->Found + i) = 'N';
		*(targets->Costs + i) = DBL_MAX;
		*(targets->Solutions + i) = NULL;
	    }
      }
    else
      {
	  /* permuting */
	  targets->From = *(multi->To + index);
	  for (i = 0; i < targets->Count; i++)
	    {
		if (i == index)
		  {
		      *(targets->To + i) = from;
		      *(targets->Found + i) = 'N';
		      *(targets->Costs + i) = DBL_MAX;
		      *(targets->Solutions + i) = NULL;
		  }
		else
		  {
		      *(targets->To + i) = *(multi->To + i);
		      *(targets->Found + i) = 'N';
		      *(targets->Costs + i) = DBL_MAX;
		      *(targets->Solutions + i) = NULL;
		  }
	    }
      }
    return targets;
}

static int
build_tsp_nn_solution (TspGaPopulationPtr ga, TspTargetsPtr targets, int index)
{
/* building a TSP NN solution */
    int j;
    int i;
    RouteNodePtr origin;
    TspGaDistancePtr dist;
    TspGaSubDistancePtr sub;
    RouteNodePtr destination;
    double cost;
    TspGaSolutionPtr solution = malloc (sizeof (TspGaSolution));
    solution->Cities = targets->Count + 1;
    solution->CitiesFrom = malloc (sizeof (RouteNodePtr) * solution->Cities);
    solution->CitiesTo = malloc (sizeof (RouteNodePtr) * solution->Cities);
    solution->Costs = malloc (sizeof (double) * solution->Cities);
    solution->TotalCost = 0.0;

    origin = targets->From;
    for (j = 0; j < targets->Count; j++)
      {
	  /* searching the nearest City */
	  dist = tsp_ga_find_from_distance (ga, origin);
	  if (dist == NULL)
	      return 0;
	  destination = NULL;
	  cost = DBL_MAX;

	  sub = *(dist->Distances + dist->NearestIndex);
	  destination = sub->CityTo;
	  cost = sub->Cost;
	  /* excluding the last City (should be a closed circuit) */
	  if (destination == targets->From)
	      destination = NULL;
	  if (destination != NULL)
	    {
		for (i = 0; i < targets->Count; i++)
		  {
		      /* checking if this City was already reached */
		      RouteNodePtr city = *(targets->To + i);
		      if (city == destination)
			{
			    if (*(targets->Found + i) == 'Y')
				destination = NULL;
			    else
				*(targets->Found + i) = 'Y';
			    break;
			}
		  }
	    }
	  if (destination == NULL)
	    {
		/* searching for an alternative destination */
		double min = DBL_MAX;
		int ind = -1;
		int k;
		for (k = 0; k < dist->Cities; k++)
		  {
		      sub = *(dist->Distances + k);
		      RouteNodePtr city = sub->CityTo;
		      /* excluding the last City (should be a closed circuit) */
		      if (city == targets->From)
			  continue;
		      for (i = 0; i < targets->Count; i++)
			{
			    RouteNodePtr city2 = *(targets->To + i);
			    if (*(targets->Found + i) == 'Y')
				continue;
			    if (city == city2)
			      {
				  if (sub->Cost < min)
				    {
					min = sub->Cost;
					ind = k;
				    }
			      }
			}
		  }
		if (ind >= 0)
		  {
		      sub = *(dist->Distances + ind);
		      destination = sub->CityTo;
		      cost = min;
		      for (i = 0; i < targets->Count; i++)
			{
			    RouteNodePtr city = *(targets->To + i);
			    if (city == destination)
			      {
				  *(targets->Found + i) = 'Y';
				  break;
			      }
			}
		  }
	    }
	  if (destination == NULL)
	      return 0;
	  *(solution->CitiesFrom + j) = origin;
	  *(solution->CitiesTo + j) = destination;
	  *(solution->Costs + j) = cost;
	  solution->TotalCost += cost;
	  origin = destination;
      }

/* returning to FROM so to close the circular path */
    destination = targets->From;
    for (i = 0; i < ga->Cities; i++)
      {
	  TspGaDistancePtr d = *(ga->Distances + i);
	  if (d->CityFrom == origin)
	    {
		int k;
		dist = *(ga->Distances + i);
		for (k = 0; k < dist->Cities; k++)
		  {
		      sub = *(dist->Distances + k);
		      RouteNodePtr city = sub->CityTo;
		      if (city == destination)
			{
			    cost = sub->Cost;
			    *(solution->CitiesFrom + targets->Count) = origin;
			    *(solution->CitiesTo + targets->Count) =
				destination;
			    *(solution->Costs + targets->Count) = cost;
			    solution->TotalCost += cost;
			}
		  }
	    }
      }

/* inserting into the GA population */
    *(ga->Solutions + index) = solution;
    return 1;
}

static int
cmp_nodes_addr (const void *p1, const void *p2)
{
/* compares two nodes  by addr [for QSORT] */
    TspGaDistancePtr pD1 = *((TspGaDistancePtr *) p1);
    TspGaDistancePtr pD2 = *((TspGaDistancePtr *) p2);
    RouteNodePtr pN1 = pD1->CityFrom;
    RouteNodePtr pN2 = pD2->CityFrom;
    if (pN1 == pN2)
	return 0;
    if (pN1 > pN2)
	return 1;
    return -1;
}

static int
cmp_dist_addr (const void *p1, const void *p2)
{
/* compares two nodes  by addr [for QSORT] */
    TspGaSubDistancePtr pD1 = *((TspGaSubDistancePtr *) p1);
    TspGaSubDistancePtr pD2 = *((TspGaSubDistancePtr *) p2);
    RouteNodePtr pN1 = pD1->CityTo;
    RouteNodePtr pN2 = pD2->CityTo;
    if (pN1 == pN2)
	return 0;
    if (pN1 > pN2)
	return 1;
    return -1;
}

static void
tsp_ga_sort_distances (TspGaPopulationPtr ga)
{
/* sorting Distances/Costs by Node addr */
    int i;
    qsort (ga->Distances, ga->Cities, sizeof (RouteNodePtr), cmp_nodes_addr);
    for (i = 0; i < ga->Cities; i++)
      {
	  TspGaDistancePtr dist = *(ga->Distances + i);
	  qsort (dist->Distances, dist->Cities, sizeof (TspGaSubDistancePtr),
		 cmp_dist_addr);
      }
    for (i = 0; i < ga->Cities; i++)
      {
	  int k;
	  int index = -1;
	  double min = DBL_MAX;
	  TspGaDistancePtr dist = *(ga->Distances + i);
	  for (k = 0; k < dist->Cities; k++)
	    {
		TspGaSubDistancePtr sub = *(dist->Distances + k);
		if (sub->Cost < min)
		  {
		      min = sub->Cost;
		      index = k;
		  }
	    }
	  if (index >= 0)
	      dist->NearestIndex = index;
      }
}

static void
tsp_ga_solve (sqlite3 * handle, int options, RoutingPtr graph,
	      RoutingNodesPtr routing, MultiSolutionPtr multiSolution)
{
/* computing a Dijkstra TSP GA Solution */
    int i;
    int j;
    double min;
    TspGaSolutionPtr bestSolution;
    int count = 0;
    int max_iterations = VROUTE_TSP_GA_MAX_ITERATIONS;
    TspGaPopulationPtr ga = NULL;
    RoutingMultiDestPtr multi;
    TspTargetsPtr targets;
    TspGaDistancePtr dist;

    if (multiSolution == NULL)
	return;
    multi = multiSolution->MultiTo;
    if (multi == NULL)
	return;

/* initialinzing the TSP GA helper struct */
    ga = build_tsp_ga_population (multi->Items + 1);

    for (i = -1; i < multi->Items; i++)
      {
	  /* determining all City-to-City distances (costs) */
	  targets = tsp_ga_permuted_targets (multiSolution->From, multi, i);
	  for (j = 0; j < targets->Count; j++)
	    {
		/* checking for undefined targets */
		if (*(targets->To + j) == NULL)
		  {
		      int k;
		      for (k = 0; k < targets->Count; k++)
			{
			    /* maskinkg unreachable targets */
			    *(targets->Found + k) = 'Y';
			}
		      build_tsp_illegal_solution (multiSolution, targets);
		      destroy_tsp_targets (targets);
		      goto invalid;
		  }
	    }
	  dijkstra_targets_solve (routing, targets);
	  for (j = 0; j < targets->Count; j++)
	    {
		/* checking for unreachable targets */
		if (*(targets->Found + j) != 'Y')
		  {
		      build_tsp_illegal_solution (multiSolution, targets);
		      destroy_tsp_targets (targets);
		      goto invalid;
		  }
	    }
	  /* inserting the distances/costs into the helper struct */
	  dist = alloc_tsp_ga_distances (targets);
	  *(ga->Distances + i + 1) = dist;
	  destroy_tsp_targets (targets);
      }
    tsp_ga_sort_distances (ga);

    for (i = -1; i < multi->Items; i++)
      {
	  /* initializing GA using permuted NN solutions */
	  int ret;
	  targets = tsp_ga_permuted_targets (multiSolution->From, multi, i);
	  ret = build_tsp_nn_solution (ga, targets, i + 1);
	  destroy_tsp_targets (targets);
	  if (!ret)
	      goto invalid;
      }

    while (max_iterations >= 0)
      {
	  /* sexual reproduction and darwinian selection */
	  for (i = 0; i < ga->Count; i++)
	    {
		/* Genetic loop - with mutations */
		TspGaSolutionPtr hybrid;
		int mutation1 = 0;
		int mutation2 = 0;
		count++;
		if (count % 13 == 0)
		  {
		      /* introducing a random mutation on parent #1 */
		      mutation1 = 1;
		  }
		if (count % 16 == 0)
		  {
		      /* introducing a random mutation on parent #2 */
		      mutation2 = 1;
		  }
		hybrid = tsp_ga_crossover (handle, ga, mutation1, mutation2);
		*(ga->Offsprings + i) = hybrid;
	    }
	  evalTspGaFitness (ga);
	  free_tsp_ga_offsprings (ga);
	  max_iterations--;
      }

/* building the TSP GA solution */
    min = DBL_MAX;
    bestSolution = NULL;
    for (i = 0; i < ga->Count; i++)
      {
	  /* searching the best solution */
	  TspGaSolutionPtr old = *(ga->Solutions + i);
	  if (old == NULL)
	      continue;
	  if (old->TotalCost < min)
	    {
		min = old->TotalCost;
		bestSolution = old;
	    }
      }
    if (bestSolution != NULL)
      {
	  targets =
	      build_tsp_ga_solution_targets (multiSolution->MultiTo->Items,
					     multiSolution->From);
	  set_tsp_ga_targets (handle, options, graph, routing, bestSolution,
			      targets);
	  build_tsp_solution (multiSolution, targets);
	  destroy_tsp_targets (targets);
      }
    destroy_tsp_ga_population (ga);
    return;

  invalid:
    destroy_tsp_ga_population (ga);
}

static void
network_free (RoutingPtr p)
{
/* memory cleanup; freeing any allocation for the network struct */
    RouteNodePtr pN;
    int i;
    if (!p)
	return;
    for (i = 0; i < p->NumNodes; i++)
      {
	  pN = p->Nodes + i;
	  if (pN->Code)
	      free (pN->Code);
	  if (pN->Arcs)
	      free (pN->Arcs);
      }
    if (p->Nodes)
	free (p->Nodes);
    if (p->TableName)
	free (p->TableName);
    if (p->FromColumn)
	free (p->FromColumn);
    if (p->ToColumn)
	free (p->ToColumn);
    if (p->GeometryColumn)
	free (p->GeometryColumn);
    if (p->NameColumn)
	free (p->NameColumn);
    free (p);
}

static RoutingPtr
network_init (const unsigned char *blob, int size)
{
/* parsing the HEADER block */
    RoutingPtr graph;
    int net64;
    int aStar = 0;
    int nodes;
    int node_code;
    int max_code_length;
    int endian_arch = gaiaEndianArch ();
    const char *table;
    const char *from;
    const char *to;
    const char *geom;
    const char *name = NULL;
    double a_star_coeff = 1.0;
    int len;
    const unsigned char *ptr;
    if (size < 9)
	return NULL;
    if (*(blob + 0) == GAIA_NET_START)	/* signature - legacy format using 32bit ints */
	net64 = 0;
    else if (*(blob + 0) == GAIA_NET64_START)	/* signature - format using 64bit ints */
	net64 = 1;
    else if (*(blob + 0) == GAIA_NET64_A_STAR_START)	/* signature - format using 64bit ints AND supporting A* */
      {
	  net64 = 1;
	  aStar = 1;
      }
    else
	return NULL;
    if (*(blob + 1) != GAIA_NET_HEADER)	/* signature */
	return NULL;
    nodes = gaiaImport32 (blob + 2, 1, endian_arch);	/* # nodes */
    if (nodes <= 0)
	return NULL;
    if (*(blob + 6) == GAIA_NET_CODE)	/* Nodes identified by a TEXT code */
	node_code = 1;
    else if (*(blob + 6) == GAIA_NET_ID)	/* Nodes indentified by an INTEGER id */
	node_code = 0;
    else
	return NULL;
    max_code_length = *(blob + 7);	/* Max TEXT Code length */
    if (*(blob + 8) != GAIA_NET_TABLE)	/* signature for TABLE NAME */
	return NULL;
    ptr = blob + 9;
    len = gaiaImport16 (ptr, 1, endian_arch);	/* TABLE NAME is varlen */
    ptr += 2;
    table = (char *) ptr;
    ptr += len;
    if (*ptr != GAIA_NET_FROM)	/* signature for FromNode COLUMN */
	return NULL;
    ptr++;
    len = gaiaImport16 (ptr, 1, endian_arch);	/* FromNode COLUMN is varlen */
    ptr += 2;
    from = (char *) ptr;
    ptr += len;
    if (*ptr != GAIA_NET_TO)	/* signature for ToNode COLUMN */
	return NULL;
    ptr++;
    len = gaiaImport16 (ptr, 1, endian_arch);	/* ToNode COLUMN is varlen */
    ptr += 2;
    to = (char *) ptr;
    ptr += len;
    if (*ptr != GAIA_NET_GEOM)	/* signature for Geometry COLUMN */
	return NULL;
    ptr++;
    len = gaiaImport16 (ptr, 1, endian_arch);	/* Geometry COLUMN is varlen */
    ptr += 2;
    geom = (char *) ptr;
    ptr += len;
    if (net64)
      {
	  if (*ptr != GAIA_NET_NAME)	/* signature for Name COLUMN - may be empty */
	      return NULL;
	  ptr++;
	  len = gaiaImport16 (ptr, 1, endian_arch);	/* Name COLUMN is varlen */
	  ptr += 2;
	  name = (char *) ptr;
	  ptr += len;
      }
    if (net64 && aStar)
      {
	  if (*ptr != GAIA_NET_A_STAR_COEFF)	/* signature for A* Heuristic Coeff */
	      return NULL;
	  ptr++;
	  a_star_coeff = gaiaImport64 (ptr, 1, endian_arch);
	  ptr += 8;
      }
    if (*ptr != GAIA_NET_END)	/* signature */
	return NULL;
    graph = malloc (sizeof (Routing));
    graph->Net64 = net64;
    graph->AStar = aStar;
    graph->EndianArch = endian_arch;
    graph->CurrentIndex = 0;
    graph->NodeCode = node_code;
    graph->MaxCodeLength = max_code_length;
    graph->NumNodes = nodes;
    graph->Nodes = malloc (sizeof (RouteNode) * nodes);
    len = strlen (table);
    graph->TableName = malloc (len + 1);
    strcpy (graph->TableName, table);
    len = strlen (from);
    graph->FromColumn = malloc (len + 1);
    strcpy (graph->FromColumn, from);
    len = strlen (to);
    graph->ToColumn = malloc (len + 1);
    strcpy (graph->ToColumn, to);
    len = strlen (geom);
    if (len <= 1)
	graph->GeometryColumn = NULL;
    else
      {
	  graph->GeometryColumn = malloc (len + 1);
	  strcpy (graph->GeometryColumn, geom);
      }
    if (!net64)
      {
	  /* Name column is not supported */
	  graph->NameColumn = NULL;
      }
    else
      {
	  len = strlen (name);
	  if (len <= 1)
	      graph->NameColumn = NULL;
	  else
	    {
		graph->NameColumn = malloc (len + 1);
		strcpy (graph->NameColumn, name);
	    }
      }
    graph->AStarHeuristicCoeff = a_star_coeff;
    return graph;
}

static int
network_block (RoutingPtr graph, const unsigned char *blob, int size)
{
/* parsing a NETWORK Block */
    const unsigned char *in = blob;
    int nodes;
    int i;
    int ia;
    int index;
    char code[256];
    double x;
    double y;
    sqlite3_int64 nodeId = -1;
    int arcs;
    RouteNodePtr pN;
    RouteArcPtr pA;
    int len;
    sqlite3_int64 arcId;
    int nodeToIdx;
    double cost;
    if (size < 3)
	goto error;
    if (*in++ != GAIA_NET_BLOCK)	/* signature */
	goto error;
    nodes = gaiaImport16 (in, 1, graph->EndianArch);	/* # Nodes */
    in += 2;
    for (i = 0; i < nodes; i++)
      {
	  /* parsing each node */
	  if ((size - (in - blob)) < 5)
	      goto error;
	  if (*in++ != GAIA_NET_NODE)	/* signature */
	      goto error;
	  index = gaiaImport32 (in, 1, graph->EndianArch);	/* node internal index */
	  in += 4;
	  if (index < 0 || index >= graph->NumNodes)
	      goto error;
	  if (graph->NodeCode)
	    {
		/* Nodes are identified by a TEXT Code */
		if ((size - (in - blob)) < graph->MaxCodeLength)
		    goto error;
		memcpy (code, in, graph->MaxCodeLength);
		in += graph->MaxCodeLength;
	    }
	  else
	    {
		/* Nodes are identified by an INTEGER Id */
		if (graph->Net64)
		  {
		      if ((size - (in - blob)) < 8)
			  goto error;
		      nodeId = gaiaImportI64 (in, 1, graph->EndianArch);	/* the Node ID: 64bit */
		      in += 8;
		  }
		else
		  {
		      if ((size - (in - blob)) < 4)
			  goto error;
		      nodeId = gaiaImport32 (in, 1, graph->EndianArch);	/* the Node ID: 32bit */
		      in += 4;
		  }
	    }
	  if (graph->AStar)
	    {
		/* fetching node's X,Y coords */
		if ((size - (in - blob)) < 8)
		    goto error;
		x = gaiaImport64 (in, 1, graph->EndianArch);	/* X coord */
		in += 8;
		if ((size - (in - blob)) < 8)
		    goto error;
		y = gaiaImport64 (in, 1, graph->EndianArch);	/* Y coord */
		in += 8;
	    }
	  else
	    {
		x = DBL_MAX;
		y = DBL_MAX;
	    }
	  if ((size - (in - blob)) < 2)
	      goto error;
	  arcs = gaiaImport16 (in, 1, graph->EndianArch);	/* # Arcs */
	  in += 2;
	  if (arcs < 0)
	      goto error;
	  /* initializing the Node */
	  pN = graph->Nodes + index;
	  pN->InternalIndex = index;
	  if (graph->NodeCode)
	    {
		/* Nodes are identified by a TEXT Code */
		pN->Id = -1;
		len = strlen (code);
		pN->Code = malloc (len + 1);
		strcpy (pN->Code, code);
	    }
	  else
	    {
		/* Nodes are identified by an INTEGER Id */
		pN->Id = nodeId;
		pN->Code = NULL;
	    }
	  pN->CoordX = x;
	  pN->CoordY = y;
	  pN->NumArcs = arcs;
	  if (arcs)
	    {
		/* parsing the Arcs */
		pN->Arcs = malloc (sizeof (RouteArc) * arcs);
		for (ia = 0; ia < arcs; ia++)
		  {
		      /* parsing each Arc */
		      if (graph->Net64)
			{
			    if ((size - (in - blob)) < 22)
				goto error;
			}
		      else
			{
			    if ((size - (in - blob)) < 18)
				goto error;
			}
		      if (*in++ != GAIA_NET_ARC)	/* signature */
			  goto error;
		      if (graph->Net64)
			{
			    arcId = gaiaImportI64 (in, 1, graph->EndianArch);	/* # Arc ROWID: 64bit */
			    in += 8;
			}
		      else
			{
			    arcId = gaiaImport32 (in, 1, graph->EndianArch);	/* # Arc ROWID: 32bit */
			    in += 4;
			}
		      nodeToIdx = gaiaImport32 (in, 1, graph->EndianArch);	/* # NodeTo internal index */
		      in += 4;
		      cost = gaiaImport64 (in, 1, graph->EndianArch);	/* # Cost */
		      in += 8;
		      if (*in++ != GAIA_NET_END)	/* signature */
			  goto error;
		      pA = pN->Arcs + ia;
		      /* initializing the Arc */
		      if (nodeToIdx < 0 || nodeToIdx >= graph->NumNodes)
			  goto error;
		      pA->NodeFrom = pN;
		      pA->NodeTo = graph->Nodes + nodeToIdx;
		      pA->ArcRowid = arcId;
		      pA->Cost = cost;
		  }
	    }
	  else
	      pN->Arcs = NULL;
	  if ((size - (in - blob)) < 1)
	      goto error;
	  if (*in++ != GAIA_NET_END)	/* signature */
	      goto error;
      }
    return 1;
  error:
    return 0;
}

static RoutingPtr
load_network (sqlite3 * handle, const char *table)
{
/* loads the NETWORK struct */
    RoutingPtr graph = NULL;
    sqlite3_stmt *stmt;
    char *sql;
    int ret;
    int header = 1;
    const unsigned char *blob;
    int size;
    char *xname;
    xname = gaiaDoubleQuotedSql (table);
    sql = sqlite3_mprintf ("SELECT NetworkData FROM \"%s\" ORDER BY Id", xname);
    free (xname);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	goto abort;
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		if (sqlite3_column_type (stmt, 0) == SQLITE_BLOB)
		  {
		      blob =
			  (const unsigned char *) sqlite3_column_blob (stmt, 0);
		      size = sqlite3_column_bytes (stmt, 0);
		      if (header)
			{
			    /* parsing the HEADER block */
			    graph = network_init (blob, size);
			    header = 0;
			}
		      else
			{
			    /* parsing ordinary Blocks */
			    if (!graph)
			      {
				  sqlite3_finalize (stmt);
				  goto abort;
			      }
			    if (!network_block (graph, blob, size))
			      {
				  sqlite3_finalize (stmt);
				  goto abort;
			      }
			}
		  }
		else
		  {
		      sqlite3_finalize (stmt);
		      goto abort;
		  }
	    }
	  else
	    {
		sqlite3_finalize (stmt);
		goto abort;
	    }
      }
    sqlite3_finalize (stmt);
    return graph;
  abort:
    network_free (graph);
    return NULL;
}

static int
vroute_create (sqlite3 * db, void *pAux, int argc, const char *const *argv,
	       sqlite3_vtab ** ppVTab, char **pzErr)
{
/* creates the virtual table connected to some shapefile */
    virtualroutingPtr p_vt;
    int err;
    int ret;
    int i;
    int n_rows;
    int n_columns;
    char *vtable = NULL;
    char *table = NULL;
    const char *col_name = NULL;
    char **results;
    char *err_msg = NULL;
    char *sql;
    int ok_id;
    int ok_data;
    char *xname;
    RoutingPtr graph = NULL;
    if (pAux)
	pAux = pAux;		/* unused arg warning suppression */
/* checking for table_name and geo_column_name */
    if (argc == 4)
      {
	  vtable = gaiaDequotedSql (argv[2]);
	  table = gaiaDequotedSql (argv[3]);
      }
    else
      {
	  *pzErr =
	      sqlite3_mprintf
	      ("[virtualrouting module] CREATE VIRTUAL: illegal arg list {NETWORK-DATAtable}\n");
	  goto error;
      }
/* retrieving the base table columns */
    err = 0;
    ok_id = 0;
    ok_data = 0;
    xname = gaiaDoubleQuotedSql (table);
    sql = sqlite3_mprintf ("PRAGMA table_info(\"%s\")", xname);
    free (xname);
    ret = sqlite3_get_table (db, sql, &results, &n_rows, &n_columns, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  err = 1;
	  goto illegal;
      }
    if (n_rows > 1)
      {
	  for (i = 1; i <= n_rows; i++)
	    {
		col_name = results[(i * n_columns) + 1];
		if (strcasecmp (col_name, "id") == 0)
		    ok_id = 1;
		if (strcasecmp (col_name, "networkdata") == 0)
		    ok_data = 1;
	    }
	  sqlite3_free_table (results);
	  if (!ok_id)
	      err = 1;
	  if (!ok_data)
	      err = 1;
      }
    else
	err = 1;
  illegal:
    if (err)
      {
	  /* something is going the wrong way */
	  *pzErr =
	      sqlite3_mprintf
	      ("[virtualrouting module] cannot build a valid NETWORK\n");
	  return SQLITE_ERROR;
      }
    p_vt = (virtualroutingPtr) sqlite3_malloc (sizeof (virtualrouting));
    if (!p_vt)
	return SQLITE_NOMEM;
    graph = load_network (db, table);
    if (!graph)
      {
	  /* something is going the wrong way */
	  *pzErr =
	      sqlite3_mprintf
	      ("[virtualrouting module] cannot build a valid NETWORK\n");
	  goto error;
      }
    p_vt->db = db;
    p_vt->graph = graph;
    p_vt->currentAlgorithm = VROUTE_DIJKSTRA_ALGORITHM;
    p_vt->currentRequest = VROUTE_SHORTEST_PATH;
    p_vt->currentOptions = VROUTE_SHORTEST_PATH_FULL;
    p_vt->currentDelimiter = ',';
    p_vt->routing = NULL;
    p_vt->pModule = &my_route_module;
    p_vt->nRef = 0;
    p_vt->zErrMsg = NULL;
/* preparing the COLUMNs for this VIRTUAL TABLE */
    xname = gaiaDoubleQuotedSql (vtable);
    if (p_vt->graph->NodeCode)
      {
	  if (p_vt->graph->NameColumn)
	    {
		sql = sqlite3_mprintf ("CREATE TABLE \"%s\" (Algorithm TEXT, "
				       "Request TEXT, Options TEXT, Delimiter TEXT, "
				       "RouteId INTEGER, RouteRow INTEGER, Role TEXT, "
				       "ArcRowid INTEGER, NodeFrom TEXT, NodeTo TEXT,"
				       " Cost DOUBLE, Geometry BLOB, Name TEXT)",
				       xname);
	    }
	  else
	    {
		sql = sqlite3_mprintf ("CREATE TABLE \"%s\" (Algorithm TEXT, "
				       "Request TEXT, Options TEXT, Delimiter TEXT, "
				       "RouteId INTEGER, RouteRow INTEGER, Role TEXT, "
				       "ArcRowid INTEGER, NodeFrom TEXT, NodeTo TEXT,"
				       " Cost DOUBLE, Geometry BLOB)", xname);
	    }
      }
    else
      {
	  if (p_vt->graph->NameColumn)
	    {
		sql = sqlite3_mprintf ("CREATE TABLE \"%s\" (Algorithm TEXT, "
				       "Request TEXT, Options TEXT, Delimiter TEXT, "
				       "RouteId INTEGER, RouteRow INTEGER, Role TEXT, "
				       "ArcRowid INTEGER, NodeFrom INTEGER, NodeTo INTEGER,"
				       " Cost DOUBLE, Geometry BLOB, Name TEXT)",
				       xname);
	    }
	  else
	    {
		sql = sqlite3_mprintf ("CREATE TABLE \"%s\" (Algorithm TEXT, "
				       "Request TEXT, Options TEXT, Delimiter TEXT, "
				       "RouteId INTEGER, RouteRow INTEGER, Role TEXT, "
				       "ArcRowid INTEGER, NodeFrom INTEGER, NodeTo INTEGER,"
				       " Cost DOUBLE, Geometry BLOB)", xname);
	    }
      }
    free (xname);
    if (sqlite3_declare_vtab (db, sql) != SQLITE_OK)
      {
	  *pzErr =
	      sqlite3_mprintf
	      ("[virtualrouting module] CREATE VIRTUAL: invalid SQL statement \"%s\"",
	       sql);
	  sqlite3_free (sql);
	  goto error;
      }
    sqlite3_free (sql);
    *ppVTab = (sqlite3_vtab *) p_vt;
    p_vt->routing = routing_init (p_vt->graph);
    free (table);
    free (vtable);
    return SQLITE_OK;
  error:
    if (table)
	free (table);
    if (vtable)
	free (vtable);
    return SQLITE_ERROR;
}

static int
vroute_connect (sqlite3 * db, void *pAux, int argc, const char *const *argv,
		sqlite3_vtab ** ppVTab, char **pzErr)
{
/* connects the virtual table to some shapefile - simply aliases vshp_create() */
    return vroute_create (db, pAux, argc, argv, ppVTab, pzErr);
}

static int
vroute_best_index (sqlite3_vtab * pVTab, sqlite3_index_info * pIdxInfo)
{
/* best index selection */
    int i;
    int errors = 0;
    int err = 1;
    int from = 0;
    int to = 0;
    int cost = 0;
    int i_from = -1;
    int i_to = -1;
    int i_cost = -1;
    if (pVTab)
	pVTab = pVTab;		/* unused arg warning suppression */
    for (i = 0; i < pIdxInfo->nConstraint; i++)
      {
	  /* verifying the constraints */
	  struct sqlite3_index_constraint *p = &(pIdxInfo->aConstraint[i]);
	  if (p->usable)
	    {
		if (p->iColumn == 8 && p->op == SQLITE_INDEX_CONSTRAINT_EQ)
		  {
		      from++;
		      i_from = i;
		  }
		else if (p->iColumn == 9 && p->op == SQLITE_INDEX_CONSTRAINT_EQ)
		  {
		      to++;
		      i_to = i;
		  }
		else if (p->iColumn == 10
			 && p->op == SQLITE_INDEX_CONSTRAINT_LE)
		  {
		      cost++;
		      i_cost = i;
		  }
		else
		    errors++;
	    }
      }
    if (from == 1 && to == 1 && errors == 0)
      {
	  /* this one is a valid Shortest Path query */
	  if (i_from < i_to)
	      pIdxInfo->idxNum = 1;	/* first arg is FROM */
	  else
	      pIdxInfo->idxNum = 2;	/* first arg is TO */
	  pIdxInfo->estimatedCost = 1.0;
	  for (i = 0; i < pIdxInfo->nConstraint; i++)
	    {
		if (pIdxInfo->aConstraint[i].usable)
		  {
		      pIdxInfo->aConstraintUsage[i].argvIndex = i + 1;
		      pIdxInfo->aConstraintUsage[i].omit = 1;
		  }
	    }
	  err = 0;
      }
    if (from == 1 && cost == 1 && errors == 0)
      {
	  /* this one is a valid "within cost" query */
	  if (i_from < i_cost)
	      pIdxInfo->idxNum = 3;	/* first arg is FROM */
	  else
	      pIdxInfo->idxNum = 4;	/* first arg is COST */
	  pIdxInfo->estimatedCost = 1.0;
	  for (i = 0; i < pIdxInfo->nConstraint; i++)
	    {
		if (pIdxInfo->aConstraint[i].usable)
		  {
		      pIdxInfo->aConstraintUsage[i].argvIndex = i + 1;
		      pIdxInfo->aConstraintUsage[i].omit = 1;
		  }
	    }
	  err = 0;
      }
    if (err)
      {
	  /* illegal query */
	  pIdxInfo->idxNum = 0;
      }
    return SQLITE_OK;
}

static int
vroute_disconnect (sqlite3_vtab * pVTab)
{
/* disconnects the virtual table */
    virtualroutingPtr p_vt = (virtualroutingPtr) pVTab;
    if (p_vt->routing)
	routing_free (p_vt->routing);
    if (p_vt->graph)
	network_free (p_vt->graph);
    sqlite3_free (p_vt);
    return SQLITE_OK;
}

static int
vroute_destroy (sqlite3_vtab * pVTab)
{
/* destroys the virtual table - simply aliases vshp_disconnect() */
    return vroute_disconnect (pVTab);
}

static void
vroute_read_row (virtualroutingCursorPtr cursor)
{
/* trying to read a "row" from Shortest Path solution */
    if (cursor->pVtab->multiSolution->Mode == VROUTE_RANGE_SOLUTION)
      {
	  if (cursor->pVtab->multiSolution->CurrentNodeRow == NULL)
	      cursor->pVtab->eof = 1;
	  else
	      cursor->pVtab->eof = 0;
      }
    else
      {
	  if (cursor->pVtab->multiSolution->CurrentRow == NULL)
	      cursor->pVtab->eof = 1;
	  else
	      cursor->pVtab->eof = 0;
      }
    return;
}

static int
vroute_open (sqlite3_vtab * pVTab, sqlite3_vtab_cursor ** ppCursor)
{
/* opening a new cursor */
    virtualroutingCursorPtr cursor =
	(virtualroutingCursorPtr)
	sqlite3_malloc (sizeof (virtualroutingCursor));
    if (cursor == NULL)
	return SQLITE_ERROR;
    cursor->pVtab = (virtualroutingPtr) pVTab;
    cursor->pVtab->multiSolution = alloc_multiSolution ();
    cursor->pVtab->eof = 0;
    *ppCursor = (sqlite3_vtab_cursor *) cursor;
    return SQLITE_OK;
}

static int
vroute_close (sqlite3_vtab_cursor * pCursor)
{
/* closing the cursor */
    virtualroutingCursorPtr cursor = (virtualroutingCursorPtr) pCursor;
    delete_multiSolution (cursor->pVtab->multiSolution);
    sqlite3_free (pCursor);
    return SQLITE_OK;
}

static void
set_multi_by_id (RoutingMultiDestPtr multiple, RoutingPtr graph)
{
// setting Node pointers to multiple destinations */
    int i;
    for (i = 0; i < multiple->Items; i++)
      {
	  sqlite3_int64 id = *(multiple->Ids + i);
	  if (id >= 1)
	      *(multiple->To + i) = find_node_by_id (graph, id);
      }
}

static void
set_multi_by_code (RoutingMultiDestPtr multiple, RoutingPtr graph)
{
// setting Node pointers to multiple destinations */
    int i;
    for (i = 0; i < multiple->Items; i++)
      {
	  const char *code = *(multiple->Codes + i);
	  if (code != NULL)
	      *(multiple->To + i) = find_node_by_code (graph, code);
      }
}

static int
vroute_filter (sqlite3_vtab_cursor * pCursor, int idxNum, const char *idxStr,
	       int argc, sqlite3_value ** argv)
{
/* setting up a cursor filter */
    int node_code = 0;
    virtualroutingCursorPtr cursor = (virtualroutingCursorPtr) pCursor;
    virtualroutingPtr net = (virtualroutingPtr) cursor->pVtab;
    RoutingMultiDestPtr multiple = NULL;
    if (idxStr)
	idxStr = idxStr;	/* unused arg warning suppression */
    node_code = net->graph->NodeCode;
    reset_multiSolution (cursor->pVtab->multiSolution);
    cursor->pVtab->eof = 0;
    if (idxNum == 1 && argc == 2)
      {
	  /* retrieving the Shortest Path From/To params */
	  if (node_code)
	    {
		/* Nodes are identified by TEXT Codes */
		if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
		    cursor->pVtab->multiSolution->From =
			find_node_by_code (net->graph,
					   (char *)
					   sqlite3_value_text (argv[0]));
		if (sqlite3_value_type (argv[1]) == SQLITE_TEXT)
		  {
		      multiple =
			  vroute_get_multiple_destinations (1,
							    cursor->pVtab->
							    currentDelimiter,
							    (const char *)
							    sqlite3_value_text
							    (argv[1]));
		      if (multiple != NULL)
			{
			    set_multi_by_code (multiple, net->graph);
			    cursor->pVtab->multiSolution->MultiTo = multiple;
			}
		  }
	    }
	  else
	    {
		/* Nodes are identified by INT Ids */
		if (sqlite3_value_type (argv[0]) == SQLITE_INTEGER)
		    cursor->pVtab->multiSolution->From =
			find_node_by_id (net->graph,
					 sqlite3_value_int (argv[0]));
		if (sqlite3_value_type (argv[1]) == SQLITE_TEXT)
		  {
		      multiple =
			  vroute_get_multiple_destinations (0,
							    cursor->pVtab->
							    currentDelimiter,
							    (const char *)
							    sqlite3_value_text
							    (argv[1]));
		      if (multiple != NULL)
			{
			    set_multi_by_id (multiple, net->graph);
			    cursor->pVtab->multiSolution->MultiTo = multiple;
			}
		  }
		else if (sqlite3_value_type (argv[1]) == SQLITE_INTEGER)
		  {
		      multiple =
			  vroute_as_multiple_destinations (sqlite3_value_int
							   (argv[1]));
		      if (multiple != NULL)
			{
			    set_multi_by_id (multiple, net->graph);
			    cursor->pVtab->multiSolution->MultiTo = multiple;
			}
		  }
	    }
      }
    if (idxNum == 2 && argc == 2)
      {
	  /* retrieving the Shortest Path To/From params */
	  if (node_code)
	    {
		/* Nodes are identified by TEXT Codes */
		if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
		  {
		      multiple =
			  vroute_get_multiple_destinations (1,
							    cursor->pVtab->
							    currentDelimiter,
							    (const char *)
							    sqlite3_value_text
							    (argv[0]));
		      if (multiple != NULL)
			{
			    set_multi_by_code (multiple, net->graph);
			    cursor->pVtab->multiSolution->MultiTo = multiple;
			}
		  }
		if (sqlite3_value_type (argv[1]) == SQLITE_TEXT)
		  {
		      cursor->pVtab->multiSolution->From =
			  find_node_by_code (net->graph,
					     (char *)
					     sqlite3_value_text (argv[1]));
		  }
	    }
	  else
	    {
		/* Nodes are identified by INT Ids */
		if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
		  {
		      multiple =
			  vroute_get_multiple_destinations (0,
							    cursor->pVtab->
							    currentDelimiter,
							    (const char *)
							    sqlite3_value_text
							    (argv[0]));
		      if (multiple != NULL)
			{
			    set_multi_by_id (multiple, net->graph);
			    cursor->pVtab->multiSolution->MultiTo = multiple;
			}
		  }
		else if (sqlite3_value_type (argv[0]) == SQLITE_INTEGER)
		  {
		      multiple =
			  vroute_as_multiple_destinations (sqlite3_value_int
							   (argv[0]));
		      if (multiple != NULL)
			{
			    set_multi_by_id (multiple, net->graph);
			    cursor->pVtab->multiSolution->MultiTo = multiple;
			}
		  }
		if (sqlite3_value_type (argv[1]) == SQLITE_INTEGER)
		    cursor->pVtab->multiSolution->From =
			find_node_by_id (net->graph,
					 sqlite3_value_int (argv[1]));
	    }
      }
    if (idxNum == 3 && argc == 2)
      {
	  /* retrieving the From and Cost param */
	  if (node_code)
	    {
		/* Nodes are identified by TEXT Codes */
		if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
		    cursor->pVtab->multiSolution->From =
			find_node_by_code (net->graph,
					   (char *)
					   sqlite3_value_text (argv[0]));
	    }
	  else
	    {
		/* Nodes are identified by INT Ids */
		if (sqlite3_value_type (argv[0]) == SQLITE_INTEGER)
		    cursor->pVtab->multiSolution->From =
			find_node_by_id (net->graph,
					 sqlite3_value_int (argv[0]));
	    }
	  if (sqlite3_value_type (argv[1]) == SQLITE_INTEGER)
	    {
		int cost = sqlite3_value_int (argv[1]);
		cursor->pVtab->multiSolution->MaxCost = cost;
	    }
	  else if (sqlite3_value_type (argv[1]) == SQLITE_FLOAT)
	      cursor->pVtab->multiSolution->MaxCost =
		  sqlite3_value_double (argv[1]);
      }
    if (idxNum == 4 && argc == 2)
      {
	  /* retrieving the From and Cost param */
	  if (node_code)
	    {
		/* Nodes are identified by TEXT Codes */
		if (sqlite3_value_type (argv[1]) == SQLITE_TEXT)
		    cursor->pVtab->multiSolution->From =
			find_node_by_code (net->graph,
					   (char *)
					   sqlite3_value_text (argv[1]));
	    }
	  else
	    {
		/* Nodes are identified by INT Ids */
		if (sqlite3_value_type (argv[1]) == SQLITE_INTEGER)
		    cursor->pVtab->multiSolution->From =
			find_node_by_id (net->graph,
					 sqlite3_value_int (argv[1]));
	    }
	  if (sqlite3_value_type (argv[0]) == SQLITE_INTEGER)
	    {
		int cost = sqlite3_value_int (argv[0]);
		cursor->pVtab->multiSolution->MaxCost = cost;
	    }
	  else if (sqlite3_value_type (argv[0]) == SQLITE_FLOAT)
	      cursor->pVtab->multiSolution->MaxCost =
		  sqlite3_value_double (argv[0]);
      }
    if (cursor->pVtab->multiSolution == NULL)
      {
	  cursor->pVtab->eof = 0;
	  cursor->pVtab->multiSolution->CurrentRow = NULL;
	  cursor->pVtab->multiSolution->CurrentRowId = 0;
	  cursor->pVtab->multiSolution->Mode = VROUTE_ROUTING_SOLUTION;
	  return SQLITE_OK;
      }
    if (cursor->pVtab->multiSolution->From
	&& cursor->pVtab->multiSolution->MultiTo)
      {
	  cursor->pVtab->eof = 0;
	  if (net->currentRequest == VROUTE_TSP_NN)
	    {
		cursor->pVtab->multiSolution->Mode = VROUTE_TSP_SOLUTION;
		if (net->currentAlgorithm == VROUTE_DIJKSTRA_ALGORITHM)
		  {
		      tsp_nn_solve (net->db, net->currentOptions, net->graph,
				    net->routing, cursor->pVtab->multiSolution);
		      cursor->pVtab->multiSolution->CurrentRowId = 0;
		      cursor->pVtab->multiSolution->CurrentRow =
			  cursor->pVtab->multiSolution->FirstRow;
		  }
	    }
	  else if (net->currentRequest == VROUTE_TSP_GA)
	    {
		cursor->pVtab->multiSolution->Mode = VROUTE_TSP_SOLUTION;
		if (net->currentAlgorithm == VROUTE_DIJKSTRA_ALGORITHM)
		  {
		      tsp_ga_solve (net->db, net->currentOptions, net->graph,
				    net->routing, cursor->pVtab->multiSolution);
		      cursor->pVtab->multiSolution->CurrentRowId = 0;
		      cursor->pVtab->multiSolution->CurrentRow =
			  cursor->pVtab->multiSolution->FirstRow;
		  }
	    }
	  else
	    {
		cursor->pVtab->multiSolution->Mode = VROUTE_ROUTING_SOLUTION;
		if (net->currentAlgorithm == VROUTE_A_STAR_ALGORITHM)
		    astar_solve (net->db, net->currentOptions, net->graph,
				 net->routing, cursor->pVtab->multiSolution);
		else
		    dijkstra_multi_solve (net->db, net->currentOptions,
					  net->graph, net->routing,
					  cursor->pVtab->multiSolution);
		cursor->pVtab->multiSolution->CurrentRowId = 0;
		cursor->pVtab->multiSolution->CurrentRow =
		    cursor->pVtab->multiSolution->FirstRow;
	    }
	  return SQLITE_OK;
      }
    if (cursor->pVtab->multiSolution->From
	&& cursor->pVtab->multiSolution->MaxCost > 0.0)
      {
	  int srid = find_srid (net->db, net->graph);
	  cursor->pVtab->eof = 0;
	  cursor->pVtab->multiSolution->Mode = VROUTE_RANGE_SOLUTION;
	  if (net->currentAlgorithm == VROUTE_DIJKSTRA_ALGORITHM)
	    {
		dijkstra_within_cost_range (net->routing,
					    cursor->pVtab->multiSolution, srid);
		cursor->pVtab->multiSolution->CurrentRowId = 0;
		cursor->pVtab->multiSolution->CurrentNodeRow =
		    cursor->pVtab->multiSolution->FirstNode;
	    }
	  return SQLITE_OK;
      }
    cursor->pVtab->eof = 0;
    cursor->pVtab->multiSolution->CurrentRow = NULL;
    cursor->pVtab->multiSolution->CurrentRowId = 0;
    cursor->pVtab->multiSolution->Mode = VROUTE_ROUTING_SOLUTION;
    return SQLITE_OK;
}

static int
vroute_next (sqlite3_vtab_cursor * pCursor)
{
/* fetching a next row from cursor */
    virtualroutingCursorPtr cursor = (virtualroutingCursorPtr) pCursor;
    if (cursor->pVtab->multiSolution->Mode == VROUTE_RANGE_SOLUTION)
      {
	  cursor->pVtab->multiSolution->CurrentNodeRow =
	      cursor->pVtab->multiSolution->CurrentNodeRow->Next;
	  if (!(cursor->pVtab->multiSolution->CurrentNodeRow))
	    {
		cursor->pVtab->eof = 1;
		return SQLITE_OK;
	    }
      }
    else
      {
	  if (cursor->pVtab->multiSolution->CurrentRow == NULL)
	    {
		cursor->pVtab->eof = 1;
		return SQLITE_OK;
	    }
	  cursor->pVtab->multiSolution->CurrentRow =
	      cursor->pVtab->multiSolution->CurrentRow->Next;
	  if (!(cursor->pVtab->multiSolution->CurrentRow))
	    {
		cursor->pVtab->eof = 1;
		return SQLITE_OK;
	    }
      }
    (cursor->pVtab->multiSolution->CurrentRowId)++;
    vroute_read_row (cursor);
    return SQLITE_OK;
}

static int
vroute_eof (sqlite3_vtab_cursor * pCursor)
{
/* cursor EOF */
    virtualroutingCursorPtr cursor = (virtualroutingCursorPtr) pCursor;
    return cursor->pVtab->eof;
}

static int
vroute_column (sqlite3_vtab_cursor * pCursor, sqlite3_context * pContext,
	       int column)
{
/* fetching value for the Nth column */
    ResultsetRowPtr row;
    RowNodeSolutionPtr row_node;
    int node_code = 0;
    const char *algorithm;
    char delimiter[128];
    const char *role;
    virtualroutingCursorPtr cursor = (virtualroutingCursorPtr) pCursor;
    virtualroutingPtr net = (virtualroutingPtr) cursor->pVtab;
    node_code = net->graph->NodeCode;
    if (cursor->pVtab->multiSolution->Mode == VROUTE_RANGE_SOLUTION)
      {
	  /* processing "within Cost range" solution */
	  row_node = cursor->pVtab->multiSolution->CurrentNodeRow;
	  if (column == 0)
	    {
		/* the currently used Algorithm */
		algorithm = "Dijkstra";
		sqlite3_result_text (pContext, algorithm, strlen (algorithm),
				     SQLITE_TRANSIENT);
	    }
	  if (column == 1)
	    {
		/* the current Request type */
		algorithm = "Isochrone";
		sqlite3_result_text (pContext, algorithm, strlen (algorithm),
				     SQLITE_TRANSIENT);
	    }
	  if (column == 2)
	    {
		/* the currently set Options */
		algorithm = "Full";
		sqlite3_result_text (pContext, algorithm,
				     strlen (algorithm), SQLITE_TRANSIENT);
	    }
	  if (column == 3)
	    {
		/* the currently set delimiter char */
		if (isprint (cursor->pVtab->currentDelimiter))
		    sprintf (delimiter, "%c [dec=%d, hex=%02x]",
			     cursor->pVtab->currentDelimiter,
			     cursor->pVtab->currentDelimiter,
			     cursor->pVtab->currentDelimiter);
		else
		    sprintf (delimiter, "[dec=%d, hex=%02x]",
			     cursor->pVtab->currentDelimiter,
			     cursor->pVtab->currentDelimiter);
		sqlite3_result_text (pContext, delimiter, strlen (delimiter),
				     SQLITE_TRANSIENT);
	    }
	  if (column == 4)
	    {
		/* the RouteNum column */
		sqlite3_result_int (pContext, 0);
	    }
	  if (column == 5)
	    {
		/* the RouteRow column */
		sqlite3_result_int (pContext, 0);
	    }
	  if (column == 6)
	    {
		/* role of this row */
		role = "Solution";
		sqlite3_result_text (pContext, role, strlen (role),
				     SQLITE_TRANSIENT);
	    }
	  if (column == 7)
	    {
		/* the ArcRowId column */
		sqlite3_result_null (pContext);
	    }
	  if (column == 8)
	    {
		/* the NodeFrom column */
		if (node_code)
		    sqlite3_result_text (pContext,
					 cursor->pVtab->multiSolution->
					 From->Code,
					 strlen (cursor->pVtab->
						 multiSolution->From->Code),
					 SQLITE_STATIC);
		else
		    sqlite3_result_int64 (pContext,
					  cursor->pVtab->multiSolution->
					  From->Id);
	    }
	  if (column == 9)
	    {
		/* the NodeTo column */
		if (node_code)
		    sqlite3_result_text (pContext, row_node->Node->Code,
					 strlen (row_node->Node->Code),
					 SQLITE_STATIC);
		else
		    sqlite3_result_int64 (pContext, row_node->Node->Id);
	    }
	  if (column == 10)
	    {
		/* the Cost column */
		sqlite3_result_double (pContext, row_node->Cost);
	    }
	  if (column == 11)
	    {
		/* the Geometry column */
		if (row_node->Srid == VROUTE_INVALID_SRID)
		    sqlite3_result_null (pContext);
		else
		  {
		      int len;
		      unsigned char *p_result = NULL;
		      gaiaGeomCollPtr geom = gaiaAllocGeomColl ();
		      geom->Srid = row_node->Srid;
		      gaiaAddPointToGeomColl (geom, row_node->Node->CoordX,
					      row_node->Node->CoordY);
		      gaiaToSpatiaLiteBlobWkb (geom, &p_result, &len);
		      sqlite3_result_blob (pContext, p_result, len, free);
		      gaiaFreeGeomColl (geom);
		  }
	    }
	  if (column == 12)
	    {
		/* the [optional] Name column */
		sqlite3_result_null (pContext);
	    }
      }
    else
      {
	  /* processing an ordinary Routing (Shortest Path or TSP) solution */
	  row = cursor->pVtab->multiSolution->CurrentRow;
	  if (row == NULL)
	    {
		/* empty resultset */
		if (column == 0)
		  {
		      /* the currently used Algorithm */
		      if (net->currentAlgorithm == VROUTE_A_STAR_ALGORITHM)
			  algorithm = "A*";
		      else
			  algorithm = "Dijkstra";
		      sqlite3_result_text (pContext, algorithm,
					   strlen (algorithm),
					   SQLITE_TRANSIENT);
		  }
		else if (column == 1)
		  {
		      /* the current Request type */
		      if (net->currentRequest == VROUTE_TSP_NN)
			  algorithm = "TSP NN";
		      else if (net->currentRequest == VROUTE_TSP_GA)
			  algorithm = "TSP GA";
		      else
			  algorithm = "Shortest Path";
		      sqlite3_result_text (pContext, algorithm,
					   strlen (algorithm),
					   SQLITE_TRANSIENT);
		  }
		else if (column == 2)
		  {
		      /* the currently set Options */
		      if (net->currentOptions == VROUTE_SHORTEST_PATH_SIMPLE)
			  algorithm = "Simple";
		      else if (net->currentOptions ==
			       VROUTE_SHORTEST_PATH_NO_ARCS)
			  algorithm = "No Arcs";
		      else if (net->currentOptions ==
			       VROUTE_SHORTEST_PATH_NO_GEOMS)
			  algorithm = "No Geometries";
		      else
			  algorithm = "Full";
		      sqlite3_result_text (pContext, algorithm,
					   strlen (algorithm),
					   SQLITE_TRANSIENT);
		  }
		else if (column == 3)
		  {
		      /* the currently set delimiter char */
		      if (isprint (cursor->pVtab->currentDelimiter))
			  sprintf (delimiter, "%c [dec=%d, hex=%02x]",
				   cursor->pVtab->currentDelimiter,
				   cursor->pVtab->currentDelimiter,
				   cursor->pVtab->currentDelimiter);
		      else
			  sprintf (delimiter, "[dec=%d, hex=%02x]",
				   cursor->pVtab->currentDelimiter,
				   cursor->pVtab->currentDelimiter);
		      sqlite3_result_text (pContext, delimiter,
					   strlen (delimiter),
					   SQLITE_TRANSIENT);
		  }
		else
		    sqlite3_result_null (pContext);
		return SQLITE_OK;
	    }
	  if (row->Undefined != NULL)
	    {
		/* special case: there is an undefined destination */
		if (column == 0)
		  {
		      /* the currently used Algorithm */
		      if (net->currentAlgorithm == VROUTE_A_STAR_ALGORITHM)
			  algorithm = "A*";
		      else
			  algorithm = "Dijkstra";
		      sqlite3_result_text (pContext, algorithm,
					   strlen (algorithm),
					   SQLITE_TRANSIENT);
		  }
		if (column == 1)
		  {
		      /* the current Request type */
		      if (net->currentRequest == VROUTE_TSP_NN)
			  algorithm = "TSP NN";
		      else if (net->currentRequest == VROUTE_TSP_GA)
			  algorithm = "TSP GA";
		      else
			  algorithm = "Shortest Path";
		      sqlite3_result_text (pContext, algorithm,
					   strlen (algorithm),
					   SQLITE_TRANSIENT);
		  }
		if (column == 2)
		  {
		      /* the currently set Options */
		      if (net->currentOptions == VROUTE_SHORTEST_PATH_SIMPLE)
			  algorithm = "Simple";
		      else if (net->currentOptions ==
			       VROUTE_SHORTEST_PATH_NO_ARCS)
			  algorithm = "No Arcs";
		      else if (net->currentOptions ==
			       VROUTE_SHORTEST_PATH_NO_GEOMS)
			  algorithm = "No Geometries";
		      else
			  algorithm = "Full";
		      sqlite3_result_text (pContext, algorithm,
					   strlen (algorithm),
					   SQLITE_TRANSIENT);
		  }
		if (column == 3)
		  {
		      /* the currently set delimiter char */
		      if (isprint (cursor->pVtab->currentDelimiter))
			  sprintf (delimiter, "%c [dec=%d, hex=%02x]",
				   cursor->pVtab->currentDelimiter,
				   cursor->pVtab->currentDelimiter,
				   cursor->pVtab->currentDelimiter);
		      else
			  sprintf (delimiter, "[dec=%d, hex=%02x]",
				   cursor->pVtab->currentDelimiter,
				   cursor->pVtab->currentDelimiter);
		      sqlite3_result_text (pContext, delimiter,
					   strlen (delimiter),
					   SQLITE_TRANSIENT);
		  }
		if (column == 4)
		  {
		      /* the RouteNum column */
		      sqlite3_result_int (pContext, row->RouteNum);
		  }
		if (column == 5)
		  {
		      /* the RouteRow column */
		      sqlite3_result_int (pContext, row->RouteRow);
		  }
		if (column == 6)
		  {
		      /* role of this row */
		      if (row->To != NULL)
			  role = "Unreachable NodeTo";
		      else
			  role = "Undefined NodeTo";
		      sqlite3_result_text (pContext, role, strlen (role),
					   SQLITE_TRANSIENT);
		  }
		if (column == 7)
		  {
		      /* the ArcRowId column */
		      sqlite3_result_null (pContext);
		  }
		if (column == 8)
		  {
		      /* the NodeFrom column */
		      if (row->From == NULL)
			{
			    sqlite3_result_text (pContext,
						 row->Undefined,
						 strlen (row->Undefined),
						 SQLITE_STATIC);
			}
		      else
			{
			    if (node_code)
				sqlite3_result_text (pContext,
						     row->From->Code,
						     strlen (row->From->Code),
						     SQLITE_STATIC);
			    else
				sqlite3_result_int64 (pContext, row->From->Id);
			}
		  }
		if (column == 9)
		  {
		      /* the NodeTo column */
		      sqlite3_result_text (pContext,
					   row->Undefined,
					   strlen (row->Undefined),
					   SQLITE_STATIC);
		  }
		if (column == 10)
		  {
		      /* the Cost column */
		      sqlite3_result_null (pContext);
		  }
		if (column == 11)
		  {
		      /* the Geometry column */
		      sqlite3_result_null (pContext);
		  }
		if (column == 12)
		  {
		      /* the [optional] Name column */
		      sqlite3_result_null (pContext);
		  }
		return SQLITE_OK;
	    }
	  if (row->linkRef == NULL)
	    {
		/* special case: this one is the solution summary */
		if (column == 0)
		  {
		      /* the currently used Algorithm */
		      if (net->currentAlgorithm == VROUTE_A_STAR_ALGORITHM)
			  algorithm = "A*";
		      else
			  algorithm = "Dijkstra";
		      sqlite3_result_text (pContext, algorithm,
					   strlen (algorithm),
					   SQLITE_TRANSIENT);
		  }
		if (column == 1)
		  {
		      /* the current Request type */
		      if (net->currentRequest == VROUTE_TSP_NN)
			  algorithm = "TSP NN";
		      else if (net->currentRequest == VROUTE_TSP_GA)
			  algorithm = "TSP GA";
		      else
			  algorithm = "Shortest Path";
		      sqlite3_result_text (pContext, algorithm,
					   strlen (algorithm),
					   SQLITE_TRANSIENT);
		  }
		if (column == 2)
		  {
		      /* the currently set Options */
		      if (net->currentOptions == VROUTE_SHORTEST_PATH_SIMPLE)
			  algorithm = "Simple";
		      else if (net->currentOptions ==
			       VROUTE_SHORTEST_PATH_NO_ARCS)
			  algorithm = "No Arcs";
		      else if (net->currentOptions ==
			       VROUTE_SHORTEST_PATH_NO_GEOMS)
			  algorithm = "No Geometries";
		      else
			  algorithm = "Full";
		      sqlite3_result_text (pContext, algorithm,
					   strlen (algorithm),
					   SQLITE_TRANSIENT);
		  }
		if (column == 3)
		  {
		      /* the currently set delimiter char */
		      if (isprint (cursor->pVtab->currentDelimiter))
			  sprintf (delimiter, "%c [dec=%d, hex=%02x]",
				   cursor->pVtab->currentDelimiter,
				   cursor->pVtab->currentDelimiter,
				   cursor->pVtab->currentDelimiter);
		      else
			  sprintf (delimiter, "[dec=%d, hex=%02x]",
				   cursor->pVtab->currentDelimiter,
				   cursor->pVtab->currentDelimiter);
		      sqlite3_result_text (pContext, delimiter,
					   strlen (delimiter),
					   SQLITE_TRANSIENT);
		  }
		if (column == 4)
		  {
		      /* the RouteNum column */
		      sqlite3_result_int (pContext, row->RouteNum);
		  }
		if (column == 5)
		  {
		      /* the RouteRow column */
		      sqlite3_result_int (pContext, row->RouteRow);
		  }
		if (column == 6)
		  {
		      /* role of this row */
		      if (cursor->pVtab->multiSolution->Mode ==
			  VROUTE_TSP_SOLUTION && row->RouteNum == 0)
			  role = "TSP Solution";
		      else
			{
			    if (row->From == row->To)
				role = "Unreachable NodeTo";
			    else
				role = "Route";
			}
		      sqlite3_result_text (pContext, role, strlen (role),
					   SQLITE_TRANSIENT);
		  }
		if (row->From == NULL || row->To == NULL)
		  {
		      /* empty [uninitialized] solution */
		      if (column > 0)
			  sqlite3_result_null (pContext);
		      return SQLITE_OK;
		  }
		if (column == 7)
		  {
		      /* the ArcRowId column */
		      sqlite3_result_null (pContext);
		  }
		if (column == 8)
		  {
		      /* the NodeFrom column */
		      if (node_code)
			  sqlite3_result_text (pContext,
					       row->From->Code,
					       strlen (row->From->Code),
					       SQLITE_STATIC);
		      else
			  sqlite3_result_int64 (pContext, row->From->Id);
		  }
		if (column == 9)
		  {
		      /* the NodeTo column */
		      if (node_code)
			  sqlite3_result_text (pContext,
					       row->To->Code,
					       strlen (row->To->Code),
					       SQLITE_STATIC);
		      else
			  sqlite3_result_int64 (pContext, row->To->Id);
		  }
		if (column == 10)
		  {
		      /* the Cost column */
		      if (row->RouteNum != 0 && (row->From == row->To))
			  sqlite3_result_null (pContext);
		      else
			  sqlite3_result_double (pContext, row->TotalCost);
		  }
		if (column == 11)
		  {
		      /* the Geometry column */
		      if (!(row->Geometry))
			  sqlite3_result_null (pContext);
		      else
			{
			    /* builds the BLOB geometry to be returned */
			    int len;
			    unsigned char *p_result = NULL;
			    gaiaToSpatiaLiteBlobWkb (row->Geometry,
						     &p_result, &len);
			    sqlite3_result_blob (pContext, p_result, len, free);
			}
		  }
		if (column == 12)
		  {
		      /* the [optional] Name column */
		      sqlite3_result_null (pContext);
		  }
	    }
	  else
	    {
		/* ordinary case: this one is an Arc used by the solution */
		if (column == 0)
		  {
		      /* the currently used Algorithm */
		      if (net->currentAlgorithm == VROUTE_A_STAR_ALGORITHM)
			  algorithm = "A*";
		      else
			  algorithm = "Dijkstra";
		      sqlite3_result_text (pContext, algorithm,
					   strlen (algorithm),
					   SQLITE_TRANSIENT);
		  }
		if (column == 1)
		  {
		      /* the current Request type */
		      if (net->currentRequest == VROUTE_TSP_NN)
			  algorithm = "TSP NN";
		      else if (net->currentRequest == VROUTE_TSP_GA)
			  algorithm = "TSP GA";
		      else
			  algorithm = "Shortest Path";
		      sqlite3_result_text (pContext, algorithm,
					   strlen (algorithm),
					   SQLITE_TRANSIENT);
		  }
		if (column == 2)
		  {
		      /* the currently set Options */
		      if (net->currentOptions == VROUTE_SHORTEST_PATH_SIMPLE)
			  algorithm = "Simple";
		      else if (net->currentOptions ==
			       VROUTE_SHORTEST_PATH_NO_ARCS)
			  algorithm = "No Arcs";
		      else if (net->currentOptions ==
			       VROUTE_SHORTEST_PATH_NO_GEOMS)
			  algorithm = "No Geometries";
		      else
			  algorithm = "Full";
		      sqlite3_result_text (pContext, algorithm,
					   strlen (algorithm),
					   SQLITE_TRANSIENT);
		  }
		if (column == 3)
		  {
		      /* the currently set delimiter char */
		      if (isprint (cursor->pVtab->currentDelimiter))
			  sprintf (delimiter, "%c [dec=%d, hex=%02x]",
				   cursor->pVtab->currentDelimiter,
				   cursor->pVtab->currentDelimiter,
				   cursor->pVtab->currentDelimiter);
		      else
			  sprintf (delimiter, "[dec=%d, hex=%02x]",
				   cursor->pVtab->currentDelimiter,
				   cursor->pVtab->currentDelimiter);
		      sqlite3_result_text (pContext, delimiter,
					   strlen (delimiter),
					   SQLITE_TRANSIENT);
		  }
		if (column == 4)
		  {
		      /* the RouteNum column */
		      sqlite3_result_int (pContext, row->RouteNum);
		  }
		if (column == 5)
		  {
		      /* the RouteRow column */
		      sqlite3_result_int (pContext, row->RouteRow);
		  }
		if (column == 6)
		  {
		      /* role of this row */
		      role = "Link";
		      sqlite3_result_text (pContext, role, strlen (role),
					   SQLITE_TRANSIENT);
		  }
		if (column == 7)
		  {
		      /* the ArcRowId column */
		      sqlite3_result_int64 (pContext,
					    row->linkRef->Arc->ArcRowid);
		  }
		if (column == 8)
		  {
		      /* the NodeFrom column */
		      if (node_code)
			  sqlite3_result_text (pContext,
					       row->linkRef->Arc->NodeFrom->
					       Code,
					       strlen (row->linkRef->Arc->
						       NodeFrom->Code),
					       SQLITE_STATIC);
		      else
			  sqlite3_result_int64 (pContext,
						row->linkRef->Arc->NodeFrom->
						Id);
		  }
		if (column == 9)
		  {
		      /* the NodeTo column */
		      if (node_code)
			  sqlite3_result_text (pContext,
					       row->linkRef->Arc->NodeTo->Code,
					       strlen (row->linkRef->Arc->
						       NodeTo->Code),
					       SQLITE_STATIC);
		      else
			  sqlite3_result_int64 (pContext,
						row->linkRef->Arc->NodeTo->Id);
		  }
		if (column == 10)
		  {
		      /* the Cost column */
		      sqlite3_result_double (pContext, row->linkRef->Arc->Cost);
		  }
		if (column == 11)
		  {
		      /* the Geometry column */
		      sqlite3_result_null (pContext);
		  }
		if (column == 12)
		  {
		      /* the [optional] Name column */
		      if (row->linkRef->Name)
			  sqlite3_result_text (pContext, row->linkRef->Name,
					       strlen (row->linkRef->Name),
					       SQLITE_STATIC);
		      else
			  sqlite3_result_null (pContext);
		  }
	    }
      }
    return SQLITE_OK;
}

static int
vroute_rowid (sqlite3_vtab_cursor * pCursor, sqlite_int64 * pRowid)
{
/* fetching the ROWID */
    virtualroutingCursorPtr cursor = (virtualroutingCursorPtr) pCursor;
    *pRowid = cursor->pVtab->multiSolution->CurrentRowId;
    return SQLITE_OK;
}

static int
vroute_update (sqlite3_vtab * pVTab, int argc, sqlite3_value ** argv,
	       sqlite_int64 * pRowid)
{
/* generic update [INSERT / UPDATE / DELETE */
    virtualroutingPtr p_vtab = (virtualroutingPtr) pVTab;
    if (pRowid)
	pRowid = pRowid;	/* unused arg warning suppression */
    if (argc == 1)
      {
	  /* performing a DELETE is forbidden */
	  return SQLITE_READONLY;
      }
    else
      {
	  if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	    {
		/* performing an INSERT is forbidden */
		return SQLITE_READONLY;
	    }
	  else
	    {
		/* performing an UPDATE */
		if (argc == 15)
		  {
		      p_vtab->currentAlgorithm = VROUTE_DIJKSTRA_ALGORITHM;
		      p_vtab->currentDelimiter = ',';
		      if (sqlite3_value_type (argv[2]) == SQLITE_TEXT)
			{
			    const unsigned char *algorithm =
				sqlite3_value_text (argv[2]);
			    if (strcasecmp ((char *) algorithm, "A*") == 0)
				p_vtab->currentAlgorithm =
				    VROUTE_A_STAR_ALGORITHM;
			}
		      if (p_vtab->graph->AStar == 0)
			  p_vtab->currentAlgorithm = VROUTE_DIJKSTRA_ALGORITHM;
		      if (sqlite3_value_type (argv[3]) == SQLITE_TEXT)
			{
			    const unsigned char *request =
				sqlite3_value_text (argv[3]);
			    if (strcasecmp ((char *) request, "TSP") == 0)
				p_vtab->currentRequest = VROUTE_TSP_NN;
			    else if (strcasecmp
				     ((char *) request, "TSP NN") == 0)
				p_vtab->currentRequest = VROUTE_TSP_NN;
			    else if (strcasecmp
				     ((char *) request, "TSP GA") == 0)
				p_vtab->currentRequest = VROUTE_TSP_GA;
			    else if (strcasecmp
				     ((char *) request, "SHORTEST PATH") == 0)
				p_vtab->currentRequest = VROUTE_SHORTEST_PATH;
			}
		      if (sqlite3_value_type (argv[4]) == SQLITE_TEXT)
			{
			    const unsigned char *options =
				sqlite3_value_text (argv[4]);
			    if (strcasecmp ((char *) options, "NO ARCS") == 0)
				p_vtab->currentOptions =
				    VROUTE_SHORTEST_PATH_NO_ARCS;
			    else if (strcasecmp
				     ((char *) options, "NO GEOMETRIES") == 0)
				p_vtab->currentOptions =
				    VROUTE_SHORTEST_PATH_NO_GEOMS;
			    else if (strcasecmp ((char *) options, "SIMPLE") ==
				     0)
				p_vtab->currentOptions =
				    VROUTE_SHORTEST_PATH_SIMPLE;
			    else if (strcasecmp ((char *) options, "FULL") == 0)
				p_vtab->currentOptions =
				    VROUTE_SHORTEST_PATH_FULL;
			}
		      if (sqlite3_value_type (argv[5]) == SQLITE_TEXT)
			{
			    const unsigned char *delimiter =
				sqlite3_value_text (argv[5]);
			    p_vtab->currentDelimiter = *delimiter;
			}
		  }
		return SQLITE_OK;
	    }
      }
}

static int
vroute_begin (sqlite3_vtab * pVTab)
{
/* BEGIN TRANSACTION */
    if (pVTab)
	pVTab = pVTab;		/* unused arg warning suppression */
    return SQLITE_OK;
}

static int
vroute_sync (sqlite3_vtab * pVTab)
{
/* BEGIN TRANSACTION */
    if (pVTab)
	pVTab = pVTab;		/* unused arg warning suppression */
    return SQLITE_OK;
}

static int
vroute_commit (sqlite3_vtab * pVTab)
{
/* BEGIN TRANSACTION */
    if (pVTab)
	pVTab = pVTab;		/* unused arg warning suppression */
    return SQLITE_OK;
}

static int
vroute_rollback (sqlite3_vtab * pVTab)
{
/* BEGIN TRANSACTION */
    if (pVTab)
	pVTab = pVTab;		/* unused arg warning suppression */
    return SQLITE_OK;
}

static int
vroute_rename (sqlite3_vtab * pVTab, const char *zNew)
{
/* BEGIN TRANSACTION */
    if (pVTab)
	pVTab = pVTab;		/* unused arg warning suppression */
    if (zNew)
	zNew = zNew;		/* unused arg warning suppression */
    return SQLITE_ERROR;
}

static int
splitevirtualroutingInit (sqlite3 * db)
{
    int rc = SQLITE_OK;
    my_route_module.iVersion = 1;
    my_route_module.xCreate = &vroute_create;
    my_route_module.xConnect = &vroute_connect;
    my_route_module.xBestIndex = &vroute_best_index;
    my_route_module.xDisconnect = &vroute_disconnect;
    my_route_module.xDestroy = &vroute_destroy;
    my_route_module.xOpen = &vroute_open;
    my_route_module.xClose = &vroute_close;
    my_route_module.xFilter = &vroute_filter;
    my_route_module.xNext = &vroute_next;
    my_route_module.xEof = &vroute_eof;
    my_route_module.xColumn = &vroute_column;
    my_route_module.xRowid = &vroute_rowid;
    my_route_module.xUpdate = &vroute_update;
    my_route_module.xBegin = &vroute_begin;
    my_route_module.xSync = &vroute_sync;
    my_route_module.xCommit = &vroute_commit;
    my_route_module.xRollback = &vroute_rollback;
    my_route_module.xFindFunction = NULL;
    my_route_module.xRename = &vroute_rename;
    sqlite3_create_module_v2 (db, "virtualrouting", &my_route_module, NULL, 0);
    return rc;
}

SPATIALITE_PRIVATE int
virtualrouting_extension_init (void *xdb)
{
    sqlite3 *db = (sqlite3 *) xdb;
    return splitevirtualroutingInit (db);
}
