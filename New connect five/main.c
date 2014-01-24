//
//  main.c
//  Connect-five
//
//  Created by Qiwei Wen on 15/01/13.
//  Copyright (c) 2013 Qiwei Wen. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include "Game.h"
#include "List.h"
#include "time.h"
#include <unistd.h>
#include <assert.h>
#include "threadPool.h"
#include <unistd.h>
#include "hashTable.h"

#define MAX_COMMAND_LENGTH 100
#define MAX_OPCODE_LENGTH 10
#define PASSWORD 19931103



//will be changed
int admin = 1;

//arbitrary function, used for testing the thread allocator
//the usleep command simulates the actual calculation time needed
int sqr(int arg1, int arg2, int arg3, void* input,int arg5, int arg6,int arg7,U64 arg8){
   // usleep(rand()%100);
    return arg1*arg1;
}

Node playerMoves;
Node AImoves;


int main(int argc, const char * argv[])
{
    int i = 0;
    int j = 0;
    int k = 0;
    
    moveNumber = 0;

    
    threads_initialise();
    srand((unsigned)time(NULL));
    chancesUsed = 0;
    lastMoveAI.x= 0;
    lastMoveAI.y = 0;
    lastMovePlayer.x = 0;
    lastMovePlayer.y = 0;
    
    numUnblocked = malloc(sizeof(int)*NUM_THREADS);
    numBlockedOneSide = malloc(sizeof(int)*NUM_THREADS);
    numBlockedBothSides = malloc(sizeof(int)*NUM_THREADS);
    currentlySearching= malloc(sizeof(int)*NUM_THREADS);
    whichSide = malloc(sizeof(int)*NUM_THREADS);
    alreadyBlocked = malloc(sizeof(int)*NUM_THREADS);
    spacedOnOneSide = malloc(sizeof(int)*NUM_THREADS);
    spaced = malloc(sizeof(int)*NUM_THREADS);
    unspaced = malloc(sizeof(int)*NUM_THREADS);
    
    unblocked_three_compensate = malloc(sizeof(int)*NUM_THREADS);
    blocked_three_compensate = malloc(sizeof(int)*NUM_THREADS);
    blocked_four_compensate = malloc(sizeof(int)*NUM_THREADS);
    
    i = 0;
    while (i < NUM_THREADS){
        numUnblocked[i] = 0;
        numBlockedOneSide[i] = 0;
        numBlockedBothSides[i] = 0;
        currentlySearching[i] = 0;
        whichSide[i] = 1;
        alreadyBlocked[i] = 0;
        spacedOnOneSide[i] = 0;
        spaced[i] = 0;
        unspaced[i] = 0;
        
        unblocked_three_compensate[i] = 0;
        blocked_three_compensate[i] = 0;
        blocked_four_compensate[i] = 0;
        
        i++;
    }
    
    Game g;
    Move m;
    int received = 0;
    char* command = malloc(sizeof(char)*MAX_COMMAND_LENGTH);
    
    while (!received){
        printf("please specify the board size (max 15, min 10)\n->");
        fgets(command,MAX_COMMAND_LENGTH,stdin);
        received = sscanf(command,"%d",&sideLength);
        
        if (sideLength > 15 || sideLength < 10) received = 0;
    }
    
    
    globalTable = newTable();
    
    received = 0;
    while (!received){
        printf("type in 1 if you'd like to play as black, or 2 otherwise\n->");
        fgets(command,MAX_COMMAND_LENGTH,stdin);
        received = sscanf(command,"%d",&playerColour);
        if (playerColour != 1 && playerColour != 2) received = 0;
    }
    if (playerColour == BLACK) AIcolour = WHITE;
    if (playerColour == WHITE) AIcolour = BLACK;
    
    rootNodePlayer = playerColour;
    
    int** board = malloc(sideLength*sizeof(int*));
    i = 0;
    j = 0;
    while (i < sideLength){
        board[i] = malloc(sideLength*sizeof(int));
        while (j<sideLength){
            board[i][j] = 0;
            j++;
        }
        j = 0;
        i++;
    }
    i = 0;
    
    g = createNewGame(BLACK, board, 0, 0);
    if (playerColour == WHITE){
        m.x = sideLength/2;
        m.y = sideLength/2;
        advanceGame(BLACK, m.x, m.y, g,0);
    }
    
    i = 0;
    j = 0;
    k = 0;
    eval_result = malloc(sizeof(int**)*sideLength);
    while (i < sideLength){
        eval_result[i] = malloc(sizeof(int*)*sideLength);
        while (j < sideLength){
            eval_result[i][j] = malloc(sizeof(int)*NUM_THREADS);
            while (k < NUM_THREADS){
                eval_result[i][j][k] = 0;
                k++;
            }
            k = 0;
            j++;
        }
        j = 0;
        k = 0;
        i++;
    }

    
    received = 0;
    while (isEnded(g) == FALSE){
        printf("\n");
        printGame(g);
        while (received == 0){
            printf("->");
            fgets(command,MAX_COMMAND_LENGTH,stdin);
            if (processCommand(command,g)) {
                received = 1;
            }else{
                if (strlen(command)!=1){
                    command[strlen(command) - 1] = '\0';
                }
                printf("!INVALID COMMAND '%s' \n",command);
                
            }
        }
        received = 0;
    }
    printf("\n");
    printGame(g);
    destroyGame(g);
    destroyList(playerMoves,1);
    destroyList(AImoves,1);
    thread_end();
    
    i = 0;
    j = 0;

    while (i < sideLength){
        while (j < sideLength){
            free(eval_result[i][j]);
            j++;
        }
        free(eval_result[i]);
        j = 0;
        i++;
    }
    
     destroyTable(globalTable);
    
    free(numUnblocked);
    free(numBlockedOneSide);
    free(numBlockedBothSides);
    free(currentlySearching);
    free( whichSide);
    free( alreadyBlocked);
    free( spacedOnOneSide);
    free( spaced);
    free( unspaced);
    
    free( unblocked_three_compensate);
    free(blocked_three_compensate);
    free(blocked_four_compensate);
    return 0;
}

