//
//  List.h
//  Connect-five
//
//  Created by Qiwei Wen on 15/01/13.
//  Copyright (c) 2013 Qiwei Wen. All rights reserved.
//



#ifndef Connect_five_List_h
#define Connect_five_List_h

#include "Game.h"


int*** eval_result;

Node createList(void*);

Node nextItem(Node);

Node insertItem(void*,Node);

Node deleteItem(int,Node,Node);

void destroyList(Node,int);

Node generateNextMoves(Game);

void printList(Node);

void* key(Node);

void* pop(Node);

int isLast(Node n);

int size(Node n);

//sort a list of moves by its heuristic value
//only the first 10 items are returned for deeper searching
Node insertionSortAscend(Node n,Game g,int threadID);

Node insertionSortDescend(Node n,Game g,int threadID);

void printList_mVal(Node n,Game g,int threadID);

int moveValue(Move m,int threadID);

int numItems(Node n);

#endif
