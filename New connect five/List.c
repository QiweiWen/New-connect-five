//
//  List.c
//  Connect-five
//
//  Created by Qiwei Wen on 15/01/13.
//  Copyright (c) 2013 Qiwei Wen. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include "List.h"
#include "Game.h"


struct _node{
    void* key;
    Node next;
};


Node createList(void* g){
    Node newList = malloc(sizeof(struct _node));
    newList->key = g;
    newList->next = NULL;
    return newList;
}

Node nextItem(Node n){
    if (n!= NULL){
        n = n->next;
    }
    return n;
}

Node insertItem(void* g, Node n){
    Node temp = n;
    Node last = temp;
    while (temp != NULL){
        last = temp;
        temp = temp->next;
    }
    last->next = createList(g);
    return n;
}

Node deleteItem(int num,Node list,Node last){
    int i = 0;
    Node temp = list;
        
    if (list != NULL && num == 0){
        list = list->next;
        if (last != NULL) last->next = list;
        free(temp);
    }else{
        while (i < num && temp != NULL){
            last = temp;
            temp = temp->next;
            i++;
        }
        if (temp != NULL){
            last->next = temp->next;
            free(temp);
        }
    }
    return list;
}

void destroyList(Node list,int freeKey){
    Node temp = list;
    while (list != NULL){
        temp = list;
        list = list->next;
        if (temp->key != NULL && freeKey) free(temp->key);
        free(temp);
    }
}

void printList(Node n){
    Node current = n;
    while (current != NULL){
        printf("(%d,%d)/",((Move*)current->key)->x,((Move*)current->key)->y);
        //printGame((Game)(current->key));
        current = current->next;
    }
    printf("\n");
}


Node generateNextMoves(Game g){
    boundary b = makeBound(g);
    Node n;
    Move* m = malloc(sizeof(Move));
    m->x = -1;
    m->y = -1;
    n = createList(m);
    int i = 0;
    int j = 0;
    int count = 0;
    while (i <sideLength){
        while (j < sideLength ){
            if (inquire(i,j,g) == 0 && i <= b.mostX && i >= b.leastX && j <= b.mostY && j >= b.leastY && worthy(i,j,g)){
                m = malloc(sizeof(Move));
                m->x = i;
                m->y = j;
                insertItem(m, n);
                count++;
            }
            j++;
        }
        j = 0;
        i++;
    }
    
    return n;
}

void* key(Node n ){
    if (n != NULL){
        return n->key;
    }else{
        return NULL;
    }
}

void* pop(Node n){
    if (n != NULL){
        void* temp;
        if (n->next != NULL){
            Node current = n;
            Node last = current;
            
            while (current->next != NULL){
                last = current;
                current = current->next;
            }
            temp = current->key;
            free(current);
            last->next = NULL;
            return temp;
        }else{
            temp = n->key;
            free(n);
            return temp;
        }
    }else{
        return NULL;
    }
}

int isLast(Node n){
    if (n!= NULL){
        if (n->next == NULL){
            return 1;
        }else{
            return 0;
        }
    }
    return 1;
}

int size(Node n){
    Node current = n;
    int i = 0;
    while (current != NULL){
        i++;
        current = current->next;
    }
    return i;
}

Node insertionSortDescend(Node n,Game g,int threadID){
    int i = 0;
    int j = 0;
    
    Node current = n;
    Node last = n;
    Node temp;

    while (last != NULL){
        advanceGame(whoseTurn(g),((Move*)key(last))->x , ((Move*)key(last))->y, g,threadID);
        eval_result[((Move*)key(last))->x][((Move*)key(last))->y][threadID] = evaluate(g, threadID);
        unmakeMove(((Move*)key(last))->x, ((Move*)key(last))->y, g);
        last = last->next;
    }
    
   
    last = n;
    
    i = 0;
    while (current != NULL){
        
        if (moveValue(*(Move*)key(current),threadID)
            <= moveValue(*(Move*)key(last),threadID)){
            last = current;
            current = current->next;
        }else{
            
            if (last == n){
                n = current;
                last->next = current->next;
                current->next = last;
            }else{
                
                if (moveValue(*(Move*)key(current),threadID)
                    > moveValue(*(Move*)key(n),threadID)){
                    last->next = current->next;
                    current->next = n;
                    n = current;
                }else{
                
                    temp = n;
                    while (temp != last && moveValue(*(Move*)key(current),threadID)
                           < moveValue(*(Move*)key(temp->next),threadID)){
                        temp = temp->next;
                    }
                    last->next = current->next;
                    current->next = temp->next;
                    temp->next = current;
                }
                
            }
            current = last->next;

        }
        
        
    }
    
    
    temp = n;
    while (i < 9){
        temp = temp->next;
        i++;
    }
    
    Node anotherTemp = temp->next;
    temp->next = NULL;
    destroyList(anotherTemp, 1);
    
    
  //  printList_mVal(n, g, threadID);
    /*
    i = 0;
    j = 0;
    while (i < sideLength){
        while (j < sideLength){
            eval_result[i][j][threadID] = 0;
            j++;
        }
        j = 0;
        i++;
    }
     */
    return n;
}

Node insertionSortAscend(Node n,Game g,int threadID){
    int i = 0;
    int j = 0;
    
    Node current = n;
    Node last = n;
    Node temp;
    
    while (last != NULL){
        advanceGame(whoseTurn(g),((Move*)key(last))->x , ((Move*)key(last))->y, g,threadID);
        eval_result[((Move*)key(last))->x][((Move*)key(last))->y][threadID] = evaluate(g, threadID);
        unmakeMove(((Move*)key(last))->x, ((Move*)key(last))->y, g);
        last = last->next;
    }
    
    
    last = n;
    
    i = 0;
    while (current != NULL){
        
        if (moveValue(*(Move*)key(current),threadID)
            >= moveValue(*(Move*)key(last),threadID)){
            last = current;
            current = current->next;
        }else{
            
            if (last == n){
                n = current;
                last->next = current->next;
                current->next = last;
            }else{
                
                if (moveValue(*(Move*)key(current),threadID)
                    < moveValue(*(Move*)key(n),threadID)){
                    last->next = current->next;
                    current->next = n;
                    n = current;
                }else{
                    
                    temp = n;
                    while (temp != last && moveValue(*(Move*)key(current),threadID)
                           > moveValue(*(Move*)key(temp->next),threadID)){
                        temp = temp->next;
                    }
                    last->next = current->next;
                    current->next = temp->next;
                    temp->next = current;
                }
                
            }
            current = last->next;
            
        }
        
        
    }
   
    temp = n;
    while (i < 9){
        temp = temp->next;
        i++;
    }
    
    Node anotherTemp = temp->next;
    temp->next = NULL;
    destroyList(anotherTemp, 1);
   
    
  //  printList_mVal(n, g, threadID);
    /*
    i = 0;
    j = 0;
    while (i < sideLength){
        while (j < sideLength){
            eval_result[i][j][threadID] = 0;
            j++;
        }
        j = 0;
        i++;
    }
     */
    return n;
}



int moveValue(Move m,int threadID){
    return eval_result[m.x][m.y][threadID];
}
void printList_mVal(Node n,Game g,int threadID){
    Node current = n;
    while (current != NULL){
        printf("%d/",moveValue(*(Move*)key(current),threadID));
        current = current->next;
    }
    printf("\n");
}

int numItems(Node n){
    int num = 0;
    Node current = n;
    while (current != NULL){
        num++;
        current = current->next;
    }
    return num;
}
