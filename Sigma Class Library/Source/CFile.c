/**************************************************************************************************/
/*                                                                                                */
/* Module  : CFILE.C */
/* Purpose : Implements the generic file objects. */
/*                                                                                                */
/**************************************************************************************************/

/*
Copyright (c) 2011, Ole K. Christensen
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

¥ Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

¥ Redistributions in binary form must reproduce the above copyright notice, this
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

#include "CFile.h"
#include "CApplication.h"
#include "CDialog.h"
#include "CMemory.h"
#include "CUtility.h"

static OSErr err = noErr;  // Last reported OS file error code

static NavMenuItemSpec *SetNavPopupExtension(NavDialogOptions *dialogOptions,
                                             INT initIndex, INT formatCount,
                                             FILE_FORMAT FormatTab[]);
static OSErr GetNavAESpec(NavReplyRecord *reply, INT index, FSSpec *spec);

/**************************************************************************************************/
/*                                                                                                */
/*                                    CONSTRUCTOR/DESTRUCTOR */
/*                                                                                                */
/**************************************************************************************************/

// Creating a file object doesn't open/create the file, but merely sets up a
// file access environment (using the FSSpec approach).

CFile::CFile(CFile *file0) {
  if (!file0) {
    creator = theApp->creator;  // Use application creator per default.
    fileType = 'TEXT';          // Default file type is plain text.
    specValid = false;
    fRefNum = -1;
    name[0] = 0;
  } else {
    creator = file0->creator;
    fileType = file0->fileType;
    specValid = file0->specValid;
    spec = file0->spec;
    fRefNum = file0->fRefNum;
    ::CopyStr(file0->name, name);
  }
  err = noErr;

  needsNavCompleteSave = false;
  needsNavDisposeReply = false;
} /* CFile::CFile */

/**************************************************************************************************/
/*                                                                                                */
/*                                        SET FILE SPEC */
/*                                                                                                */
/**************************************************************************************************/

FERROR CFile::Set(CHAR *fileName, OSTYPE theFileType, OSTYPE theCreator,
                  FILEPATH pathType) {
  INT vNum;
  LONG dirID;
  Str255 pname;

  specValid = false;
  ::C2P_Str(fileName, pname);
  ::CopyStr(fileName, name);
  fileType = theFileType;
  if (theCreator != '????') creator = theCreator;

  switch (pathType) {
    case filePath_Default:
      vNum = 0;
      dirID = 0;
      break;
    case filePath_ConfigDir:
      if (err = ::FindFolder(kOnSystemDisk, kPreferencesFolderType,
                             kDontCreateFolder, &vNum, &dirID))
        return fileError_PrefDirNotFound;
      break;
    case filePath_Documents:
      if (err = ::FindFolder(kUserDomain, kDocumentsFolderType,
                             kDontCreateFolder, &vNum, &dirID))
        return fileError_DocsDirNotFound;
      break;
    case filePath_AppSupport:
      if (err = ::FindFolder(kUserDomain, kApplicationSupportFolderType,
                             kDontCreateFolder, &vNum, &dirID))
        return fileError_AppSupDirNotFound;
      break;
    case filePath_Logs:  //### Doesn't work??
      if (err = ::FindFolder(kUserDomain, kInstallerLogsFolderType,
                             kDontCreateFolder, &vNum, &dirID))
        return fileError_LogsDirNotFound;
      break;
  }

  err = ::FSMakeFSSpec(vNum, dirID, pname, &spec);
  if (err != noErr && err != fnfErr) return fileError_InvalidFileSpec;

  specValid = true;
  ::P2C_Str(spec.name, name);
  return fileError_NoError;
} /* CFile::Set */

FERROR CFile::Set(CFile *file) {
  ::CopyStr(file->name, name);
  creator = file->creator;
  fileType = file->fileType;
  specValid = file->specValid;
  spec = file->spec;
  fRefNum = file->fRefNum;
  return fileError_NoError;
} /* CFile::Set */

FERROR CFile::SetName(CHAR *fileName) {
  if (!Exists()) {
    ::C2P_Str(fileName, spec.name);
    ::CopyStr(fileName, name);
  }

  return fileError_NoError;
} /* CFile::SetType */

