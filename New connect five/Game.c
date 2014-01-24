//
//  Game.c
//  Connect-five
//
//  Created by Qiwei Wen on 1number/01/13.
//  Copyright (c) 2013 Qiwei Wen. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include "List.h"
#include "Game.h"
#include "threadPool.h"
#include "hashTable.h"

#define FIVE_VALUE 10000
#define ALMOST_AS_GOOD 1000
#define UNBLOCKED_THREE 4
#define BLOCKED_THREE 1
#define BLOCKED_FOUR 5
#define UNBLOCKED_TWO 1
#define MAX_DEPTH 4

int* threadWorkStats;

int nodesVisited = 0;

int crazyOutputMode = 0;

struct _game{
    int** board;
    int whoseTurn;
    int terminalScore;
    int ended;
};

struct _search_result{
    int found;
    int blocked;
    int spaced;
};



int isEnded(Game g){
    return g->ended;
}

int whoseTurn(Game g){
    return g->whoseTurn;
}

int inquire(int x, int y, Game g){
    assert(x >= 0 && y >= 0 && x < sideLength && y < sideLength);
    return g->board[x][y];
}

Game createNewGame(int whoseTurn,int** board,int terminalScore,int ended){
    Game g = malloc(sizeof(struct _game));
    g->whoseTurn = whoseTurn;
    g->board = board;
    g->terminalScore = terminalScore;
    g->ended = ended;
    return g;
}

void advanceGame(int player,int x,int y,Game g,int threadID){
    
    if ((player == BLACK || player == WHITE)
        && player == g->whoseTurn
        && x < sideLength
        && y < sideLength
        && x >= 0 && y >= 0){
        
        if (g->board[x][y] == EMPTY){
            g->board[x][y] = player;
            
            if (g->whoseTurn == BLACK){
                g->whoseTurn = WHITE;
            }else{
                g->whoseTurn = BLACK;
            }
            
        }
        stats result = searchAround(x,y,g,5,EMPTY,threadID);
        int foundFive =result.found && (result.existUnspaced);
        g->ended = foundFive;
    }
}


void put(int x,int y, Game g,int player){
    g->board[x][y] = player;
}

stats searchAround(int x,int y,Game g,int number,int pretend,int threadID){
    if (pretend != EMPTY){
        if (pretend == BLACK){
            g->board[x][y] = BLACK;
        }else if (pretend == WHITE){
            g->board[x][y] = WHITE;
        }
    }
    assert(number >= 2 && number <= 5);
    numBlockedBothSides[threadID] = 0;
    numBlockedOneSide[threadID] = 0;
    numUnblocked[threadID] = 0;
    alreadyBlocked[threadID] = 0;
    spacedOnOneSide[threadID] = 0;
    currentlySearching[threadID] = number;
    whichSide[threadID] = 1;
    spaced[threadID] = 0;
    unspaced[threadID] = 0;
    
    int found = 0;
    
    found = search(&incr,&incr,number,x,y,g,threadID).found+search(&incr,&doNothing,number,x,y,g,threadID).found+search(&incr,&decr,number,x,y,g,threadID).found
    +search(&doNothing,&decr,number,x,y,g,threadID).found+search(&decr,&decr,number,x,y,g,threadID).found
    +search(&decr,&doNothing,number,x,y,g,threadID).found+search(&decr,&incr,number,x,y,g,threadID).found
    +search(&doNothing,&incr,number,x,y,g,threadID).found;
    
    whichSide[threadID] = 2;
    
    found = found + (search(&incr,&incr,number-1,x,y,g,threadID).found && search(&decr,&decr,2,x,y,g,threadID).found)
    +(search(&incr,&doNothing,number-1,x,y,g,threadID).found && search(&decr,&doNothing,2,x,y,g,threadID).found)
    +(search(&incr,&decr,number-1,x,y,g,threadID).found && search(&decr,&incr,2,x,y,g,threadID).found)
    +(search(&doNothing,&decr,number-1,x,y,g,threadID).found && search(&doNothing,&incr,2,x,y,g,threadID).found);
    if (number > 2){
        if (number -1 != 2){
            found = found + (search(&decr,&decr,number-1,x,y,g,threadID).found && search(&incr,&incr,2,x,y,g,threadID).found)
            +(search(&decr,&doNothing,number-1,x,y,g,threadID).found && search(&incr,&doNothing,2,x,y,g,threadID).found)
            +(search(&decr,&incr,number-1,x,y,g,threadID).found && search(&incr,&decr,2,x,y,g,threadID).found)
            +(search(&doNothing,&incr,number-1,x,y,g,threadID).found && search(&doNothing,&decr,2,x,y,g,threadID).found);
        }
        
        
        if (number == 5){
            whichSide[threadID] = 2;
            found = found + ((search(&incr,&incr,number-2,x,y,g,threadID).found && search(&decr,&decr,3,x,y,g,threadID).found)
                             +(search(&incr,&doNothing,number-2,x,y,g,threadID).found && search(&decr,&doNothing,3,x,y,g,threadID).found)
                             +(search(&incr,&decr,number-2,x,y,g,threadID).found && search(&decr,&incr,3,x,y,g,threadID).found)
                             +(search(&doNothing,&decr,number-2,x,y,g,threadID).found && search(&doNothing,&incr,3,x,y,g,threadID).found));
            
        }
    }
    if (pretend != EMPTY) g->board[x][y] = EMPTY;
    stats s;
    if (found) found = 1;
    s.found = found;
    s.numBlockedBothSides = numBlockedBothSides[threadID];
    s.numBlockedOneSide = numBlockedOneSide[threadID];
    s.numUnblocked = numUnblocked[threadID];
    s.existSpaced = spaced[threadID];
    s.existUnspaced  = unspaced[threadID];
    return s;
}

void unmakeMove(int x, int y, Game g){
    if (g->board[x][y] != EMPTY && g->whoseTurn != g->board[x][y]){
        if (g->whoseTurn == BLACK){
            g->whoseTurn = WHITE;
        }else{
            g->whoseTurn = BLACK;
        }
        if (g->ended){
            g->ended = 0;
        }
        g->board[x][y] = 0;
        
    }
}

int** duplicate(int** original){
    int** copy = malloc(sideLength*sizeof(int*));
    int i = 0;
    int j = 0;
    while(i < sideLength){
        copy[i] = malloc(sideLength*sizeof(int));
        while (j < sideLength){
            copy[i][j] = original[i][j];
            j++;
        }
        j = 0;
        i++;
    }
    return copy;
}

void destroyGame(Game g){
    int i = 0;
    while (i < sideLength){
        free(g->board[i]);
        i++;
    }
    free(g->board);
    free(g);
}

