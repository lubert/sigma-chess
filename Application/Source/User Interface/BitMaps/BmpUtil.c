/*
Copyright (c) 2011, Ole K. Christensen
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

� Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

� Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "BmpUtil.h"
#include "BoardView.h"

CBitmap *figurineBmp;
CBitmap *utilBmp;
CView *utilBmpView, *wSquareBmpView, *bSquareBmpView;

/**************************************************************************************************/
/*                                                                                                */
/*                                           2D PIECE SETS */
/*                                                                                                */
/**************************************************************************************************/

void CacheSrcBitMap(CView *source, INT x0, INT y0,
                    RGBColor c[minSquareWidth][minSquareWidth]);

void ScaleBitMap(RGBColor c[minSquareWidth][minSquareWidth],
                 INT size,  // Size (height and width) of small source bitmap
                 CView *dest, INT X0,
                 INT Y0,   // Top left corner of larger dest bitmap
                 INT Size  // Size (height and width) of larger dest bitmap
);

PieceBmp *pieceBmp1 = nil;  // The small/standard 42x42 pieces
CBitmap *pieceBmp2 = nil;   // The medium 50x50 pieces
CBitmap *pieceBmp3 = nil;   // The large 58x58 pieces
CBitmap *pieceBmp4 = nil;   // The even larger 64x64 pieces

CView *pieceBmpView1 = nil;
CView *pieceBmpView2 = nil;
CView *pieceBmpView3 = nil;
CView *pieceBmpView4 = nil;

PieceBmp::PieceBmp(INT pieceSet)
    : CBitmap(pieceSet + firstPieceSetID, 16) {} /* PieceBmp::PieceBmp */

void PieceBmp::LoadPieceSet(INT pieceSet) {
  if (pieceSet < pieceSet_Count)
    LoadPicture(pieceSet + firstPieceSetID);  // Standard built-in piece sets
  else
    LoadPieceSetPlugin(pieceSet - pieceSet_Count);  // Custom plug-in piece sets

  if (pieceBmpView2)
    pieceBmpView2->DrawRectFill(pieceBmpView2->bounds, &color_Blue);
  if (pieceBmpView3)
    pieceBmpView3->DrawRectFill(pieceBmpView3->bounds, &color_Blue);
  if (pieceBmpView4)
    pieceBmpView4->DrawRectFill(pieceBmpView4->bounds, &color_Blue);

  for (PIECE p = pawn; p <= king; p++)
    for (COLOUR c = white; c <= black; c += black) {
      CRect src = CalcPieceBmpRect(p + c, squareWidth1);
      CRect dst2 = CalcPieceBmpRect(p + c, squareWidth2);
      CRect dst3 = CalcPieceBmpRect(p + c, squareWidth3);
      CRect dst4 = CalcPieceBmpRect(p + c, squareWidth4);

      RGBColor C[squareWidth1][squareWidth1];
      CacheSrcBitMap(pieceBmpView1, src.left, src.top, C);
      if (pieceBmpView2)
        ScaleBitMap(C, squareWidth1, pieceBmpView2, dst2.left, dst2.top,
                    squareWidth2);
      if (pieceBmpView3)
        ScaleBitMap(C, squareWidth1, pieceBmpView3, dst3.left, dst3.top,
                    squareWidth3);
      if (pieceBmpView4)
        ScaleBitMap(C, squareWidth1, pieceBmpView4, dst4.left, dst4.top,
                    squareWidth4);
    }
} /* PieceBmp::LoadPieceSet */

CRect PieceBmp::CalcPieceRect(PIECE p) {
  CRect r(1, 1, 43, 43);
  if (p != empty)
    r.Offset((pieceType(p) - 1) * 43, (pieceColour(p) == white ? 0 : 43));
  return r;
} /* PieceBmp::CalcPieceRect */

CRect CalcPieceBmpRect(PIECE p, INT sqWidth) {
  sqWidth++;
  CRect r(1, 1, sqWidth, sqWidth);
  if (p != empty) {
    r.Offset((pieceType(p) - 1) * sqWidth, 0);
    if (pieceColour(p) == black) r.Offset(0, sqWidth);
  }
  return r;
} /* CalcPieceBmpRect */

/*---------------------------------- Piece Set Plug-ins
 * ------------------------------------------*/
// At launch time, Sigma Chess scans the ":Plug-ins:Piece Sets" directory for
// all files of type '�PST' and builds a list of these sets and their file
// names. A piece set file must be a resource file containing the following:
// 1. A 'PICT' resource with id 1000 (the actual piece image)
// 2. An optional 'cicn' resource with id 1000 (the display menu icon).  NOT
// IMPLEMENTED YET