FERROR CFile::SetType(OSTYPE theFileType) {
  if (specValid && Exists()) {
    FInfo info;
    if (err = ::FSpGetFInfo(&spec, &info)) return fileError_InvalidFileSpec;
    info.fdType = theFileType;
    if (err = ::FSpSetFInfo(&spec, &info)) return fileError_InvalidFileSpec;
  }

  fileType = theFileType;
  return fileError_NoError;
} /* CFile::SetType */

FERROR CFile::SetCreator(OSTYPE theCreator) {
  if (specValid && Exists()) {
    FInfo info;
    if (err = ::FSpGetFInfo(&spec, &info)) return fileError_InvalidFileSpec;
    info.fdCreator = theCreator;
    if (err = ::FSpSetFInfo(&spec, &info)) return fileError_InvalidFileSpec;
  }

  creator = theCreator;
  return fileError_NoError;
} /* CFile::SetCreator */

/**************************************************************************************************/
/*                                                                                                */
/*                                         CREATE/DELETE */
/*                                                                                                */
/**************************************************************************************************/

/*-------------------------------------- Create Delete File
 * --------------------------------------*/

FERROR CFile::Create(void) {
  specValid = false;
  if (err = ::FSpCreate(&spec, creator, fileType, smSystemScript))
    return fileError_CreateFailed;
  specValid = true;
  return fileError_NoError;
} /* CFile::Create */

FERROR CFile::Delete(void) {
  specValid = false;
  if (err = ::FSpDelete(&spec)) return fileError_DeleteFailed;
  return fileError_NoError;
} /* CFile::Delete */

/*-------------------------------------- Create Resource Fork
 * ------------------------------------*/

FERROR CFile::CreateRes(void) {
  specValid = false;
  ::FSpCreateResFile(&spec, creator, fileType, smSystemScript);
  if (::ResError() != noErr) return fileError_CreateFailed;
  specValid = true;
  return fileError_NoError;
} /* CFile::CreateRes */

/**************************************************************************************************/
/*                                                                                                */
/*                                          OPEN/CLOSE */
/*                                                                                                */
/**************************************************************************************************/

/*------------------------------------- Open/Close Data Fork
 * -------------------------------------*/

FERROR CFile::Open(FILEPERM filePerm) {
  if (err = ::FSpOpenDF(&spec, filePerm, &fRefNum)) return fileError_OpenFailed;
  return SetPos(0);
} /* CFile::Open */

FERROR CFile::Close(void) {
  if (err = ::FSClose(fRefNum)) return fileError_CloseFailed;
  if (err = ::FlushVol(nil, spec.vRefNum))  //### Remove this call? (Apple
                                            //Carbon Doc says it's evil!)
    return fileError_FlushFailed;
  return fileError_NoError;
} /* CFile::Close */

/*----------------------------------- Open/Close Resource Fork
 * -----------------------------------*/

FERROR CFile::OpenRes(FILEPERM filePerm) {
  fRefNumRes = ::FSpOpenResFile(&spec, filePerm);
  if (fRefNumRes == -1) return fileError_OpenFailed;
  return fileError_NoError;
} /* CFile::OpenRes */

FERROR CFile::CloseRes(void) {
  ::CloseResFile(fRefNumRes);
  if (ResError() != noErr) return fileError_CloseFailed;
  return fileError_NoError;
} /* CFile::CloseRes */

/**************************************************************************************************/
/*                                                                                                */
/*                                          READ/WRITE */
/*                                                                                                */
/**************************************************************************************************/

FERROR CFile::Read(ULONG *bytes, PTR buffer)  // Returns bytes read
{
  if (!buffer || (err = ::FSRead(fRefNum, (long *)bytes, buffer)))
    return fileError_ReadFailed;
  return fileError_NoError;
} /* CFile::Read */

FERROR CFile::Write(ULONG *bytes, PTR buffer)  // Returns bytes written
{
  if (!buffer || (err = ::FSWrite(fRefNum, (long *)bytes, buffer)))
    return fileError_WriteFailed;
  return fileError_NoError;
} /* CFile::Write */

FERROR CFile::Clear(void) {
  FERROR ferr;
  if (ferr = Open(filePerm_Wr)) return ferr;
  SetSize(0);
  return Close();
} /* CFile::Clear */

/**************************************************************************************************/
/*                                                                                                */
/*                                     LOAD/SAVE WHOLE FILE */
/*                                                                                                */
/**************************************************************************************************/

/*------------------------------------ Binary File Operations
 * ------------------------------------*/