searchInfo search(int (*fx)(int), int (*fy)(int), int number,int x,int y,Game g,int threadID){
    if (whichSide[threadID] == 1){
        whichSide[threadID] = 2;
    }else if (whichSide[threadID] == 2){
        whichSide[threadID] = 1;
    }
    
    searchInfo result;
    result.blocked = 0;
    result.spaced = 0;
    int found = 1;
    int blankSpaceAllowed = 1;
    int i = 0;
    int toSearch = g->board[x][y];
    int enemy;
    if (toSearch == BLACK){
        enemy = WHITE;
    }else {
        enemy = BLACK;
    }
    
    
    int origX = x;
    int origY = y;
    
    while (i < number && x >= 0 && y >= 0 && x<sideLength && y < sideLength && found == 1){
        if (g->board[x][y] == toSearch){
            x = fx(x);
            y = fy(y);
            i++;
        } else {
            if (g->board[x][y] == EMPTY && blankSpaceAllowed == 1){
                blankSpaceAllowed --;
                x = fx(x);
                y = fy(y);
            }else{
                found = 0;
            }
        }
    }
    if (found) found = (i == number);
    
    if (found){
        int x1,y1,x2,y2;
        
        //take the inverse functions of the input functions;
        x1 = -fx(-origX);
        y1 = -fy(-origY);
        
        x2 = x;
        y2 = y;
        
        if (outOfBound(x1,y1) || outOfBound(x2,y2)){
            if (outOfBound(x1, y1) && outOfBound(x2, y2)){
                result.blocked = 2;
            }else{
                result.blocked = 1;
            }
        }else{
            if (number != currentlySearching[threadID]){
                if (g->board[x1][y1] != enemy && g->board[x2][y2] == EMPTY){
                    result.blocked = 0;
                }else{
                    if (g->board[x2][y2] == toSearch){
                        if (currentlySearching[threadID] != 5){
                            found = 0;
                        }
                    }else{
                        if (g->board[x1][y1] == enemy && g->board[x2][y2] == enemy){
                            result.blocked = 2;
                        }else{
                            result.blocked = 1;
                        }
                    }
                }
            }else{
                
                if (g->board[x1][y1] == EMPTY && g->board[x2][y2] == EMPTY){
                    result.blocked = 0;
                }else{
                    if (g->board[x2][y2] == toSearch || g->board[x1][y1] == toSearch){
                        if (currentlySearching[threadID] != 5){
                            found = 0;
                        }
                    }else{
                        if (g->board[x1][y1] == enemy && g->board[x2][y2] == enemy){
                            result.blocked = 2;
                        }else{
                            result.blocked = 1;
                        }
                    }
                }
                
            }
        }
        
        if (currentlySearching[threadID] != number){
            if (whichSide[threadID] == 1){
                if (blankSpaceAllowed == 0){
                    spacedOnOneSide[threadID] = 1;
                }else{
                    spacedOnOneSide[threadID] = 0;
                }
            }else{
                if (spacedOnOneSide[threadID]){
                    if (blankSpaceAllowed == 0){
                        found = 0;
                    }else{
                        result.spaced = 1;
                    }
                }else{
                    if (blankSpaceAllowed == 0){
                        result.spaced = 1;
                    }
                }
            }
        }else{
            if (blankSpaceAllowed == 0){
                result.spaced = 1;
            }
        }
        
    }
    
    
    
    result.found = found;
    if (found){
        if (currentlySearching[threadID] == number){
            spaced[threadID] = spaced[threadID]||result.spaced;
            unspaced[threadID] = unspaced[threadID] || !(result.spaced);
            
            if (result.blocked == 0){
                numUnblocked[threadID]++;
            }else if (result.blocked == 1){
                numBlockedOneSide[threadID]++;
            }else if (result.blocked == 2){
                numBlockedBothSides[threadID]++;
            }
        }else{
            if (whichSide[threadID] == 2){
                spaced[threadID] = spaced[threadID]||result.spaced;
                unspaced[threadID] = unspaced[threadID] || !(result.spaced);
                
                if (alreadyBlocked[threadID]){
                    if (result.blocked == 1){
                        numBlockedBothSides[threadID]++;
                    }else if (result.blocked == 0){
                        numBlockedOneSide[threadID]++;
                    }
                }else{
                    if (result.blocked == 1){
                        numBlockedOneSide[threadID]++;
                    }else if (result.blocked == 0){
                        numUnblocked[threadID]++;
                    }
                }
                
            }else{
                if (result.blocked == 1){
                    alreadyBlocked[threadID] = 1;
                }else if (result.blocked == 0){
                    alreadyBlocked[threadID] = 0;
                }
                
            }
            
        }
    }else{
        if (whichSide[threadID] == 1){
            whichSide[threadID] = 2;
        }
    }
    return result;
}

int incr(int a){
    a+=1;
    return a;
}

int decr(int a){
    a-=1;
    return a;
}

int doNothing(int a){
    return a;
}

void printGame(Game g){
    
    if (!g->ended) printf("player %d's turn\n",g->whoseTurn);
    if (g->ended) printf("End state:\n");
    int i = 0;
    int j = sideLength - 1;
    while (j >= 0){
        while (i < sideLength){
            if (i==0) {
                printf("%d",j);
                if (j < 10) printf(" ");
            }
            
            printf("|%c|",numToChar(g->board[i][j]));
            
            i++;
        }
        printf("\n");
        i = 0;
        j--;
    }
    i = 0;
    printf("   ");
    while (i < sideLength){
        printf("%d ",i);
        if (i < 10)printf(" ");
        i++;
    }
    printf("\n");
}

void withDraw(Game g){
    g->board[lastMoveAI.x][lastMoveAI.y] = 0;
    g->board[lastMovePlayer.x][lastMovePlayer.y] = 0;
}