#define maxPieceSetPlugins 32

static INT pieceSetPluginCount = 0;
static CHAR PieceSetPlugin[maxPieceSetPlugins][maxFileNameLen + 1];

void InitPieceSetPlugins(void) {
  pieceSetPluginCount = 0;

  CInfoPBRec cat;
  Str63 volName;
  OSErr err = noErr;
  INT vRefNum;
  LONG dirID;
  FSSpec fspec;
  Str255 ioName;

  C2P_Str(":Plug-ins:Piece Sets:Read me!.pdf", ioName);
  ::HGetVol(volName, &vRefNum, &dirID);
  err = ::FSMakeFSSpec(vRefNum, dirID, ioName, &fspec);

  cat.hFileInfo.ioNamePtr = ioName;
  cat.hFileInfo.ioVRefNum = vRefNum;

  for (INT i = 0; i < maxPieceSetPlugins && err == noErr; i++) {
    C2P_Str("", ioName);
    cat.hFileInfo.ioFDirIndex = i + 1;
    cat.dirInfo.ioDrDirID = fspec.parID;
    cat.hFileInfo.ioACUser = 0;

    err = ::PBGetCatInfoSync(&cat);

    if (err == noErr && !(cat.hFileInfo.ioFlAttrib & (1 << 4))) {
      FInfo *info = &(cat.hFileInfo.ioFlFndrInfo);
      if (info->fdType == '�PST' && info->fdCreator == 'RSED' &&
          ioName[1] != '(')
        P2C_Str(ioName, PieceSetPlugin[pieceSetPluginCount++]);
    }
  }
} /* InitPieceSetPlugins */

void AddPieceSetPlugins(CMenu *pm) {
  if (pieceSetPluginCount == 0) return;

  pm->AddSeparator();
  for (INT i = 0; i < pieceSetPluginCount; i++)
    pm->AddItem(PieceSetPlugin[i], pieceSet_Last + 1 + i);
} /* AddPieceSetPlugins */

INT PieceSetPluginCount(void) {
  return pieceSetPluginCount;
} /* PieceSetPluginCount */

void PieceBmp::LoadPieceSetPlugin(INT n)  // n = 0..9
{
  CHAR fileName[100];
  CFile file;

  Format(fileName, ":Plug-ins:Piece Sets:%s", PieceSetPlugin[n]);
  file.Set(fileName, '�PST');

  if (file.OpenRes(filePerm_Rd) == fileError_NoError) {
    LoadPicture(1000);
    file.CloseRes();
  }
} /* PieceBmp::LoadPieceSetPlugin */

/**************************************************************************************************/
/*                                                                                                */
/*                                         2D BOARD SQUARES */
/*                                                                                                */
/**************************************************************************************************/

CBitmap *wSquareBmp, *bSquareBmp;

static void LoadBoardTypePlugin(INT n);

void LoadSquareBmp(INT boardType) {
  if (boardType == 0) {
    wSquareBmpView->DrawRectFill(wSquareBmpView->bounds,
                                 &Prefs.Appearance.whiteSquare);
    bSquareBmpView->DrawRectFill(bSquareBmpView->bounds,
                                 &Prefs.Appearance.blackSquare);
  } else if (boardType < boardType_Count) {
    wSquareBmp->LoadPicture(2000 + boardType - 1);
    bSquareBmp->LoadPicture(2100 + boardType - 1);
  } else {
    LoadBoardTypePlugin(boardType - boardType_Count);
  }
} /* LoadSquareBmp */

/*------------------------------------------ Board Plug-ins
 * --------------------------------------*/
// At launch time, Sigma Chess scans the ":Plug-ins:Boards" directory for all
// files of type '�BRD' and builds a list of these boards and their file names.
// A board file must be a resource file containing the following:

// 1. A 'PICT' resource with id 1000 (the white square image)
// 2. A 'PICT' resource with id 1001 (the black square image)
// 3. An optional 'cicn' resource with id 1000 (the display menu icon).  NOT
// IMPLEMENTED YET

#define maxBoardTypePlugins 32

static INT boardTypePluginCount = 0;
static CHAR BoardTypePlugin[maxBoardTypePlugins][maxFileNameLen + 1];

