#include <AppleEvents.h>

OSErr InitializeAppleEvents(AppleEvent *firstEvent, AppleEvent *firstReply);
void ChessAction(long eventID, char *msg, char *replymsg);

#ifdef __CONDITIONALMACROS__
#define AEFilterProcType_ Boolean
#else
#define AEFilterProcType_ short
#endif

extern pascal AEFilterProcType_ (*gAEIdleProc)(EventRecord *theEvent,
                                               long *sleepTime,
                                               RgnHandle *mouseRgn);
extern pascal AEFilterProcType_ (*gAEFilterProc)(EventRecord *theEvent,
                                                 long returnID,
                                                 long transactionID,
                                                 const AEAddressDesc *address);

pascal OSErr DoCHESEvent(const AppleEvent *event, AppleEvent *reply,
                         long refcon);
OSErr SendMessage(AEDesc *target, DescType eventID, char *msg, char *replymsg);

OSErr InitAppleEvents(AppleEvent *firstEvent, AppleEvent *firstReply);

/* Generic chess interface
   ----------------------- */

enum { WHITE, BLACK, NSIDE }; /* side */

enum { /* pieces */
       EMPTY,
       WPAWN,
       WROOK,
       WKNIGHT,
       WBISHOP,
       WQUEEN,
       WKING,
       BPAWN,
       BROOK,
       BKNIGHT,
       BBISHOP,
       BQUEEN,
       BKING,
       NPIECE
};

typedef struct {
  short theBoard[64]; /* piece on each square */
  short turn;         /* side to move */
  short cstat;        /* castle modes allowed */
  short enpas;        /* to-sq for en passant capture */
} Board;

typedef short SMove;

#define File(sq) ((sq)&7)
#define Rank(sq) ((sq) >> 3)
#define rankflip(x) ((x) + 56 - (((x) >> 3) << 4))

#define SQF(move) ((move >> 8) & 0x3F)
#define SQT(move) (move & 0x3F)
#define UPROM(move) ((move >> 6) & 0x03)
#define CAPTURE(move) (move & (1 << 14))
#define SPECIAL(move) (move & (1 << 15))
#define CAPMOVE (1 << 8)  /* add to u to set CAPTURE bit */
#define SPECMOVE (1 << 9) /* add to u to set SPECIAL bit */
#define SMOVE(sqf, sqt, u) (((sqf) << 8) + ((u) << 6) + (sqt))

void ParseSetup(Board *aBoard, char *str);
void FormatSetup(Board *aBoard, char *str);
SMove ParseMove(char *movstr);
char *FormatMove(SMove move);

short GetPiece(int sq);
short GenerateMoves(int piece, int sqf, SMove *movbuf, int maxmoves);
Boolean CheckMove(SMove move);
Boolean MateMove(SMove move);