//under construction
Move decideMove(Game g){
    
    crazyOutputMode++;
    
    threadWorkStats = malloc(sizeof(int)*NUM_THREADS + 1);
    int index = 0;
    while (index < NUM_THREADS + 1){
        threadWorkStats[index] = 0;
        index++;
    }
    
    nodesVisited = 0;
    rootNodePlayer = g->whoseTurn;
    
    int searchForEndState = 0;
    
    Node best_moves;
    int best_index = 0;
    
    Move m;
    Node n = nextItem(generateNextMoves(g)); //the first item in the list is (-1,-1)
    
    int me = g->whoseTurn;
    int enemy;
    if (me == BLACK){
        enemy = WHITE;
    }else{
        enemy = BLACK;
    }
    n = insertionSortDescend(n, g, 0);
    Node current = n;
    Node last = current;
    
    Game temp;
    
    task* t;
    
    int val = 0;
  
    while (current != NULL && val < FIVE_VALUE){
        val = eval_result[((Move*)key(current))->x][((Move*)key(current))->y][0];
       // printf("(%d,%d) val is %d\n",((Move*)key(current))->x,((Move*)key(current))->y,val);
        last = current;
        current = nextItem(current);
    }
    
    if (val >= FIVE_VALUE){
        searchForEndState = 1;
    }
    
    U64 hashKey = newHash(g);
    
    //printf("%ld is the oldkey\n",hashKey);
    
    U64 newKey;
    
    advanceGame(whoseTurn(g), ((Move*)key(n))->x, ((Move*)key(n))->y, g, 0);
    
    m.x = ((Move*)key(n))->x;
    m.y = ((Move*)key(n))->y;
    
    newKey = hash(hashKey, m, g);
    
    val = min(MAX_DEPTH - 1, -10000001, 10000001, g, 0, 1,searchForEndState,newKey);
    
    unmakeMove(((Move*)key(n))->x, ((Move*)key(n))->y, g);
    
    //printf("(%d,%d) %d\n",((Move*)key(n))->x, ((Move*)key(n))->y,val);
    
    /*
    if (val >= ALMOST_AS_GOOD){
        m.x = ((Move*)key(n))->x;
        m.y = ((Move*)key(n))->y;
        destroyList(n, 1);
        return m;
    }
    */
    
    //
    Node rubbish;
    int num = 0;
    
    current = nextItem(n);
    
    
    /*
    
    int alpha = val;
    int beta = 10000001;
    
    while (current != NULL){
        temp = malloc(sizeof(struct _game));
        temp->ended = g->ended;
        temp->board = duplicate(g->board);
        temp->terminalScore = g->terminalScore;
        temp->whoseTurn = g->whoseTurn;
        
        
        advanceGame(whoseTurn(temp), ((Move*)key(current))->x, ((Move*)key(current))->y, temp, 0);
        
        m = *(Move*)key(current);
        newKey = hash(hashKey, m, g);
        
        val = min(MAX_DEPTH - 1,alpha,beta,temp,0,0,searchForEndState,newKey);
        
        unmakeMove(((Move*)key(current))->x, ((Move*)key(current))->y,  temp);
        
        printf("(%d,%d) %d\n",m.x, m.y,val);
        
       // if (val > alpha) alpha = val;
        
        
        
        current = nextItem(current);
    }
    
    printf("nodes visited: %d\n",nodesVisited);
    
    assert(0==1);
    
    */
    
    while (current != NULL){
        temp = malloc(sizeof(struct _game));
        temp->ended = g->ended;
        temp->board = duplicate(g->board);
        temp->terminalScore = g->terminalScore;
        temp->whoseTurn = g->whoseTurn;
        
        advanceGame(whoseTurn(temp), ((Move*)key(current))->x, ((Move*)key(current))->y, temp, 0);
        
        m = *(Move*)key(current);
        newKey = hash(hashKey, m, g);
        
        num++;
        if (num == 1){
            rubbish = createList(temp);
        }else{
            insertItem(temp,rubbish);
        }
        
        
        
        
        // printGame(temp);
        
        t = malloc(sizeof(task));
        t->arg1 = searchForEndState? 3 : MAX_DEPTH - 1;
        t->arg2 = val;
        t->arg3 = 10000001;
        t->arg4 = temp;
        t->arg6 = 0;
        t->arg7 = searchForEndState;
        t->arg8 = newKey;
        t->result = 0;
        t->func = &min;
        
        queue_add_task(t);
        current = nextItem(current);
    }
    
    queue_add_task(NULL);
    task* fetch;
    while (!workDone){
        //usleep(10);
       
        pthread_mutex_lock(mutex_queue);
        fetch = queue_do_task(workQueue);
        pthread_mutex_unlock(mutex_queue);
        if (fetch != NULL){
            fetch->result = fetch->func(fetch->arg1,fetch->arg2,fetch->arg3,fetch->arg4,0,fetch->arg6,fetch->arg7,fetch->arg8);
            taskNum--;
            tempNum--;
            if (taskNum == 0){
                workDone = 1;
            }
        }
        
    }
    
    printf("Nodes visited: %d\n",nodesVisited);
    
    current = rubbish;
    
    int i = 0;
    
    while (current != NULL){
        destroyGame(key(current));
        current = nextItem(current);
    }
    
    
    printList(n);
    printf("%d/",val);
    printResult(original);
     // assert(0==1);
    
    Node n_first = n;
    n = nextItem(n);
    
    current = original->first;
    
    int maximum = val;
    best_moves = createList((Move*)key(n_first));
    best_index = 1;
    while (current != NULL){
       // printf("result is %d,maximum is %d\n",((task*)key(current))->result,maximum);
       // printf("point is (%d,%d)\n",((Move*)key(n))->x,((Move*)key(n))->y);
        
        if ( ((task*)key(current))->result > maximum){
            maximum = ((task*)key(current))->result;
            
            destroyList(best_moves, 0);
            best_index = 1;
            best_moves = createList((Move*)key(n));
            
        }else if (((task*)key(current))->result == maximum){
            best_index++;
            insertItem((Move*)key(n), best_moves);
        }
        current = nextItem(current);
        n = nextItem(n);
    }
    
    best_index = rand()%best_index;
    
    n = best_moves;
   
    hash_insert(hashKey, globalTable, MAX_DEPTH, g, maximum);
    
    i = 0;
    while (i < best_index){
        n = nextItem(n);
        i++;
    }
    m.x = ((Move*)key(n))->x;
    m.y = ((Move*)key(n))->y;
   
    
    
    destroyList(best_moves, 0);
    destroyList(n_first, 1);
    
    destroyList(rubbish, 0);
    destroyList(original->first, 1);
    free(workQueue);
    free(original);
    original = NULL;
    workQueue = NULL;
    
    index = 0;
    while (index < NUM_THREADS + 1){
        printf("%d\n",threadWorkStats[index]);
        index++;
    }
    return m;
}