FERROR CFile::Load(ULONG *bytes, PTR *data) {
  *bytes = 0;
  *data = nil;

  FERROR ferr;
  if (ferr = Open(filePerm_Rd)) return ferr;
  if (!GetSize(bytes)) {
    *data = Mem_AllocPtr(*bytes);
    if (*data) Read(bytes, *data);
  }
  return Close();
} /* CFile::Load */

FERROR CFile::Save(ULONG bytes, PTR data) {
  FERROR ferr;
  ULONG dbytes = bytes;
  if (ferr = Open(filePerm_Wr)) return ferr;
  Write(&dbytes, data);
  SetSize(dbytes);
  return Close();
} /* CFile::Save */

FERROR CFile::Append(ULONG bytes, PTR data) {
  FERROR ferr;
  ULONG size, dbytes = bytes;
  if (ferr = Open(filePerm_Wr)) return ferr;
  GetSize(&size);
  SetPos(size);
  Write(&dbytes, data);
  size += dbytes;
  SetSize(size);
  return Close();
} /* CFile::Append */

/*-------------------------------------- Text File Operations
 * ------------------------------------*/

FERROR CFile::LoadStr(CHAR **str) {
  ULONG bytes = 0;
  *str = nil;

  FERROR ferr;
  if (ferr = Open(filePerm_Rd)) return ferr;
  if (!GetSize(&bytes)) {
    *str = (CHAR *)Mem_AllocPtr(bytes +
                                1);  // Allocate extra char for null termination
    if (*str) {
      Read(&bytes, (PTR)*str);
      (*str)[bytes] = 0;  // Null terminate!!!
    }
  }
  return Close();
} /* CFile::LoadStr */

FERROR CFile::SaveStr(CHAR *str) {
  return Save(StrLen(str), (PTR)str);
} /* CFile::SaveStr */

FERROR CFile::AppendStr(CHAR *str) {
  return Append(StrLen(str), (PTR)str);
} /* CFile::AppendStr */

/**************************************************************************************************/
/*                                                                                                */
/*                                             MISC */
/*                                                                                                */
/**************************************************************************************************/

FERROR CFile::GetPathName(CHAR *pathName, INT maxLen) {
  FSRef fsRef;

  if (::FSpMakeFSRef(&spec, &fsRef) != noErr) return fileError_InvalidFileSpec;
  if (::FSRefMakePath(&fsRef, (unsigned char *)pathName, maxLen) != noErr)
    return fileError_InvalidFileSpec;
  return fileError_NoError;
} /* CFile::GetPathName */

FERROR CFile::CompleteSave(void)
// Should be called after completing saving a file that was "created"/"saved"
// from the "SaveDialog".
{
  if (needsNavCompleteSave) {
    ::NavCompleteSave(&saveReply, kNavTranslateInPlace);
    needsNavCompleteSave = false;
  }

  if (needsNavDisposeReply) {
    ::NavDisposeReply(&saveReply);
    needsNavDisposeReply = false;
  }

  return fileError_NoError;
} /* CFile::CompleteSave */

BOOL CFile::Exists(void) {
  if (!specValid) return false;

  FSSpec s;
  return (::FSMakeFSSpec(spec.vRefNum, spec.parID, spec.name, &s) == noErr);
} /* CFile::Exists */

BOOL CFile::Equal(CFile *file) {
  return (spec.vRefNum == file->spec.vRefNum &&
          spec.parID == file->spec.parID && spec.name == file->spec.name);
} /* CFile::Equal */

/*------------------------------------- File Position/Size
 * ---------------------------------------*/

FERROR CFile::GetPos(ULONG *pos) {
  if (err = ::GetFPos(fRefNum, (long *)pos)) return fileError_GetPos;
  return fileError_NoError;
} /* CFile::GetPos */

FERROR CFile::SetPos(ULONG pos) {
  if (err = ::SetFPos(fRefNum, fsFromStart, (long)pos)) return fileError_SetPos;
  return fileError_NoError;
} /* CFile::SetPos */

FERROR CFile::GetSize(ULONG *size) {
  if (err = ::GetEOF(fRefNum, (long *)size)) return fileError_GetSize;
  return fileError_NoError;
} /* CFile::GetSize */

FERROR CFile::SetSize(ULONG size) {
  if (err = ::SetEOF(fRefNum, (long)size)) return fileError_SetSize;
  return fileError_NoError;
} /* CFile::SetSize */