void InitBoardTypePlugins(void) {
  boardTypePluginCount = 0;

  CInfoPBRec cat;
  Str63 volName;
  OSErr err = noErr;
  INT vRefNum;
  LONG dirID;
  FSSpec fspec;
  Str255 ioName;

  C2P_Str(":Plug-ins:Boards:Read me!.pdf", ioName);
  ::HGetVol(volName, &vRefNum, &dirID);
  err = ::FSMakeFSSpec(vRefNum, dirID, ioName, &fspec);

  cat.hFileInfo.ioNamePtr = ioName;
  cat.hFileInfo.ioVRefNum = vRefNum;

  for (INT i = 0; i < maxBoardTypePlugins && err == noErr; i++) {
    C2P_Str("", ioName);
    cat.hFileInfo.ioFDirIndex = i + 1;
    cat.dirInfo.ioDrDirID = fspec.parID;
    cat.hFileInfo.ioACUser = 0;

    err = ::PBGetCatInfoSync(&cat);

    if (err == noErr && !(cat.hFileInfo.ioFlAttrib & (1 << 4))) {
      FInfo *info = &(cat.hFileInfo.ioFlFndrInfo);
      if (info->fdType == '�BRD' && info->fdCreator == 'RSED' &&
          ioName[1] != '(')
        P2C_Str(ioName, BoardTypePlugin[boardTypePluginCount++]);
    }
  }
} /* InitBoardTypePlugins */

void AddBoardTypePlugins(CMenu *pm) {
  if (boardTypePluginCount == 0) return;

  pm->AddSeparator();
  for (INT i = 0; i < boardTypePluginCount; i++)
    pm->AddItem(BoardTypePlugin[i], boardType_Last + 1 + i);
} /* AddBoardTypePlugins */

INT BoardTypePluginCount(void) {
  return boardTypePluginCount;
} /* BoardTypePluginCount */

static void LoadBoardTypePlugin(INT n)  // n = 0..9
{
  CHAR fileName[100];
  CFile file;

  Format(fileName, ":Plug-ins:Boards:%s", BoardTypePlugin[n]);
  file.Set(fileName, '�BRD');

  if (file.OpenRes(filePerm_Rd) == fileError_NoError) {
    wSquareBmp->LoadPicture(1000);
    bSquareBmp->LoadPicture(1001);
    file.CloseRes();
  }
} /* LoadBoardTypePlugin */

/**************************************************************************************************/
/*                                                                                                */
/*                                    BITMAP SCALING ALGORITHM */
/*                                                                                                */
/**************************************************************************************************/

#define isblue(c) (c->red == 0 && c->green == 0 && c->blue == 0xFFFF)

void CacheSrcBitMap(CView *source, INT x0, INT y0,
                    RGBColor c[minSquareWidth][minSquareWidth]) {
  source->SavePort();

  for (INT x = 0; x < minSquareWidth; x++)
    for (INT y = 0; y < minSquareWidth; y++)
      ::GetCPixel(x0 + x, y0 + y, &(c[x][y]));

  source->RestorePort();
} /* CacheSrcBitMap */

void ScaleBitMap(RGBColor c[minSquareWidth][minSquareWidth],
                 INT size,  // Size (height and width) of small source bitmap
                 CView *dest, INT X0,
                 INT Y0,   // Top left corner of larger dest bitmap
                 INT Size  // Size (height and width) of larger dest bitmap
) {
  dest->SavePort();

  for (INT X = 0; X < Size; X++)
    for (INT Y = 0; Y < Size; Y++) {
      INT x1 = (8 * X * size) / Size;
      INT x2 = (8 * (X + 1) * size) / Size;
      INT y1 = (8 * Y * size) / Size;
      INT y2 = (8 * (Y + 1) * size) / Size;

      INT dx1, dy1, dx2, dy2;

      if (x1 % 8 == x2 % 8)
        dx1 = x2 - x1, dx2 = 0;
      else
        dx1 = 8 - x1 % 8, dx2 = x2 - x1 - dx1;

      if (y1 % 8 == y2 % 8)
        dy1 = y2 - y1, dy2 = 0;
      else
        dy1 = 8 - y1 % 8, dy2 = y2 - y1 - dy1;

      RGBColor *c1 = &(c[x1 / 8][y1 / 8]);
      RGBColor *c2 = &(c[x2 / 8][y1 / 8]);
      RGBColor *c3 = &(c[x1 / 8][y2 / 8]);
      RGBColor *c4 = &(c[x2 / 8][y2 / 8]);

      ULONG q1 =
          (isblue(c1) ? 0
                      : dx1 * dy1);  // Quadrant 1 (NW) color influence factor
      ULONG q2 =
          (isblue(c2) ? 0
                      : dx2 * dy1);  // Quadrant 2 (NE) color influence factor
      ULONG q3 =
          (isblue(c3) ? 0
                      : dx1 * dy2);  // Quadrant 3 (SW) color influence factor
      ULONG q4 =
          (isblue(c4) ? 0
                      : dx2 * dy2);  // Quadrant 4 (SE) color influence factor

      ULONG qsum = q1 + q2 + q3 + q4;

      if (qsum >= (x2 - x1) * (y2 - y1) / 2) {
        RGBColor dst;
        dst.red = MinL(
            0xFFFF,
            (c1->red * q1 + c2->red * q2 + c3->red * q3 + c4->red * q4) / qsum);
        dst.green = MinL(0xFFFF, (c1->green * q1 + c2->green * q2 +
                                  c3->green * q3 + c4->green * q4) /
                                     qsum);
        dst.blue = MinL(0xFFFF, (c1->blue * q1 + c2->blue * q2 + c3->blue * q3 +
                                 c4->blue * q4) /
                                    qsum);

        ::SetCPixel(X0 + X, Y0 + Y, &dst);
      }
    }

  dest->RestorePort();
} /* ScaleBitMap */