int max(int depth, int alpha, int beta, void* temp,int threadID,int principal,int searchForEndState,U64 oldKey){
    
    int a = 0;
    
    if ((a = hash_lookUp(oldKey, globalTable)) != VAL_UNKNOWN){
        /*
        printf("LOOKING UP, key is %ld\n",oldKey);
        printGame(temp);
        printf("a is %d,max,depth %d, principal variation? %d\n\n",a,depth,principal);
         */
        return a;
    }
    
    threadWorkStats[threadID]++;
    
    nodesVisited ++;
    
    Game g = (Game)temp;
    Node n;
    
    Move m;
    U64 hashKey;
    
    int val = 0;
    if (depth == 0||g->ended){
        if (searchForEndState){
            if (!g->ended){
                return -10000000;
            }else{
                return evaluate(g, threadID);
            }
        }else{
            return evaluate(g, threadID);
        }
    }
    
   // printf("A NEW DEPTH ONE NODE\n");
   // printGame(g);
    
    n = generateNextMoves(g);
    n = nextItem(n);
    if (depth != 1) n = insertionSortDescend(n, temp, threadID);
    val = 0;
    int x = ((Move*)key(n))->x;
    int y = ((Move*)key(n))->y;
    Node current = n;
    
    if (principal && depth >= 2){
        advanceGame(whoseTurn(g), x, y, g, threadID);
        
        m = *(Move*)key(current);
        hashKey = hash(oldKey, m, g);
       
        val = min(depth - 1, alpha, beta, g,threadID,principal,searchForEndState,hashKey);
        
        unmakeMove(x, y, g);
        
        if (val > alpha) alpha = val;
        
        //parallelise the evaluation of younger children
        
        task* t;
        Game new;
        int taskNumber = 0;
        
        Node rubbish;
        
        current = nextItem(current);
        
        while (current != NULL){
            taskNumber++;
            new = malloc(sizeof(struct _game));
            new->ended = g->ended;
            new->board = duplicate(g->board);
            new->whoseTurn = g->whoseTurn;
            new->terminalScore = g->terminalScore;
            
            if (taskNumber == 1){
                rubbish = createList(new);
            }else{
                insertItem(new, rubbish);
            }
            
            advanceGame(whoseTurn(new), ((Move*)key(current))->x, ((Move*)key(current))->y, new, threadID);
            
            m = *(Move*)key(current);
            hashKey = hash(oldKey, m, g);
            
            t = malloc(sizeof(task));
            t->arg1 = depth - 1;
            t->arg2 = alpha;
            t->arg3 = beta;
            t->arg4 = new;
            t->arg6 = 0;
            t->arg7 = searchForEndState;
            t->arg8 = hashKey;
            t->result = 0;
            t->func = &min;
            queue_add_task(t);
            current = nextItem(current);
        }
        
        if (depth == 2 && crazyOutputMode == 2){
            crazyOutputSignal = 2;
        }
        
        queue_add_task(NULL);
        task* fetch;
        while (!workDone){
            //usleep(10);
            
            pthread_mutex_lock(mutex_queue);
            fetch = queue_do_task(workQueue);
            pthread_mutex_unlock(mutex_queue);
            if (fetch != NULL){
                fetch->result = fetch->func(fetch->arg1,fetch->arg2,fetch->arg3,fetch->arg4,0,fetch->arg6,fetch->arg7,fetch->arg8);
                taskNum--;
                tempNum--;
                if (taskNum == 0){
                    workDone = 1;
                }
            }
            
        }

        int max = val;
        int num;
        current = original->first;
        while (current != NULL){
            num = ((task*)key(current))->result;
            if (num > max){
                max = num;
            }
            current = nextItem(current);
        }
        
        //cleanup, yuck.
        destroyList(original->first, 1);
        free(original);
        original = NULL;
        free(workQueue);
        workQueue = NULL;
        destroyList(n, 1);
        current = rubbish;
        while (current != NULL){
            destroyGame(key(current));
            current = nextItem(current);
        }
        destroyList(rubbish, 0);
        hash_insert(oldKey, globalTable, depth, g, max);
        return max;
    }else{
        
        int maxChild = -10000001;
        current = n;        
        while (current != NULL){
            x = ((Move*)key(current))->x;
            y = ((Move*)key(current))->y;
            advanceGame(whoseTurn(g), x, y, g, threadID);
            m = *(Move*)key(current);
        //     printGame(g);
            hashKey = hash(oldKey, m, g);
            val = min(depth - 1, alpha, beta, g, threadID, principal,searchForEndState,hashKey);
            
            unmakeMove(x, y, g);
            
            if (val > maxChild){
                maxChild = val;
            }
            
            if (val > alpha) alpha = val;
            if (alpha > beta){
                //cutoff
                destroyList(n, 1);
                hash_insert(oldKey, globalTable, depth, g, alpha);
                return alpha;
            }
            current = nextItem(current);
            
        }
        destroyList(n, 1);
        hash_insert(oldKey, globalTable, depth, g, maxChild);
        return maxChild;
    }
}

int min(int depth, int alpha, int beta, void* temp,int threadID,int principal,int searchForEndState,U64 oldKey){
    
    int a = 0;
    
    if ((a = hash_lookUp(oldKey, globalTable)) != VAL_UNKNOWN){
        /*
        printf("LOOKING UP, key is %ld\n",oldKey);
        printGame(temp);
        printf("a is %d,min,depth %d, principal variation? %d\n\n",a,depth,principal);
         */
        return a;
    }
    
    U64 hashKey = 0;
    
    threadWorkStats[threadID]++;
    
    
    Move m;
    
    nodesVisited ++;
    int val = 0;
    Node n;
    Game g = (Game)temp;
    if (depth == 0||g->ended){
        if (searchForEndState){
            if (!g->ended){
                return -10000000;
            }else{
                return evaluate(g, threadID);
            }
        }else{
            return evaluate(g, threadID);
        }
    }
    
    /*
    if (depth == 1){
        printf("SINGLE-THREAD MODE, ANOTHER DEPTH ONE NODE\n");
        printf("parameters: alpha = %d, beta = %d\n",alpha,beta);
        printGame(g);
    }
    */
    
    n = generateNextMoves(g);
    n = nextItem(n);
    if (depth != 1) n = insertionSortAscend(n, temp, threadID);
    
    int x = ((Move*)key(n))->x;
    int y = ((Move*)key(n))->y;
    Node current = n;
    
    current = nextItem(current);
    
    if (principal && depth >= 2){
        
        advanceGame(whoseTurn(g), x, y, g, threadID);
        
        m = *(Move*)key(current);
        
        hashKey = hash(oldKey, m, g);
       
        val = max(depth - 1, alpha, beta, g,threadID,principal,searchForEndState,hashKey);
        
        unmakeMove(x, y, g);
    
        if (val < beta) beta = val;
        
        //parallelise the evaluation of younger children
        
        task* t;
        Game new;
        int taskNumber = 0;
        
        Node rubbish;
        
        while (current != NULL){
            taskNumber++;
            new = malloc(sizeof(struct _game));
            new->ended = g->ended;
            new->board = duplicate(g->board);
            new->whoseTurn = g->whoseTurn;
            new->terminalScore = g->terminalScore;
            
            if (taskNumber == 1){
                rubbish = createList(new);
            }else{
                insertItem(new, rubbish);
            }
            
            advanceGame(whoseTurn(new), ((Move*)key(current))->x, ((Move*)key(current))->y, new, threadID);
            
            m = *(Move*)key(current);
            
            hashKey = hash(oldKey, m, g);
          //  printGame(new);
            
            t = malloc(sizeof(task));
            t->arg1 = depth - 1;
            t->arg2 = alpha;
            t->arg3 = beta;
            t->arg4 = new;
            t->arg6 = 0;
            t->arg7 = searchForEndState;
            t->arg8 = hashKey;
            t->result = 0;
            t->func = &max;
            queue_add_task(t);
            current = nextItem(current);
        }
        queue_add_task(NULL);
        task* fetch;
        while (!workDone){
            //usleep(10);
            
            pthread_mutex_lock(mutex_queue);
            fetch = queue_do_task(workQueue);
            pthread_mutex_unlock(mutex_queue);
            if (fetch != NULL){
                fetch->result = fetch->func(fetch->arg1,fetch->arg2,fetch->arg3,fetch->arg4,0,fetch->arg6,fetch->arg7,fetch->arg8);
                taskNum--;
                tempNum--;
                if (taskNum == 0){
                    workDone = 1;
                }
            }
            
        }
        
       // printList(n);
       // printf("%d/",val);
       // printResult(original);

        
        int min = val;
        int num;
        current = original->first;
        while (current != NULL){
            num = ((task*)key(current))->result;
            if (num < min){
                min = num;
            }
            current = nextItem(current);
        }
        
        //cleanup, yuck.
        destroyList(original->first, 1);
        free(original);
        original = NULL;
        free(workQueue);
        workQueue = NULL;
        destroyList(n, 1);
        current = rubbish;
        while (current != NULL){
            destroyGame(key(current));
            current = nextItem(current);
        }
        destroyList(rubbish, 0);

        hash_insert(oldKey, globalTable, depth, g, min);
        
        return min;
    }else{
        
        int minChild = 10000001;
        current = n;
      
        
        while (current != NULL){
            x = ((Move*)key(current))->x;
            y = ((Move*)key(current))->y;
            advanceGame(whoseTurn(g), x, y, g, threadID);
            
            m = *(Move*)key(current);
            
            hashKey = hash(oldKey, m, g);
            
            val = max(depth - 1, alpha, beta, g, threadID, principal,searchForEndState,hashKey);
            
            /*
            if (depth == 1){
                printGame(g);
                printf("val is %d\n",val);
                
            }
            */
            
            unmakeMove(x, y, g);
            
            
            if (val < minChild){
                minChild = val;
            }
            if (val < beta) beta = val;
            if (alpha > beta){
                //cutoff
                destroyList(n, 1);
                
                /*
                if (depth == 1){
                    printf("returning %d\n",beta);
                }
                 */
                hash_insert(oldKey, globalTable, depth, g, beta);
                return beta;
            }
            current = nextItem(current);

        }
        destroyList(n, 1);
        
        /*
        if (depth == 1){
            printf("returning %d\n",minChild);
        }
        */
        hash_insert(oldKey, globalTable, depth, g, minChild);
        return minChild;
    }
}