/*---------------------------------------- File Locking
 * ------------------------------------------*/

FERROR CFile::SetLock(BOOL locked) {
  if (!specValid) return fileError_InvalidFileSpec;
  err = (locked ? FSpSetFLock(&spec) : FSpRstFLock(&spec));
  return (!err ? fileError_NoError : fileError_FailedLocking);
} /* CFile::SetLock */

BOOL CFile::IsLocked(void) {
  if (!specValid) return false;
  return (FSpRename(&spec, spec.name) == fLckdErr);
  /*
     FInfo info;
     if (FSpGetFInfo(&spec, &info)) return false;
     return ((info.fdFlags & kNameLocked) != 0);
  */
} /* CFile::IsLocked */

void FSSpec2CFile(FSSpec *theSpec, CFile *cfile) {
  FInfo finfo;

  cfile->spec = *theSpec;
  cfile->specValid = true;
  ::P2C_Str(cfile->spec.name, cfile->name);
  ::FSpGetFInfo(theSpec, &finfo);
  cfile->fileType = finfo.fdType;
} /* FSSpec2CFile */

/**************************************************************************************************/
/*                                                                                                */
/*                                       FILE OPEN DIALOG */
/*                                                                                                */
/**************************************************************************************************/

pascal Boolean OpenFilterProc(AEDesc *theItem, void *info,
                              NavCallBackUserData callBackUD,
                              NavFilterModes filterMode);
pascal void OpenEventProc(const NavEventCallbackMessage callBackSelector,
                          NavCBRecPtr callBackParms,
                          NavCallBackUserData callBackUD);

CFileOpenDialog::CFileOpenDialog(void) {
  currFormat = 0;
  fileOpenCount = 0;
} /* CFileOpenDialog::CFileOpenDialog */

/*------------------------------------ Run File Open Dialog
 * --------------------------------------*/

BOOL CFileOpenDialog::Run(CFile *targetFile, CHAR *title, INT initItem,
                          INT formatCount, FILE_FORMAT FormatTab[])

// This function works in two modes:
//
// 1) Single file mode
// 2) Multi file mode
//
// This is controlled by the "targetFile" object. If this is not nil, then case
// 1) applies and the targetFile is simply initialized so it "points" to the
// file selected by the user. The file isn't opened here. If targetFile = nil,
// then case 2) applies, and we simply send some "HandleOpenFile" events to the
// application object.

{
  currFormat = (formatCount > 0 ? FormatTab[0].id : '????');
  NavDialogOptions dialogOptions;

  // Specify default options for dialog box
  err = ::NavGetDefaultDialogOptions(&dialogOptions);

  // Set the various options:
  // dialogOptions.dialogOptionFlags |= kNavSelectDefaultLocation;
  dialogOptions.dialogOptionFlags ^= kNavAllowPreviews;
  if (targetFile) dialogOptions.dialogOptionFlags ^= kNavAllowMultipleFiles;
  if (title) ::C2P_Str(title, dialogOptions.windowTitle);

  SetNavPopupExtension(&dialogOptions, initItem, formatCount, FormatTab);

  NavEventUPP eventProc = ::NewNavEventUPP(OpenEventProc);
  NavObjectFilterUPP filterProc = ::NewNavObjectFilterUPP(OpenFilterProc);
  NavReplyRecord reply;
  NavTypeListHandle typeList = NULL;

  err = ::NavGetFile(NULL, &reply, &dialogOptions, eventProc, nil, filterProc,
                     typeList, (void *)this);

  ::DisposeNavEventUPP(eventProc);
  ::DisposeNavObjectFilterUPP(filterProc);

  if (err != noErr) {
    if (err != userCanceledErr) FileErr();
  } else if (reply.validRecord) {
    // Deal with multiple file selection
    long count;
    err = AECountItems(&(reply.selection), &count);
    fileOpenCount = count;

    if (err == noErr)
      for (long index = 1; index <= count; index++) {
        FSSpec theSpec;
        err = GetNavAESpec(&reply, index, &theSpec);
        if (err == noErr)
          if (targetFile)
            FSSpec2CFile(&theSpec, targetFile);
          else
            theApp->HandleOpenFSSpec(&theSpec);
      }

    // Dispose of NavReplyRecord, resources, descriptors
    err = NavDisposeReply(&reply);
    if (err != noErr) FileErr();
  }

  if (formatCount > 0) DisposeHandle((Handle)(dialogOptions.popupExtension));

  return (err == noErr);
} /* CFileOpenDialog::Run */