/**************************************************************************************************/
/*                                                                                                */
/*                               BITMAP CACHE (TOOLBAR BUTTONS MAINLY) */
/*                                                                                                */
/**************************************************************************************************/

#define bmpCacheSize 100

typedef struct {
  INT bmpID;
  CBitmap *bmp;
} BMP_CACHE;

static INT bmpCount = 0;
static BMP_CACHE BmpCache[bmpCacheSize];

CBitmap *GetBmp(INT bmpID, INT depth) {
  if (bmpCount == 0)  // If no bitmaps in cache yet -> Reset cache first
  {
    for (INT i = 0; i < bmpCacheSize; i++)
      BmpCache[i].bmpID = -1, BmpCache[i].bmp = nil;
  }

  for (INT i = 0; i < bmpCount; i++)
    if (bmpID == BmpCache[i].bmpID) return BmpCache[i].bmp;

  if (bmpCount < bmpCacheSize) {
    BmpCache[bmpCount].bmpID = bmpID;
    BmpCache[bmpCount].bmp = new CBitmap(bmpID, depth);
    return BmpCache[bmpCount++].bmp;
  } else
    return new CBitmap(bmpID, depth);
} /* GetBitmap */

/**************************************************************************************************/
/*                                                                                                */
/*                                       START UP INITIALIZATION */
/*                                                                                                */
/**************************************************************************************************/

void InitBmpUtilModule(void) {
  INT maxSquareWidth = squareWidth4;

  CRect r(0, 0, maxSquareWidth, maxSquareWidth);
  CRect r1(0, 0, 6 * (squareWidth1 + 1) + 1, 2 * (squareWidth1 + 1) + 1);
  CRect r2(0, 0, 6 * (squareWidth2 + 1) + 1, 2 * (squareWidth2 + 1) + 1);
  CRect r3(0, 0, 6 * (squareWidth3 + 1) + 1, 2 * (squareWidth3 + 1) + 1);
  CRect r4(0, 0, 6 * (squareWidth4 + 1) + 1, 2 * (squareWidth4 + 1) + 1);

  pieceBmp1 = new PieceBmp(0);
  pieceBmp2 = new CBitmap(r2.Width(), r2.Height(), 16);
  pieceBmp3 = new CBitmap(r3.Width(), r3.Height(), 16);
  pieceBmp4 = new CBitmap(r4.Width(), r4.Height(), 16);

  pieceBmpView1 = new CView(pieceBmp1, r1);
  pieceBmpView2 = new CView(pieceBmp2, r2);
  pieceBmpView3 = new CView(pieceBmp3, r3);
  pieceBmpView4 = new CView(pieceBmp4, r4);

  figurineBmp = new CBitmap(figurineID, 8);

  wSquareBmp = new CBitmap(maxSquareWidth, maxSquareWidth, 16);
  bSquareBmp = new CBitmap(maxSquareWidth, maxSquareWidth, 16);
  wSquareBmpView = new CView(wSquareBmp, r);
  bSquareBmpView = new CView(bSquareBmp, r);

  utilBmp = new CBitmap(2 * maxSquareWidth, 2 * maxSquareWidth, 16);
  utilBmpView = new CView(utilBmp, utilBmp->bounds);

  LoadSquareBmp(0);
} /* InitBmpUtilModule */