void endGame(Game g){
    if (g != NULL) g->ended = 1;
}



int evaluate (Game g,int threadID){
    
    
    if (g->ended){
        if (g->whoseTurn == rootNodePlayer){
            return -1000*FIVE_VALUE;
        }else{
            return 1000*FIVE_VALUE;
        }
    }
    
    int maxWinningStringMe = 0;
    int maxWinningStringEnemy = 0;
    
    int value = 0;
    int pointValue = 0;
    int me = rootNodePlayer;
    int enemy;
    if (me == BLACK){
        enemy = WHITE;
    }else{
        enemy = BLACK;
    }
    stats searchResultMe;
    stats searchResultEnemy;
    stats searchResultThreeMe;
    stats searchResultFourMe;
    stats searchResultThreeEnemy;
    stats searchResultFourEnemy;
    stats searchResultTwoMe;
    stats searchResultTwoEnemy;
    
    int numFiveMe = 0;
    int numFiveEnemy = 0;
    
    int numWinningMovesMe = 0;
    int numWinningMovesEnemy = 0;
    
    int numThreesMe = 0;
    int numThreesEnemy = 0;
    
    int numFoursMe = 0;
    int numFoursEnemy = 0;
    
    int really_winning = 0;
    
    Node fivesMe;
    Node fivesEnemy;
    
    Node winningMovesMe;
    Node winningMovesEnemy;
    
    Node myThrees;
    Node myFours;
    
    Node enemyThrees;
    Node enemyFours;
    
    Move i_j;
    
    Move* temp;
    
    boundary b = makeBound(g);
    int i = b.leastX;
    int j = b.leastY;
    int k = 0;
    while (i <= b.mostX && value < FIVE_VALUE && value > -FIVE_VALUE){
        while (j <= b.mostY && value < FIVE_VALUE && value > -FIVE_VALUE ){
            if (worthy(i,j,g)){
                really_winning = 0;
                if (g->board[i][j] == EMPTY){
                    pointValue = 0;
                    searchResultMe = searchAround(i, j, g, 5, me,threadID);
                    searchResultEnemy = searchAround(i, j, g, 5, enemy,threadID);
                    
                    if (searchResultMe.found && (searchResultMe.existUnspaced)) {
                        
                        numFiveMe++;
                        temp = malloc(sizeof(Move));
                        temp->x = i;
                        temp->y = j;
                        if (numFiveMe == 1) {
                            fivesMe = createList(temp);
                        }else{
                            insertItem(temp, fivesMe);
                        }
                    }
                    
                    
                    if (searchResultEnemy.found && (searchResultEnemy.existUnspaced)) {
                        
                        numFiveEnemy++;
                        temp = malloc(sizeof(Move));
                        temp->x = i;
                        temp->y = j;
                        if (numFiveEnemy == 1) {
                            fivesEnemy = createList(temp);
                        }else{
                            insertItem(temp, fivesEnemy);
                        }
                    }
                    
                    searchResultThreeMe = searchAround(i, j, g, 3, me,threadID);
                    searchResultFourMe = searchAround(i, j, g, 4, me,threadID);
                    
                    searchResultThreeEnemy = searchAround(i, j, g, 3, enemy,threadID);
                    searchResultFourEnemy = searchAround(i, j, g, 4, enemy,threadID);
                    
                    really_winning = 0;
                    
                    if (winning(searchResultThreeMe,searchResultFourMe)){
                        
                        i_j.x = i;
                        i_j.y = j;
                        if (!((searchResultFourMe.numUnblocked && searchResultFourMe.existUnspaced) || searchResultFourMe.numBlockedOneSide >= 2)){
                            
                            really_winning = reallyWinning(searchResultFourMe.numBlockedOneSide, searchResultThreeMe.numUnblocked,i_j , g, me);
                            if (really_winning && searchResultFourMe.numBlockedOneSide){
                                maxWinningStringMe = 4;
                            }else{
                                maxWinningStringMe = 3;
                            }
                        }else{
                            maxWinningStringMe = 4;
                            really_winning = 1;
                        }
                    }
                    
                    if (really_winning){
                        
                       //printf("(%d,%d),me\n",i,j);
                        
                        temp = malloc(sizeof(Move));
                        temp->x = i;
                        temp->y = j;
                        numWinningMovesMe++;
                        if (numWinningMovesMe == 1){
                            winningMovesMe = createList(temp);
                        }else{
                            insertItem(temp, winningMovesMe);
                        }
                        
                    }else{
                        
                        if (searchResultThreeMe.found){
                            numThreesMe++;
                            temp = malloc(sizeof(Move));
                            temp->x = i;
                            temp->y = j;
                            if (numThreesMe == 1){
                                myThrees = createList(temp);
                            }else{
                                insertItem(temp, myThrees);
                            }
                            
                            k = 0;
                            
                            while (k < searchResultThreeMe.numUnblocked){
                                pointValue += UNBLOCKED_THREE;
                                k++;
                            }
                            k = 0;
                            while (k < searchResultThreeMe.numBlockedOneSide){
                                pointValue+= BLOCKED_THREE;
                                k++;
                            }
                        }
                        
                        if (searchResultFourMe.found){
                            numFoursMe++;
                            temp = malloc(sizeof(Move));
                            temp->x = i;
                            temp->y = j;
                            if (numFoursMe == 1){
                                myFours = createList(temp);
                            }else{
                                insertItem(temp, myFours);
                            }
                            k = 0;
                            while (k < searchResultFourMe.numBlockedOneSide){
                                pointValue+= BLOCKED_FOUR;
                                k++;
                            }
                        }
                    }
                    
                    really_winning = 0;
                    
                    if (winning(searchResultThreeEnemy,searchResultFourEnemy)){
                        i_j.x = i;
                        i_j.y = j;
                        
                        if (!((searchResultFourEnemy.numUnblocked && searchResultFourEnemy.existUnspaced)  || searchResultFourEnemy.numBlockedOneSide >= 2)){
                            
                            really_winning = reallyWinning(searchResultFourEnemy.numBlockedOneSide, searchResultThreeEnemy.numUnblocked,i_j , g, enemy);
                            if (really_winning && searchResultFourEnemy.numBlockedOneSide){
                                maxWinningStringEnemy = 4;
                            }else{
                                maxWinningStringEnemy = 3;
                            }
                        }else{
                            maxWinningStringEnemy = 4;
                            really_winning = 1;
                        }
                    }
                    if(really_winning){
                        
                        //printf("(%d,%d),enemy\n",i,j);
                        
                        
                        
                        numWinningMovesEnemy++;
                        temp = malloc(sizeof(Move));
                        temp->x = i;
                        temp->y = j;
                        
                        if (numWinningMovesEnemy == 1){
                            winningMovesEnemy = createList(temp);
                        }else{
                            insertItem(temp, winningMovesEnemy);
                        }
                        
                    }else{
                        
                        
                        
                        if (searchResultThreeEnemy.found){
                            numThreesEnemy++;
                            temp = malloc(sizeof(Move));
                            temp->x = i;
                            temp->y = j;
                            if (numThreesEnemy == 1){
                                enemyThrees = createList(temp);
                            }else{
                                insertItem(temp, enemyThrees);
                            }
                            
                            k = 0;
                            while (k < searchResultThreeEnemy.numUnblocked){
                                pointValue -= UNBLOCKED_THREE;
                                k++;
                            }
                            k = 0;
                            while (k < searchResultThreeEnemy.numBlockedOneSide){
                                pointValue -= BLOCKED_THREE;
                                k++;
                            }
                        }
                        
                        if (searchResultFourEnemy.found){
                            numFoursEnemy++;
                            temp = malloc(sizeof(Move));
                            temp->x = i;
                            temp->y = j;
                            if (numFoursEnemy == 1){
                                enemyFours = createList(temp);
                            }else{
                                insertItem(temp, enemyFours);
                            }
                            k = 0;
                            while (k < searchResultFourEnemy.numBlockedOneSide){
                                pointValue -= BLOCKED_FOUR;
                                k++;
                            }
                        }
                    }
                    
                    
                    
                    searchResultTwoMe = searchAround(i, j, g, 2, me,threadID);
                    searchResultTwoEnemy = searchAround(i, j, g, 2, enemy,threadID);
                    if (searchResultTwoMe.found){
                        k = 0;
                        while (k < searchResultTwoMe.numUnblocked){
                            pointValue += UNBLOCKED_TWO;
                            k++;
                        }
                    }
                    
                    if (searchResultTwoEnemy.found){
                        k = 0;
                        while (k < searchResultTwoEnemy.numUnblocked){
                            pointValue -= UNBLOCKED_TWO;
                            k++;
                        }
                    }
                    value +=pointValue;
                }
            }
            j++;
        }
        j = 0;
        i++;
    }
    
    if (numWinningMovesMe)numWinningMovesMe = shrink(numWinningMovesMe,winningMovesMe,g,me,1,threadID);
    if (numWinningMovesEnemy)numWinningMovesEnemy =  shrink(numWinningMovesEnemy, winningMovesEnemy, g, enemy,1,threadID);
    
    // if (numThreesMe && whoseTurn(g) == enemy) shrink(numThreesMe,myThrees,g,me,0,threadID);
    //if (numThreesEnemy && whoseTurn(g) == me) shrink(numThreesEnemy,enemyThrees,g,enemy,0,threadID);
    // if (numFoursMe && whoseTurn(g) == enemy) shrink(numFoursMe,myFours,g,me,0,threadID);
    // if (numFoursEnemy && whoseTurn(g) == me) shrink(numFoursEnemy,enemyFours,g,enemy,0,threadID);
    
    value -= UNBLOCKED_THREE*unblocked_three_compensate[threadID];
    value -= BLOCKED_THREE*blocked_three_compensate[threadID];
    value -= BLOCKED_FOUR*blocked_four_compensate[threadID];
    
    
    if (whoseTurn(g) == me){
        if (numFiveMe){
            value = FIVE_VALUE*numFiveMe;
        }else if (numFiveEnemy >= 2){
            value = -FIVE_VALUE*numFiveEnemy;
        }
    }else{
        if (numFiveEnemy){
            value = -FIVE_VALUE*numFiveEnemy;
        }else if (numFiveMe >= 2){
            value = FIVE_VALUE*numFiveMe;
        }
    }
    
    if (value < FIVE_VALUE && value > -FIVE_VALUE ){
        
        if (maxWinningStringMe >maxWinningStringEnemy){
            numWinningMovesEnemy = 0;
        }else if (maxWinningStringEnemy > maxWinningStringMe){
            numWinningMovesMe = 0;
        }
        
        if (numWinningMovesMe){
            if (whoseTurn(g) == me){
                value = ALMOST_AS_GOOD*numWinningMovesMe;
            }else if (numWinningMovesMe >= 2 && !numWinningMovesEnemy){
                value = ALMOST_AS_GOOD*numWinningMovesMe;
            }
            
            
        }
        if (value < ALMOST_AS_GOOD && value > -ALMOST_AS_GOOD){
            if (numWinningMovesEnemy){
                if (whoseTurn(g) == enemy){
                    value = -ALMOST_AS_GOOD*numWinningMovesEnemy;
                }else if (numWinningMovesEnemy >= 2 && !numWinningMovesMe){
                    value = -ALMOST_AS_GOOD*numWinningMovesEnemy;
                }
            }
        }
        
    }
    
    if (numFiveMe)  destroyList(fivesMe,1);
    if (numFiveEnemy) destroyList(fivesEnemy,1);
    if (numWinningMovesMe) destroyList(winningMovesMe,1);
    if (numWinningMovesEnemy) destroyList(winningMovesEnemy,1);
    if (numFoursMe) destroyList(myFours,1);
    if (numFoursEnemy)destroyList(enemyFours, 1);
    if (numThreesMe) destroyList(myThrees, 1);
    if (numThreesEnemy) destroyList(enemyThrees, 1);
    
    return value;
}