/*------------------------------------ Filter & Event Procs
 * --------------------------------------*/

pascal Boolean OpenFilterProc(AEDesc *theItem, void *info,
                              NavCallBackUserData callBackUD,
                              NavFilterModes filterMode) {
  Boolean display = true;
  NavFileOrFolderInfo *theInfo = (NavFileOrFolderInfo *)info;
  CFileOpenDialog *dlg = (CFileOpenDialog *)callBackUD;

  if (theItem && theInfo && theItem->descriptorType == typeFSS &&
      !theInfo->isFolder && dlg) {
    CHAR fileName[64];
    FSSpec spec;
    CopyStr("", fileName);

    AEGetDescData(theItem, &spec, sizeof(FSSpec));
    if (spec.name[0] > 0 && spec.name[0] < 32)
      ::P2C_Str(spec.name, fileName);
    else
      Beep(1);
    /*
          else if (::AECoerceDesc(theItem, typeFSS, &fssDesc) == noErr)
          {
             FSSpec spec;
             AEGetDescData(&fssDesc, &spec, sizeof(FSSpec));
             ::P2C_Str(spec.name, fileName);
          }
    */
    if (!dlg->Filter(theInfo->fileAndFolder.fileInfo.finderInfo.fdType,
                     fileName))
      display = false;
  }

  return display;
} /* OpenFilterProc */

pascal void OpenEventProc(const NavEventCallbackMessage callBackSelector,
                          NavCBRecPtr callBackParms,
                          NavCallBackUserData callBackUD) {
  if (!callBackParms || !callBackUD) return;

  CFileOpenDialog *dlg = (CFileOpenDialog *)callBackUD;

  switch (callBackSelector) {
    case kNavCBStart:
      break;
    case kNavCBPopupMenuSelect:
      NavMenuItemSpec *Item =
          (NavMenuItemSpec *)callBackParms->eventData.eventDataParms.param;
      if (dlg) dlg->currFormat = Item->menuType;
      break;
    case kNavCBEvent:
      EventRecord *event = callBackParms->eventData.eventDataParms.event;
      if (event->what == updateEvt || event->what == activateEvt)
        theApp->DoEvent(event);
      break;
  }
} /* OpenEventProc */

/*------------------------------------------ Filter
 * ----------------------------------------------*/
// The default Filter routine simply assumes that the "FILE_FORMAT.id" and
// "NavMenuItemSpec.menuType" fields are actually file types (OSTYPE).

BOOL CFileOpenDialog::Filter(OSTYPE fileType, CHAR *fileName) {
  return (fileType == currFormat);
} /* CFileOpenDialog::Filter */

/*--------------------------------------- Open Text File
 * -----------------------------------------*/

CFileTextOpenDialog::CFileTextOpenDialog(void)
    : CFileOpenDialog() {} /* CFileTextOpenDialog::CFileTextOpenDialog */

BOOL CFileTextOpenDialog::Filter(OSTYPE fileType, CHAR *fileName) {
  return (fileType == 'TEXT');
} /* CFileTextOpenDialog::Filter */

/**************************************************************************************************/
/*                                                                                                */
/*                                       FILE SAVE DIALOG */
/*                                                                                                */
/**************************************************************************************************/

//### TODO: Initial fileformat selection

pascal void SaveEventProc(const NavEventCallbackMessage callBackSelector,
                          NavCBRecPtr callBackParms,
                          NavCallBackUserData callBackUD);
// kNavCtlSelectCustomType.

