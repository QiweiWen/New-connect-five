//
//  Game.h
//  Connect-five
//
//  Created by Qiwei Wen on 15/01/13.
//  Copyright (c) 2013 Qiwei Wen. All rights reserved.
//

#ifndef Connect_five_Game_h
#define Connect_five_Game_h

#define BLACK 1
#define WHITE 2
#define EMPTY 0
#define CONFIRM 3

#define MAX_CHANCES 3

#define NUM_THREADS 3
#include <pthread.h>

typedef long U64;

pthread_mutex_t* mutex_eval_chart;

int sideLength;
int playerColour;
int AIcolour;
int rootNodePlayer;

int moveNumber;

//global variables used for shape searching
int* numUnblocked;
int* numBlockedOneSide;
int* numBlockedBothSides;
int* currentlySearching;
int* whichSide;
int* alreadyBlocked;
int* spacedOnOneSide;
int* spaced;
int* unspaced;

int* unblocked_three_compensate;
int* blocked_three_compensate;
int* blocked_four_compensate;
//


//pointer to the game struct
typedef struct _game* Game;
typedef struct _node* Node;

typedef struct _move{
    int x;
    int y;
}Move;

typedef struct _searchAround_result stats;

typedef struct _search_result searchInfo;

struct _searchAround_result{
    int found;
    int numBlockedOneSide;
    int numBlockedBothSides;
    int numUnblocked;
    int existSpaced;
    int existUnspaced;
};

struct _boundary{
    int mostX;
    int mostY;
    int leastX;
    int leastY;
};

typedef struct _boundary boundary;

Move lastMovePlayer;
Move lastMoveAI;
int chancesUsed;

//returns whether tha game is ended
int isEnded(Game g);

int whoseTurn(Game g);

int inquire(int x, int y, Game g);

//create a new game, return the pointer to the new game
Game createNewGame(int whoseTurn,int** board,int terminalScore,int ended);

//make a move and check if the game ends
void advanceGame(int player,int x,int y,Game g,int threadID);

void put(int x,int y, Game g,int player);

//unmake a move
void unmakeMove(int x, int y, Game g);

//take a move back
void withDraw(Game g);

//duplicate a board
int** duplicate(int**);

//end a game and release all the space allocated
void destroyGame(Game g);

//search in a specified direction from a point for a specified number of pieces in a row
searchInfo search(int (*fx)(int), int (*fy)(int), int number,int x,int y,Game g,int threadID);

//search around a point for a specific number of pieces in a row
stats searchAround(int x,int y,Game g,int number,int pretend,int threadID);

//add one
int incr(int);

//subtract one
int decr(int);

//return the number passed in
int doNothing(int);

//make a move in responce to the user's move
Move decideMove(Game g);

//the game tree search algorithm
int max(int depth, int alpha, int beta, void* g,int threadID,int principal,int searchForEndstate,U64 oldKey);

int min(int depth, int alpha, int beta, void* g,int threadID,int principal,int searchForEndState,U64 oldKey);

//return the value of the current state
int evaluate(Game g,int threadID);

//print out the Game
void printGame(Game g);

//force-end the game
void endGame(Game g);

//return the boundary for evaluation
boundary makeBound(Game g);

int worthy(int x,int y,Game g);

int outOfBound(int x,int y);

//takes in a list of moves && check mutual independency
//by blocking && unblocking each.
int shrink(int oldNum,Node n,Game g,int me,int winningMove,int threadID);

//if a move constitutes a winning situation
int winning(stats three,stats four);

//a function which aims to avoid points like "O|*|*| |.|*| |*|" from being treated as winning moves
int reallyWinning(int four, int three, Move position, Game g,int player);

int search_for_pattern(int (*fx)(int),int (*fy)(int), char* pattern, Move position, Game g,int length);

char numToChar(int num);

int processCommand(char* command,Game g);
#endif
