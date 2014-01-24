//
//  hashTable.h
//  Connect-five
//
//  Created by Qiwei Wen on 3/03/13.
//  Copyright (c) 2013 Qiwei Wen. All rights reserved.
//

#ifndef Connect_five_hashTable_h
#define Connect_five_hashTable_h

#include "Game.h"

#define VAL_UNKNOWN 10000002

//typedef long U64;

typedef struct _hashKey* Key;

typedef struct _hashTable* Table;

Table globalTable;

U64*** zobrist;

U64 rand64();

Table newTable(void);

Table hash_insert(U64 key,Table t,int depth,Game g,int value);

int hash_lookUp(U64 hashKey, Table t);

void destroyTable(Table t);

U64 hash(U64 oldKey,Move m,Game g);

U64 newHash(Game g);

#endif
