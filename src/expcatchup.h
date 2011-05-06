typedef enum _CatchupConst {
   CatchupStop = 0,
   CatchupNormal = 8,
   CatchupDiscretionary = 9,
   CatchupMax = 9,
} CatchupConst;

void catchup_set( char* _expHome, int catchup );
int catchup_get( char* _expHome);
