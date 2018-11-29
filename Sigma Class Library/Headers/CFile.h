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

#pragma once

#include "General.h"

/**************************************************************************************************/
/*                                                                                                */
/*                                        CONSTANTS & MACROS */
/*                                                                                                */
/**************************************************************************************************/

enum FERROR {
  fileError_NoError = 0,
  fileError_GenericError,
  fileError_FileNotOpen,
  fileError_FileAlreadyOpen,
  fileError_CreateFailed,
  fileError_DeleteFailed,
  fileError_OpenFailed,
  fileError_CloseFailed,
  fileError_FlushFailed,
  fileError_ReadFailed,
  fileError_WriteFailed,
  fileError_GetPos,
  fileError_SetPos,
  fileError_GetSize,
  fileError_SetSize,
  fileError_FailedLocking,
  fileError_InvalidFileSpec,
  fileError_PrefDirNotFound,
  fileError_DocsDirNotFound,
  fileError_AppSupDirNotFound,
  fileError_LogsDirNotFound
};

enum FILEPATH {
  filePath_Default = 0,
  filePath_ConfigDir = kPreferencesFolderType,
  filePath_Documents = kDocumentsFolderType,
  filePath_AppSupport = kApplicationSupportFolderType,
  filePath_Logs = kInstallerLogsFolderType
};

enum FILEPERM {
  filePerm_Rd = fsRdPerm,
  filePerm_Wr = fsWrPerm,
  filePerm_RdWr = fsRdWrPerm
};

#define maxFileNameLen 31

/**************************************************************************************************/
/*                                                                                                */
/*                                         CLASS DEFINITIONS */
/*                                                                                                */
/**************************************************************************************************/

typedef OSType OSTYPE;

typedef struct {
  OSTYPE id;      // Unique ID identifying Show/Format menu item
  CHAR text[32];  // Item text ("" or "-") if separator.
} FILE_FORMAT;

class CFile {
 public:
  CFile(CFile *file = nil);

  FERROR Set(CHAR *fileName, OSTYPE fileType, OSTYPE creator = '????',
             FILEPATH pathType = filePath_Default);
  FERROR Set(CFile *file);
  FERROR SetName(CHAR *fileName);
  FERROR SetType(OSTYPE fileType);
  FERROR SetCreator(OSTYPE creator);

  FERROR Create(void);
  FERROR Delete(void);
  FERROR Open(FILEPERM filePerm);
  FERROR Close(void);
  FERROR Read(ULONG *bytes, PTR buffer);
  FERROR Write(ULONG *bytes, PTR buffer);
  FERROR Clear(void);
  FERROR CompleteSave(void);

  FERROR Load(ULONG *bytes, PTR *data);
  FERROR Save(ULONG bytes, PTR data);
  FERROR Append(ULONG bytes, PTR data);

  FERROR LoadStr(CHAR **str);
  FERROR SaveStr(CHAR *str);
  FERROR AppendStr(CHAR *str);

  FERROR CreateRes(void);
  FERROR OpenRes(FILEPERM filePerm);
  FERROR CloseRes(void);

  FERROR GetPathName(CHAR *pathName, INT maxLen);
  BOOL Exists(void);
  BOOL IsLocked(void);
  BOOL Equal(CFile *file);

  FERROR GetPos(ULONG *pos);
  FERROR SetPos(ULONG pos);
  FERROR GetSize(ULONG *size);
  FERROR SetSize(ULONG size);

  FERROR SetLock(BOOL locked);

  // File dialog handling
  BOOL SaveDialog(CHAR *prompt, CHAR *name, INT initIndex = 0,
                  INT formatCount = 0, FILE_FORMAT *FormatTab = nil);

  //--- PRIVATE ---

  Str255 dialogPrompt;
  INT fileFormatItem;                 // File format popup-menu item
  NavMenuItemSpec *initMenuItemSpec;  // Initially selected menu item in the
                                      // file format popup.
  BOOL saveReplace;

  NavReplyRecord saveReply;
  BOOL needsNavCompleteSave, needsNavDisposeReply;

  // Instance variables:
  CHAR name[maxFileNameLen + 1];

  OSTYPE creator, fileType;

  // Internal stuff:
  BOOL specValid;
  FSSpec spec;
  INT fRefNum, fRefNumRes;
};

class CFileOpenDialog {
 public:
  CFileOpenDialog(void);
  BOOL Run(CFile *targetFile = nil, CHAR *title = nil, INT initIndex = 0,
           INT formatCount = 0, FILE_FORMAT *FormatTab = nil);
  virtual BOOL Filter(OSTYPE fileType, CHAR *fileName);

  OSTYPE currFormat;
  long fileOpenCount;
};

class CFileTextOpenDialog : public CFileOpenDialog {
 public:
  CFileTextOpenDialog(void);
  virtual BOOL Filter(OSTYPE fileType, CHAR *name);
};

/**************************************************************************************************/
/*                                                                                                */
/*                                         GLOBAL DATA STRUCTURES */
/*                                                                                                */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                                                                                */
/*                                          FUNCTION PROTOTYPES */
/*                                                                                                */
/**************************************************************************************************/

BOOL CFile_GetFolderPathDialog(CHAR *folderPathName, INT maxLen);

BOOL FileErr(FERROR errCode = fileError_GenericError);

FERROR Res_Load(OSTYPE type, INT id, HANDLE *H);
FERROR Res_Free(HANDLE H);
FERROR Res_Write(HANDLE H);
FERROR Res_Delete(HANDLE H);
ULONG Res_GetSize(HANDLE H);
FERROR Res_Add(HANDLE H, OSTYPE type, INT id, CHAR *name);

void FSSpec2CFile(FSSpec *theSpec, CFile *cfile);