boundary makeBound(Game g){
    boundary b;
    b.mostX = 0;
    b.mostY = 0;
    b.leastX = sideLength;
    b.leastY = sideLength;
    int i = 0;
    int j = 0;
    while (i < sideLength){
        while (j < sideLength){
            if (g->board[i][j] != EMPTY){
                if (i > b.mostX){
                    b.mostX = i;
                }
                if (i < b.leastX){
                    b.leastX = i;
                }
                if (j > b.mostY){
                    b.mostY = j;
                }
                if (j < b.leastY){
                    b.leastY = j;
                }
            }
            j++;
        }
        j = 0;
        i++;
    }
    if (b.mostX < sideLength - 2){
        b.mostX+=2;
    }else if (b.mostX < sideLength - 1){
        b.mostX+=1;
    }
    
    if (b.mostY < sideLength - 2){
        b.mostY+=2;
    }else if (b.mostY < sideLength - 1){
        b.mostY+=1;
    }
    
    if (b.leastX >= 2){
        b.leastX-=2;
    }else if (b.leastX >= 1){
        b.leastX -= 1;
    }
    
    if (b.leastY >= 2){
        b.leastY-=2;
    }else if (b.leastY >= 1){
        b.leastY-= 1;
    }
    return b;
}

int worthy(int x,int y,Game g){
    
    int i = x;
    int j = y;
    
    int found = 0;
    //   int num = 0;
    
    if (i <= 2){
        i = 0;
    }else{
        i -= 2;
    }
    if (j <= 2){
        j = 0;
    }else{
        j -= 2;
    }
    
    int k = 0;
    int l = 0;
    while (l < 5 && !outOfBound(i, j) && !found){
        while (k < 5 && !outOfBound(i, j) && !found){
            
            if (g->board[i][j] != EMPTY){
                //     num++;
                //}
                //if (moveNumber == 1 && num){
                found = 1;
                // }else{
                //   if (num == 2) found = 1;
            }
            i++;
            k++;
        }
        i -= k;
        j++;
        k = 0;
        l++;
    }
    
    return found;
}

