#ifndef _SEQ_LISTNODE
#define _SEQ_LISTNODE

struct listNode {
    char *data;
    struct listNode *nextPtr;
  };

typedef struct listNode LISTNODE;
typedef LISTNODE *LISTNODEPTR;

struct tokenNode {
   char *token;
   char *data;
   struct tokenNode *nextPtr;
};

typedef struct tokenNode TOKENNODE;
typedef TOKENNODE *TOKENNODEPTR;

/****************************************************************
*insertItem: Insert an item 's' into the list 'list'. The memory
*necessary to store the string 's' will be allocated by insertIem.
*****************************************************************/
void SeqListNode_insertItem(LISTNODEPTR *list, char *s);
void SeqListNode_insertTokenItem(TOKENNODEPTR *list, char *token, char *data);

/****************************************************************
*deleteItem: Delete the first item from the list 'list'. It returns
*the string that had been deleted. The memory will be deallocated by
*deleteItem.
*****************************************************************/
char* SeqListNode_deleteItem(LISTNODEPTR *list);

/****************************************************************
*isListEmpty: Returns 1 if list 'sPtr' is empty, eitherwise 0
*****************************************************************/
int SeqListNode_isListEmpty(LISTNODEPTR sPtr);
    
/********************************************************************************
* SeqListNode_isItemExists: returns true if the data already exists in the list
*                           otherwise returns false
********************************************************************************/
int SeqListNode_isItemExists(LISTNODEPTR sPtr, char *data);
int SeqListNode_isTokenItemExists(TOKENNODEPTR sPtr, char *token);
char *SeqListNode_getTokenData(TOKENNODEPTR sPtr, char *token);

/****************************************************************
*deleteWholeList: delete the list 'list' and deallocate the memories
*****************************************************************/
void SeqListNode_deleteWholeList(LISTNODEPTR *list);

/****************************************************************
*printList: print the list 'list'
*****************************************************************/
void SeqListNode_printList(LISTNODEPTR list);

/********************************************************************************
 * SeqListNode_reverseList: reverse the contents of the list
 ********************************************************************************/
void SeqListNode_reverseList(LISTNODEPTR *sPtr);

#endif