int processCommand(char* command,Game g){
    int read_success = 1;
    int operand1;
    int operand2;
    int operand3;
    int numArg = 0;
    Move* tempMove;
    
    
    if (command != NULL){
        char* opcode = malloc(sizeof(char)*MAX_OPCODE_LENGTH);
        
        int i = 0;
        
        while (i < MAX_OPCODE_LENGTH){
            opcode[i] = '\0';
            i++;
        }
        i = 0;
        while (command[i] != 32 && command[i] != '\n' && i < MAX_OPCODE_LENGTH && i < strlen(command)){
            opcode[i] = command[i];
            i++;
        }
        while (command[i] == ' ' && i < strlen(command)){
            i++;
        }
        
        if (!strcmp(opcode,"p")){
            numArg = sscanf(&command[i],"%d %d",&operand1,&operand2);
            if (numArg != 2){
                read_success = 0;
            }else{
                if (operand1 < sideLength && operand2 < sideLength
                    &&operand1 >= 0 && operand2 >=0 && inquire(operand1,operand2,g)==EMPTY){
                    moveNumber++;
                    advanceGame(whoseTurn(g), operand1, operand2, g,0);
                    lastMovePlayer.x = operand1;
                    lastMovePlayer.y = operand2;
                    if (!(isEnded(g))){
                        Move m = decideMove(g);
                        lastMoveAI = m;
                        advanceGame(whoseTurn(g), m.x, m.y, g,0);
                        if (moveNumber == 1){
                            tempMove = malloc(sizeof(Move));
                            *tempMove = lastMovePlayer;
                            playerMoves = createList(tempMove);
                            tempMove = malloc(sizeof(Move));
                            *tempMove = lastMoveAI;
                            AImoves = createList(tempMove);
                        }else{
                            tempMove = malloc(sizeof(Move));
                            tempMove->x = m.x;
                            tempMove->y = m.y;
                            insertItem(tempMove, AImoves);
                            tempMove = malloc(sizeof(Move));
                            tempMove->x = operand1;
                            tempMove->y = operand2;
                            insertItem(tempMove,playerMoves);
                        }
                    }
                }else{
                    printf("invalid argument(s) for command 'put'\n");
                }
            }
        }else if (!strcmp(opcode,"w")) {
            
            if (chancesUsed < MAX_CHANCES && moveNumber != 0){
                
                if (!admin) chancesUsed++;
                moveNumber--;
                
                tempMove = pop(playerMoves);
                lastMovePlayer.x = tempMove->x;
                lastMovePlayer.y = tempMove->y;
                tempMove = pop(AImoves);
                lastMoveAI.x = tempMove->x;
                lastMoveAI.y = tempMove->y;
                withDraw(g);
            }else{
                printf("\nyou cannot revert now\n");
            }
        }else if (!strcmp(opcode,"q")){
            endGame(g);
            printf("GAME OVER\n");
        }else if (!strcmp(opcode,"t")){
            if (!admin){
                printf("Please type in the password\n");
                int password;
                char* pw_string = malloc(9*sizeof(char));
                fgets(pw_string,9,stdin);
                sscanf(pw_string,"%d",&password);
                if (password == PASSWORD){
                    admin = 1;
                    printf("admin logged in. Test mode enabled\n");
                }
                getchar();
            }else{
                printf("Already logged in.\n");
            }
        }else if (!strcmp(opcode,"pl")){
            if (admin){
                numArg = sscanf(&command[i],"%d %d",&operand1,&operand2);
                if (numArg != 2){
                    read_success = 0;
                }else{
                    if (operand1 < sideLength && operand2 < sideLength
                        &&operand1 >= 0 && operand2 >=0 && inquire(operand1,operand2,g)==EMPTY){
                        if (whoseTurn(g) == AIcolour){
                            moveNumber++;
                        }
                        advanceGame(whoseTurn(g),operand1, operand2, g,0);
                        if (whoseTurn(g) == playerColour){
                            tempMove = malloc(sizeof(Move));
                            tempMove->x = operand1;
                            tempMove->y = operand2;
                            if (moveNumber == 1){
                                AImoves = createList(tempMove);
                            }else{
                                insertItem(tempMove,AImoves);
                            }
                        }else{
                            tempMove = malloc(sizeof(Move));
                            tempMove->x = operand1;
                            tempMove->y = operand2;
                            if (moveNumber == 0){
                                playerMoves = createList(tempMove);
                            }else{
                                insertItem(tempMove,playerMoves);
                            }
                        }
                    }else{
                        printf("invalid argument(s) for command 'place'\n");
                    }
                }
            }else{
                printf("Authorisation required. Get aurhorised by using the command 't'\n");
            }
            
        }else if (!strcmp(opcode,"eval")){
            if (admin){
                rootNodePlayer = whoseTurn(g);
                //rootNodePlayer = 2;
                printf("the worth of the current state is %d\n",evaluate(g,0));
            }else{
                printf("Authorisation required. Get aurhorised by using the command 't'\n");
            }
        }else if (!strcmp(opcode,"best_move")){
            if (admin){
                rootNodePlayer = whoseTurn(g);
                tempMove = malloc(sizeof(Move));
                *tempMove = decideMove(g);
                printf("The best move for the current player is (%d,%d)\n",tempMove->x,tempMove->y);
                free(tempMove);
            }else{
                printf("Authorisation required. Get aurhorised by using the command 't'\n");
            }
        }else if (!strcmp(opcode,"boundary")){
            if (admin){
                boundary b = makeBound(g);
                printf("boundaries are (%d,%d),(%d,%d)\n",b.mostX,b.mostY,b.leastX,b.leastY);
            }else{
                printf("Authorisation required. Get aurhorised by using the command 't'\n");
            }
        }else if (!strcmp(opcode,"nextMoves")){
            if (admin){
                Node n = generateNextMoves(g);
                printList(n);
            }else{
                printf("Authorisation required. Get aurhorised by using the command 't'\n");
            }
        }else if (!strcmp(opcode,"stats")){
            if (admin){
                numArg = sscanf(&command[i],"%d %d %d",&operand1,&operand2,&operand3);
                if (numArg != 3){
                    read_success = 0;
                }else{
                    if (operand1 < sideLength && operand2 < sideLength
                        &&operand1 >= 0 && operand2 >=0 && operand3 >= 2 && operand3 <=5 && inquire(operand1,operand2,g)==EMPTY){
                        stats s = searchAround(operand1, operand2, g, operand3, whoseTurn(g),0);
                        printf("statistics of point (%d,%d), required string length: %d\n",operand1,operand2,operand3);
                        printf("number of unblocked %d in a row: %d\n",operand3,s.numUnblocked);
                        printf("number of %d in a row blocked on one side %d\n",operand3,s.numBlockedOneSide);
                        printf("number of %d in a row blocked on both sides %d\n",operand3,s.numBlockedBothSides);
                        printf("existSpaced is %d\n",s.existSpaced);
                        printf("existUnspaced is %d\n",s.existUnspaced);
                    }else{
                        printf("invalid argument(s) for command 'statistics.'\n");
                    }
                }
            }else{
                printf("Authorisation required. Get aurhorised by using the command 't'\n");
            }
        }else if (!strcmp(opcode,"pl_test")){
            
            processCommand("pl 9 8",g);
    
            processCommand("pl 10 8",g);
            
            processCommand("pl 9 7", g);
            
            
            
            processCommand("pl 9 9", g);
            
            processCommand("pl 9 6", g);
            
            
            
            processCommand("pl 10 9", g);
            
            
        }else if (!strcmp(opcode,"a")){
            int i = 0;
            task* t;
            while (i < 1000){
                t = malloc(sizeof(task));
                t->arg1 = i;
                t->func = &sqr;
                queue_add_task(t);
                i++;
            }
            i = 0;
            queue_add_task(NULL);
            task* fetch;
            while (!workDone){
                usleep(10);
                pthread_mutex_lock(mutex_queue);
                fetch = queue_do_task(workQueue);
                pthread_mutex_unlock(mutex_queue);
                if (fetch != NULL){
                    fetch->result = fetch->func(fetch->arg1,fetch->arg2,fetch->arg3,fetch->arg4,0,fetch->arg6,fetch->arg7,fetch->arg8);
                    i++;
                    taskNum--;
                    tempNum--;
                    if (taskNum == 0){
                        workDone = 1;
                    }
                }
                 
            }
            printf("main thread did %d lot of work\n",i);
            printResult(original);
/*
            
            processCommand("pl 9 8",g);
            processCommand("pl 6 6",g);
            
            processCommand("pl 9 7", g);
            
            
            
            processCommand("pl 9 9", g);
            
            processCommand("pl 9 6", g);
            
            
            
            processCommand("pl 10 9", g);
            
            processCommand("pl 8 7", g);
            
            
            
            processCommand("pl 11 11", g);
            
            processCommand("pl 7 6", g);
            
            
            
            processCommand("pl 11 7", g);
            
            
            
            processCommand("pl 11 9", g);
            
            
            
            processCommand("pl 11 6", g);
            
            processCommand("pl 4 4", g);
            
     */     
            
            
             
        }else{
            read_success = 0;
        }
    }
    return read_success;
}