int outOfBound(int x,int y){
    return (x < 0 || y < 0 || x >= sideLength || y>= sideLength);
}

int shrink(int oldNum,Node n,Game g,int me,int winningMove,int threadID){
    int newNum = oldNum;
    stats searchResultThree;
    stats searchResultFour;
    
    stats searchResultThreeOld;
    stats searchResultFourOld;
    
    int enemy;
    if (me == BLACK){
        enemy = WHITE;
    }else {
        enemy = BLACK;
    }
    
    
    Move* temp;
    Node current = n;
    Node last = current;
    assert(n != NULL);
    while (n!= NULL){
        temp = (Move*)key(n);
        g->board[temp->x][temp->y] = enemy;
        if (g->whoseTurn == me){
            g->whoseTurn = enemy;
        }else{
            g->whoseTurn = me;
        }
        last = n;
        current = nextItem(n);
        while (current != NULL){
            //take the blocking stone away for now and compare the new and old stats
            temp = (Move*)key(current);
            searchResultThree = searchAround(temp->x, temp->y, g, 3, me,threadID);
            searchResultFour = searchAround(temp->x, temp->y, g, 4, me,threadID);
            temp = (Move*)key(n);
            g->board[temp->x][temp->y] = EMPTY;
            if (g->whoseTurn == me){
                g->whoseTurn = enemy;
            }else{
                g->whoseTurn = me;
            }
            temp = (Move*)key(current);
            searchResultThreeOld = searchAround(temp->x, temp->y, g, 3, me,threadID);
            searchResultFourOld = searchAround(temp->x, temp->y, g, 4, me,threadID);
            temp = (Move*)key(n);
            g->board[temp->x][temp->y] = enemy;
            if (g->whoseTurn == me){
                g->whoseTurn = enemy;
            }else{
                g->whoseTurn = me;
            }
            
            temp = (Move*)key(current);
            if (winningMove){
                if (!winning(searchResultThree,searchResultFour)){
                    //delete the node && go to the next one
                    
                    current = deleteItem(0, current,last);
                    newNum--;
                }else{
                    last = current;
                    current = nextItem(current);
                    
                }
            }else{
                
                if (me == g->whoseTurn){
                    unblocked_three_compensate[threadID] -= searchResultThreeOld.numUnblocked-searchResultThree.numUnblocked;
                    blocked_three_compensate[threadID] -= searchResultThreeOld.numBlockedOneSide
                    -searchResultThree.numBlockedOneSide;
                    blocked_four_compensate[threadID] -= searchResultFourOld.numBlockedOneSide - searchResultFour.numBlockedOneSide;
                }else{
                    unblocked_three_compensate[threadID] += searchResultThreeOld.numUnblocked-searchResultThree.numUnblocked;
                    blocked_three_compensate[threadID] += searchResultThreeOld.numBlockedOneSide
                    -searchResultThree.numBlockedOneSide;
                    blocked_four_compensate[threadID] += searchResultFourOld.numBlockedOneSide - searchResultFour.numBlockedOneSide;
                }
                last = current;
                current = nextItem(current);
                
                
            }
        }
        temp = (Move*)key(n);
        g->board[temp->x][temp->y] = EMPTY;
        if (g->whoseTurn == me){
            g->whoseTurn = enemy;
        }else{
            g->whoseTurn = me;
        }
        n = nextItem(n);
    }
    return newNum;
}

int winning(stats three,stats four){
    int winning;
    winning = ((three.found && three.numUnblocked >= 2)
               ||(four.found && four.numUnblocked>=1 && (four.existUnspaced))
               ||(four.found && three.found
                  && (four.numBlockedOneSide >= 1) && three.numUnblocked>=1)
               ||(four.found && four.found
                  && four.numBlockedOneSide >= 2));
    return winning;
}