BOOL CFile::SaveDialog(CHAR *title, CHAR *initName, INT initItem,
                       INT formatCount, FILE_FORMAT FormatTab[]) {
  NavDialogOptions dialogOptions;
  FSSpec theSpec;

  saveReplace = false;
  needsNavCompleteSave = false;
  needsNavDisposeReply = false;

  // Specify default options for dialog box
  err = ::NavGetDefaultDialogOptions(&dialogOptions);

  // Set the various options:
  if (formatCount == 0) dialogOptions.dialogOptionFlags ^= kNavNoTypePopup;
  //    dialogOptions.dialogOptionFlags |= kNavSelectDefaultLocation;
  dialogOptions.dialogOptionFlags |= kNavDontAddTranslateItems;
  dialogOptions.dialogOptionFlags ^= kNavAllowStationery;
  if (title) ::C2P_Str(title, dialogOptions.windowTitle);
  ::C2P_Str(initName, dialogOptions.savedFileName);
  initMenuItemSpec =
      SetNavPopupExtension(&dialogOptions, initItem, formatCount, FormatTab);
  if (formatCount > 0) fileFormatItem = FormatTab[0].id;

  NavEventUPP eventProc = ::NewNavEventUPP(SaveEventProc);
  saveReply.translationNeeded = false;
  err = ::NavPutFile(NULL, &saveReply, &dialogOptions, eventProc, '????',
                     kNavGenericSignature, (void *)this);
  ::DisposeNavEventUPP(eventProc);

  if (err != noErr) {
    if (err != userCanceledErr) FileErr();
  } else if (saveReply.validRecord) {
    err = GetNavAESpec(&saveReply, 1, &theSpec);
    FSSpec2CFile(&theSpec, this);

    if (err == noErr) {
      needsNavCompleteSave = true;
      needsNavDisposeReply = true;
    } else {
      NavDisposeReply(&saveReply);
      needsNavDisposeReply = false;
    }
  }

  if (formatCount > 0) DisposeHandle((Handle)(dialogOptions.popupExtension));

  if (err != noErr) return false;

  specValid = true;
  saveReplace = saveReply.replacing;
  spec = theSpec;
  ::P2C_Str(spec.name, name);

  return true;
} /* CFile::SaveDialog */

/*----------------------------------------- Event Procs
 * ------------------------------------------*/

pascal void SaveEventProc(const NavEventCallbackMessage callBackSelector,
                          NavCBRecPtr callBackParms,
                          NavCallBackUserData callBackUD) {
  CFile *file = (CFile *)callBackUD;

  switch (callBackSelector) {
    case kNavCBStart:
      //       if (file->initMenuItemSpec)
      //          OSErr err = NavCustomControl((NavContext)file,
      //          kNavCtlSelectCustomType, file->initMenuItemSpec);
      break;
    case kNavCBPopupMenuSelect:
      NavMenuItemSpec *Item =
          (NavMenuItemSpec *)callBackParms->eventData.eventDataParms.param;
      if (file) file->fileFormatItem = Item->menuType;
      break;
    case kNavCBEvent:
      EventRecord *event = callBackParms->eventData.eventDataParms.event;
      if (event->what == updateEvt || event->what == activateEvt)
        theApp->DoEvent(event);
      break;
  }
} /* SaveEventProc */

/**************************************************************************************************/
/*                                                                                                */
/*                                         COMMON NAV STUFF */
/*                                                                                                */
/**************************************************************************************************/

static NavMenuItemSpec *SetNavPopupExtension(NavDialogOptions *dialogOptions,
                                             INT initIndex, INT formatCount,
                                             FILE_FORMAT FormatTab[]) {
  NavMenuItemSpec *initItem = nil;

  // Build "Show" popup menu:
  if (formatCount > 0) {
    NavMenuItemSpecArrayHandle mh = (NavMenuItemSpecArrayHandle)NewHandle(
        formatCount * sizeof(NavMenuItemSpec));
    HLock((Handle)mh);
    NavMenuItemSpecArrayPtr mp = *mh;
    for (INT i = 0; i < formatCount; i++) {
      mp[i].version = kNavMenuItemSpecVersion;
      mp[i].menuCreator = theApp->creator;
      mp[i].menuType = FormatTab[i].id;
      ::C2P_Str(FormatTab[i].text, mp[i].menuItemName);
    }
    initItem = &mp[initIndex];
    HUnlock((Handle)mh);
    dialogOptions->popupExtension = mh;
  }

  return initItem;
} /* SetNavPopupExtension */

static OSErr GetNavAESpec(NavReplyRecord *reply, INT index, FSSpec *spec) {
  AEKeyword theKeyword;
  DescType actualType;
  Size actualSize;

  return AEGetNthPtr(&(reply->selection), index, typeFSS, &theKeyword,
                     &actualType, spec, sizeof(FSSpec), &actualSize);
} /* GetNavAESpec */

/**************************************************************************************************/
/*                                                                                                */
/*                                          ERROR HANDLING */
/*                                                                                                */
/**************************************************************************************************/

static fileErrDlgOpen = false;

