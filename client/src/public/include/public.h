#ifndef PUBLIC_H
#define PUBLIC_H

#define MAX_BUFFER_SIZE 1024
#define MAX_BUFFERED_EVENT_COUNT 10
#define UM_MAX_PATH 260

typedef enum _MFLT_EVENT_TYPE {
  MFLT_CREATE,
  MFLT_CLOSE,
  MFLT_DELETE,
  MFLT_OPEN,
  MFLT_WRITE,
  MFLT_PROCESS_CREATE,
  MFLT_PROCESS_TERMINATE,
} MFLT_EVENT_TYPE,
    *PMFLT_EVENT_TYPE;

typedef union _MFLT_OBJ_INFO {
  struct {
    WCHAR pwcFileName[UM_MAX_PATH];

    ULONG uiFileNameLength;

    WCHAR pwcVolumeName[UM_MAX_PATH];

    ULONG uiVolumeNameLength;

    // ULONG uiSize;

    // BOOLEAN bHasReadAccess;

    // BOOLEAN bHasWriteAccess;

    // BOOLEAN bHasDeleteAccess;

    // BOOLEAN bHasSharedReadAccess;

    // BOOLEAN bHasSharedWriteAccess;

    // BOOLEAN bHasSharedDeleteAccess;

    // BOOLEAN bIsModified;

    BOOLEAN bIsDirectory;

    BOOLEAN bIsOverwritten;

    LONG iOperationStatus;
  } fileInfo;
  struct {
    ULONG uiPID;

    ULONG uiParentPID;

    WCHAR pwcImageName[UM_MAX_PATH];

    ULONG uiImageNameLength;

    WCHAR pwcCommandLine[UM_MAX_PATH];

    ULONG uiCommandLineLength;

    LONG iExitcode;
  } procInfo;
} MFLT_OBJ_INFO, *PMFLT_OBJ_INFO;

typedef struct _MFLT_EVENT_RECORD {
  ULARGE_INTEGER uliSysTime;

  MFLT_EVENT_TYPE eventType;

  MFLT_OBJ_INFO objInfo;
} MFLT_EVENT_RECORD, *PMFLT_EVENT_RECORD;

#endif  //! PUBLIC_H