int reallyWinning(int four, int three, Move position, Game g,int player){
    char * fake_four_three_left;
    char * fake_four_three_right;
    char * fake_four_three_mk2_left;
    char * fake_four_three_mk2_right;
    
    int originalFour = four;
    
    if (four){
        fake_four_three_left = malloc(sizeof(char)*3);
        fake_four_three_right = malloc(sizeof(char)*3);
        fake_four_three_mk2_left = malloc(sizeof(char));
        fake_four_three_mk2_right = malloc(sizeof(char)*3);
    }
    
    char * fake_three_three_left = malloc(sizeof(char)*2);
    char * fake_three_three_right = malloc(sizeof(char)*3);
    
    
    if (player == BLACK){
        if (four){
            strncpy(fake_four_three_left," **",3);
            strncpy(fake_four_three_right,"* *",3);
            strncpy(fake_four_three_mk2_left," *",1);
            strncpy(fake_four_three_mk2_right,"* *",3);
        }
        strncpy(fake_three_three_left," *",2);
        strncpy(fake_three_three_right,"* *",3);
        
    }else{
        if (four){
            strncpy(fake_four_three_left," OO",3);
            strncpy(fake_four_three_right,"O O",3);
            strncpy(fake_four_three_mk2_left," O",1);
            strncpy(fake_four_three_mk2_right,"O O",3);
        }
        strncpy(fake_three_three_left," O",2);
        strncpy(fake_three_three_right,"O O",3);
        
    }
    int four_three_eliminate = 0;
    
    if (four){
        four_three_eliminate = (search_for_pattern(&doNothing, &incr, fake_four_three_left, position, g, 3) &&
                                search_for_pattern(&doNothing, &decr, fake_four_three_right, position, g, 3)) +
        (search_for_pattern(&incr, &incr, fake_four_three_left, position, g, 3) &&
         search_for_pattern(&decr, &decr, fake_four_three_right, position, g, 3)) +
        (search_for_pattern(&incr, &doNothing, fake_four_three_left, position, g, 3) &&
         search_for_pattern(&decr, &doNothing, fake_four_three_right, position, g, 3)) +
        (search_for_pattern(&incr, &decr, fake_four_three_left, position, g, 3) &&
         search_for_pattern(&decr, &incr, fake_four_three_right, position, g, 3)) +
        (search_for_pattern(&doNothing, &decr, fake_four_three_left, position, g, 3) &&
         search_for_pattern(&doNothing, &incr, fake_four_three_right, position, g, 3)) +
        (search_for_pattern(&decr, &decr, fake_four_three_left, position, g, 3) &&
         search_for_pattern(&incr, &incr, fake_four_three_right, position, g, 3)) +
        (search_for_pattern(&decr, &doNothing, fake_four_three_left, position, g, 3) &&
         search_for_pattern(&incr, &doNothing, fake_four_three_right, position, g, 3)) +
        (search_for_pattern(&decr, &incr, fake_four_three_left, position, g, 3) &&
         search_for_pattern(&incr, &decr, fake_four_three_right, position, g, 3));
        
        
        four_three_eliminate += (search_for_pattern(&doNothing, &incr, fake_four_three_mk2_left, position, g, 1) &&
                                 search_for_pattern(&doNothing, &decr, fake_four_three_mk2_right, position, g, 3)) +
        (search_for_pattern(&incr, &incr, fake_four_three_mk2_left, position, g, 1) &&
         search_for_pattern(&decr, &decr, fake_four_three_mk2_right, position, g, 3)) +
        (search_for_pattern(&incr, &doNothing, fake_four_three_mk2_left, position, g, 1) &&
         search_for_pattern(&decr, &doNothing, fake_four_three_mk2_right, position, g, 3)) +
        (search_for_pattern(&incr, &decr, fake_four_three_mk2_left, position, g, 1) &&
         search_for_pattern(&decr, &incr, fake_four_three_mk2_right, position, g, 3)) +
        (search_for_pattern(&doNothing, &decr, fake_four_three_mk2_left, position, g, 1) &&
         search_for_pattern(&doNothing, &incr, fake_four_three_mk2_right, position, g, 3)) +
        (search_for_pattern(&decr, &decr, fake_four_three_mk2_left, position, g, 1) &&
         search_for_pattern(&incr, &incr, fake_four_three_mk2_right, position, g, 3)) +
        (search_for_pattern(&decr, &doNothing, fake_four_three_mk2_left, position, g, 1) &&
         search_for_pattern(&incr, &doNothing, fake_four_three_mk2_right, position, g, 3)) +
        (search_for_pattern(&decr, &incr, fake_four_three_mk2_left, position, g, 1) &&
         search_for_pattern(&incr, &decr, fake_four_three_mk2_right, position, g, 3));
        
    }
    
    int dual_three_eliminate = (search_for_pattern(&doNothing, &incr, fake_three_three_left, position, g, 2) &&
                                search_for_pattern(&doNothing, &decr, fake_three_three_right, position, g, 3)) +
    (search_for_pattern(&incr, &incr, fake_three_three_left, position, g, 2) &&
     search_for_pattern(&decr, &decr, fake_three_three_right, position, g, 3)) +
    (search_for_pattern(&incr, &doNothing, fake_three_three_left, position, g, 2) &&
     search_for_pattern(&decr, &doNothing, fake_three_three_right, position, g, 3)) +
    (search_for_pattern(&incr, &decr, fake_three_three_left, position, g, 2) &&
     search_for_pattern(&decr, &incr, fake_three_three_right, position, g, 3)) +
    (search_for_pattern(&doNothing, &decr, fake_three_three_left, position, g, 2) &&
     search_for_pattern(&doNothing, &incr, fake_three_three_right, position, g, 3)) +
    (search_for_pattern(&decr, &decr, fake_three_three_left, position, g, 2) &&
     search_for_pattern(&incr, &incr, fake_three_three_right, position, g, 3)) +
    (search_for_pattern(&decr, &doNothing, fake_three_three_left, position, g, 2) &&
     search_for_pattern(&incr, &doNothing, fake_three_three_right, position, g, 3)) +
    (search_for_pattern(&decr, &incr, fake_three_three_left, position, g, 2) &&
     search_for_pattern(&incr, &decr, fake_three_three_right, position, g, 3));
    
    
  //  printf("dual three eliminate is %d\n",dual_three_eliminate);
    
    three -= dual_three_eliminate*2 + four_three_eliminate;
    four -= four_three_eliminate;
    
    if (originalFour){
        free(fake_four_three_left);
        free(fake_four_three_right);
        free(fake_four_three_mk2_left);
        free(fake_four_three_mk2_right);
    }
    free(fake_three_three_left);
    free(fake_three_three_right);
    
    return !((three == 0) && (four == 0));
}

char numToChar(int num){
    if (num == BLACK) return '*';
    if (num == WHITE) return 'O';
    if (num == EMPTY) return ' ';
    return '\0';
}

int search_for_pattern(int (*fx)(int),int (*fy)(int), char* pattern, Move position, Game g,int length){
    
    
    
    int found = 0;
    int wrong = 0;
    int i = fx(position.x);
    int j = fy(position.y);
    int k = 0;
    while (!outOfBound(i,j) && k < length && !wrong){
        if (pattern[k] != numToChar(g->board[i][j])){
            wrong = 1;
        }else{
            i = fx(i);
            j = fy(j);
            k++;
        }
    }
    found = (k == length);
    if (found && !outOfBound(i,j)){
        found = found && (pattern[k - 1] != numToChar(g->board[i][j]));
    }
    return found;
}