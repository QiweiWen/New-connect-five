//
//  hashTable.c
//  Connect-five
//
//  Created by Qiwei Wen on 3/03/13.
//  Copyright (c) 2013 Qiwei Wen. All rights reserved.
//

#include <stdio.h>
#include "hashTable.h"
#include "Game.h"
#include <stdlib.h>

//a 4MB hash table
#define TABLE_SIZE 1048576


struct _hashTable{
    Key* table;
};

struct _hashKey{
    U64 key;
    int val;
    int depth;
};

U64 rand64(){
    return rand() ^ ((U64)rand() << 15) ^ ((U64)rand() << 30) ^ ((U64)rand() << 45) ^ ((U64)rand() << 60);
}

Table newTable(void){
    Table t = malloc(sizeof(struct _hashTable));
    t->table = malloc(sizeof(struct _hashKey)*TABLE_SIZE);
    U64 i = 0;
    while (i < TABLE_SIZE){
        t->table[i] = malloc(sizeof(struct _hashKey));
        (t->table[i])->key = 0;
        (t->table[i])->depth = 0;
        (t->table[i])->val = VAL_UNKNOWN;
        i++;
    }
    zobrist = malloc(sizeof(U64**)*sideLength);
    i = 0;
    int j = 0;
    int k = 0;
    while (i < sideLength){
        zobrist[i] = malloc(sizeof(U64*)*sideLength);
        while (j < sideLength){
            zobrist[i][j] = malloc(sizeof(U64)*3);
            while (k < 3){
                zobrist[i][j][k] = rand64();
                k++;
            }
            k = 0;
            j++;
        }
        k = 0;
        j = 0;
        i++;
    }
    return t;
}

Table hash_insert(U64 key,Table t,int depth,Game g,int value){
    /*
    printf("INSERTING, key is %ld\n",key);
    printGame(g);
    printf("value is %d\n\n",value);
    */
    U64 newKey = key;
    U64 index = newKey%TABLE_SIZE;
    if (index < 0) index+= TABLE_SIZE;
    if ((t->table[index])->val == VAL_UNKNOWN){
        (t->table[index])->key = newKey;
        (t->table[index])->depth = depth;
        (t->table[index])->val = value;
    }else{
        if (depth > (t->table[index])->depth){
            (t->table[index])->key = newKey;
            (t->table[index])->depth = depth;
            (t->table[index])->val = value;
        }
    }
    return t;
}

int hash_lookUp(U64 hashKey, Table t){
    U64 index = hashKey%TABLE_SIZE;
    if (index < 0) index+= TABLE_SIZE;
    return (t->table[index])->val;
}

//TODO: free the keys in the array
void destroyTable(Table t){
    int i = 0;
    int j = 0;
    while (i < sideLength){
        while (j < sideLength){
            free(zobrist[i][j]);
            j++;
        }
        j = 0;
        free(zobrist[i]);
        i++;
    }
    U64 index = 0;
    while (index < TABLE_SIZE){
        free(t->table[index]);
        index++;
    }
    free(t->table);
    free(t);
}

U64 hash(U64 oldKey,Move m,Game g){
    return oldKey^zobrist[m.x][m.y][inquire(m.x,m.y,g)];
}

U64 newHash(Game g){
    U64 key = 0;
    int i = 0;
    int j = 0;
    while (i < sideLength){
        while (j < sideLength){
            key = key^zobrist[i][j][inquire(i,j,g)];
            j++;
        }
        j = 0;
        i++;
    }
    return key;
}