BOOL FileErr(FERROR errCode) {
  if (errCode == fileError_NoError) return false;

  CHAR *s, msg[100];

  switch (errCode) {
    case fileError_GenericError:
      s = "File error";
      break;
    case fileError_FileNotOpen:
      s = "File not open";
      break;
    case fileError_FileAlreadyOpen:
      s = "File already open";
      break;
    case fileError_CreateFailed:
      s = "Failed creating file";
      break;
    case fileError_DeleteFailed:
      s = "Failed deleteing file";
      break;
    case fileError_OpenFailed:
      s = "Failed opening file";
      break;
    case fileError_CloseFailed:
      s = "Failed closing file";
      break;
    case fileError_FlushFailed:
      s = "Failed flushing volume changes";
      break;
    case fileError_ReadFailed:
      s = "Failed reading file";
      break;
    case fileError_WriteFailed:
      s = "Failed writing file";
      break;
    case fileError_GetPos:
      s = "Failed getting file position";
      break;
    case fileError_SetPos:
      s = "Failed setting file position";
      break;
    case fileError_GetSize:
      s = "Failed getting file size";
      break;
    case fileError_SetSize:
      s = "Failed setting file size";
      break;
    case fileError_FailedLocking:
      s = "Failed changing file lock";
      break;
    case fileError_InvalidFileSpec:
      s = "Invalid file specification";
      break;
    case fileError_PrefDirNotFound:
      s = "Failed locating preferences directory";
      break;
    case fileError_DocsDirNotFound:
      s = "Failed locating Documents directory";
      break;
    case fileError_AppSupDirNotFound:
      s = "Failed locating Application support directory";
      break;
    default:
      return true;
  }

  if (!fileErrDlgOpen) {
    fileErrDlgOpen = true;
    Format(msg, "%s (OS Error %d)", s, err);
    if (!QuestionDialog(nil, "File Error", msg, "OK", "Quit")) theApp->Abort();
    fileErrDlgOpen = false;
  }

  return true;
} /* FileErr */

/*--------- Generic Resource Routines (Current Resource File) ---------*/

FERROR Res_Load(OSTYPE type, INT id, HANDLE *H) {
  *H = (HANDLE)::GetResource((ResType)type, id);
  return (FERROR)::ResError();
} /* Res_Load */

FERROR Res_Free(HANDLE H) {
  ::ReleaseResource((Handle)H);
  return (FERROR)::ResError();
} /* Res_Free */

FERROR Res_Write(HANDLE H) {
  ::ChangedResource((Handle)H);
  ::WriteResource((Handle)H);
  return (FERROR)::ResError();
} /* Res_Write */

FERROR Res_Delete(HANDLE H) {
  ::RemoveResource((Handle)H);
  return (FERROR)::ResError();
} /* Res_Delete */

/*
ULONG Res_GetSize (HANDLE H)
{
   return ::SizeResource((Handle)H);
} /* Res_GetSize */

FERROR Res_Add(HANDLE H, OSTYPE type, INT id, CHAR *name) {
  Str255 pname;
  C2P_Str(name, pname);
  ::AddResource((Handle)H, type, id, pname);
  return (FERROR)::ResError();
} /* Res_Add */

/**************************************************************************************************/
/*                                                                                                */
/*                                         GET FOLDER PATH */
/*                                                                                                */
/**************************************************************************************************/

BOOL CFile_GetFolderPathDialog(CHAR *folderPathName, INT maxLen) {
  NavDialogOptions dialogOptions;
  NavReplyRecord reply;
  FSSpec folderSpec;
  FSRef folderRef;
  BOOL success = false;

  // Specify default options for dialog box
  err = ::NavGetDefaultDialogOptions(&dialogOptions);
  err = ::NavChooseFolder(NULL, &reply, &dialogOptions, NULL, NULL, NULL);

  if (err != noErr) {
    if (err != userCanceledErr) FileErr();
  } else if (reply.validRecord) {
    if (::GetNavAESpec(&reply, 1, &folderSpec) == noErr &&
        ::FSpMakeFSRef(&folderSpec, &folderRef) == noErr &&
        ::FSRefMakePath(&folderRef, (unsigned char *)folderPathName, maxLen) ==
            noErr)
      success = true;

    err = ::NavDisposeReply(&reply);
    if (err != noErr) FileErr();
  }

  return success;
}  // CFile_GetFolderPathDialog
