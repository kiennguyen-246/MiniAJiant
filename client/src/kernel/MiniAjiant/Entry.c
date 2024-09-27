#include "Entry.h"

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject,
                     PUNICODE_STRING pusRegistryPath) {
  // DbgPrint("DriverEntry called\n");
  UNREFERENCED_PARAMETER(pusRegistryPath);

  NTSTATUS ntStatus = STATUS_SUCCESS;
  PSECURITY_DESCRIPTOR psd;
  OBJECT_ATTRIBUTES oa;
  UNICODE_STRING usMfltPortName;

  mfltData.pDriverObject = pDriverObject;

  /// Register the filter
  ntStatus =
      FltRegisterFilter(pDriverObject, &fltRegistration, &mfltData.pFilter);
  if (ntStatus != STATUS_SUCCESS) {
    DbgPrint("Register filter failed 0x%08x\n", ntStatus);
    return ntStatus;
  }
  // DbgPrint("Register filter successful\n");

  // DbgBreakPoint();

  /// Create a communication port
  ntStatus = FltBuildDefaultSecurityDescriptor(&psd, FLT_PORT_ALL_ACCESS);
  if (ntStatus != STATUS_SUCCESS) {
    DbgPrint("Build security descriptor failed 0x%08x\n", ntStatus);
    FltUnregisterFilter(mfltData.pFilter);
    return ntStatus;
  }

  RtlInitUnicodeString(&usMfltPortName, MFLT_COM_PORT_NAME);

  InitializeObjectAttributes(&oa, &usMfltPortName,
                             OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL,
                             psd);
  ntStatus = FltCreateCommunicationPort(mfltData.pFilter, &mfltData.pServerPort,
                                        &oa, NULL, mfltComConnect,
                                        mfltComDisconnect, NULL, 1);
  FltFreeSecurityDescriptor(psd);
  if (ntStatus != STATUS_SUCCESS) {
    DbgPrint("Create COM port failed 0x%08x\n", ntStatus);
    FltUnregisterFilter(mfltData.pFilter);
    return ntStatus;
  }

  /// Register a CreateProcess notify routine
  ntStatus =
      PsSetCreateProcessNotifyRoutineEx(mfltCreateProcessNotifyRoutine, FALSE);
  if (ntStatus != STATUS_SUCCESS) {
    DbgPrint("Start CreateProcess notify routine failed 0x%08x\n", ntStatus);
  } else {
    DbgPrint("Start CreateProcess notify routine successfully\n");
  }

  /// Start filtering (Must unregister immediately if fail)
  ntStatus = FltStartFiltering(mfltData.pFilter);
  if (ntStatus != STATUS_SUCCESS) {
    DbgPrint("Start filtering failed 0x%08x\n", ntStatus);
    FltCloseCommunicationPort(mfltData.pServerPort);
    FltUnregisterFilter(mfltData.pFilter);
    return ntStatus;
  }
  DbgPrint("Start filtering successful\n");

  return ntStatus;
}

NTSTATUS DriverUnload(FLT_FILTER_UNLOAD_FLAGS fltUnloadFlags) {
  UNREFERENCED_PARAMETER(fltUnloadFlags);

  // DbgPrint("DriverUnload called\n");

  /// Unregister the CreateProcess notify routine
  PsSetCreateProcessNotifyRoutineEx(mfltCreateProcessNotifyRoutine, TRUE);

  /// Close the communication port
  FltCloseCommunicationPort(mfltData.pServerPort);

  /// Unregister filter
  FltUnregisterFilter(mfltData.pFilter);

  return STATUS_SUCCESS;
}

NTSTATUS mfltComConnect(PFLT_PORT pClientPort, PVOID pServerPortCookie,
                        PVOID pConnectionContext, ULONG uiSizeOfContext,
                        PVOID* pConnectionCookie) {
  UNREFERENCED_PARAMETER(pServerPortCookie);
  UNREFERENCED_PARAMETER(pConnectionContext);
  UNREFERENCED_PARAMETER(uiSizeOfContext);
  UNREFERENCED_PARAMETER(pConnectionCookie);

  DbgPrint("mfltComConnect called\n");
  FLT_ASSERT(mfltData.pClientPort == NULL);
  mfltData.pClientPort = pClientPort;
  return STATUS_SUCCESS;
}

NTSTATUS mfltComDisconnect(PVOID pConnectionCookie) {
  UNREFERENCED_PARAMETER(pConnectionCookie);

  DbgPrint("mfltComDisconnect called\n");
  mfltData.bIsComPortClosed = TRUE;
  FltCloseClientPort(mfltData.pFilter, &mfltData.pClientPort);
  return STATUS_SUCCESS;
}

VOID mfltCreateProcessNotifyRoutine(PEPROCESS pProcess, HANDLE hPid,
                                    PPS_CREATE_NOTIFY_INFO pCreateInfo) {
  PMFLT_EVENT_RECORD pEventRecord;
  LARGE_INTEGER liSystemTime = {0};
  LARGE_INTEGER liSystemLocalTime = {0};
  PFLT_GENERIC_WORKITEM pWorkItem = NULL;
  WCHAR pwcTruncated[] = L"... (truncated)";

  pEventRecord = (PMFLT_EVENT_RECORD)ExAllocatePool2(
      POOL_FLAG_NON_PAGED, sizeof(MFLT_EVENT_RECORD), 'REFM');
  if (pEventRecord == NULL) {
    DbgPrint("Cannot allocate pEventRecord\n");
    return;
  }
  RtlZeroMemory(pEventRecord, sizeof(MFLT_EVENT_RECORD));

  // DbgBreakPoint();

  KeQuerySystemTime(&liSystemTime);
  ExSystemTimeToLocalTime(&liSystemTime, &liSystemLocalTime);
  pEventRecord->uliSysTime.QuadPart = liSystemLocalTime.QuadPart;
  if (pCreateInfo != NULL) {
    // DbgPrint("Process PID = %d created from parent process PID = %d\n", hPid,
    // pCreateInfo->ParentProcessId);
    pEventRecord->eventType = MFLT_PROCESS_CREATE;
    pEventRecord->objInfo.procInfo.uiPID = (ULONG)(ULONGLONG)hPid;
    pEventRecord->objInfo.procInfo.uiParentPID =
        (ULONG)(ULONGLONG)pCreateInfo->ParentProcessId;
    if (pCreateInfo->ImageFileName->Buffer != NULL) {
      // DbgBreakPoint();
      RtlCopyMemory(pEventRecord->objInfo.procInfo.pwcImageName,
                    pCreateInfo->ImageFileName->Buffer,
                    pCreateInfo->ImageFileName->Length);
      pEventRecord->objInfo.procInfo.uiImageNameLength =
          pCreateInfo->ImageFileName->Length;
    }
    // DbgBreakPoint();
    if (pCreateInfo->CommandLine->Buffer != NULL) {
      if (pCreateInfo->CommandLine->Length <= UM_MAX_PATH) {
        RtlCopyMemory(pEventRecord->objInfo.procInfo.pwcCommandLine,
                      pCreateInfo->CommandLine->Buffer,
                      pCreateInfo->CommandLine->Length);
        /*wcscat(pEventRecord->objInfo.procInfo.pwcCommandLine,
               pCreateInfo->CommandLine->Buffer)*/
        ;
        pEventRecord->objInfo.procInfo.uiCommandLineLength =
            pCreateInfo->CommandLine->Length;
      } else {
        RtlCopyMemory(pEventRecord->objInfo.procInfo.pwcCommandLine,
                      pCreateInfo->CommandLine->Buffer,
                      (UM_MAX_PATH - 1 - wcslen(pwcTruncated)));
        
        wcscat_s(pEventRecord->objInfo.procInfo.pwcCommandLine, UM_MAX_PATH, pwcTruncated);
        pEventRecord->objInfo.procInfo.uiCommandLineLength =
            (ULONG)(ULONGLONG)wcslen(
                pEventRecord->objInfo.procInfo.pwcCommandLine);
      }
    }

  } else {
    NTSTATUS ntsExitCode = PsGetProcessExitStatus(pProcess);
    // DbgPrint("Process PID = %d exited with exitcode %d (0x%08x).\n", hPid,
    //          ntsExitCode, ntsExitCode);
    //  DbgPrint("Process PID = %d exited.\n", hPid);
    pEventRecord->eventType = MFLT_PROCESS_TERMINATE;
    pEventRecord->objInfo.procInfo.uiPID = (ULONG)(ULONGLONG)hPid;
    pEventRecord->objInfo.procInfo.iExitcode = (ULONG)ntsExitCode;
  }

  pWorkItem = FltAllocateGenericWorkItem();
  FltQueueGenericWorkItem(pWorkItem, mfltData.pFilter,
                          mfltSendMessageWorkItemRoutine, DelayedWorkQueue,
                          pEventRecord);
}

VOID mfltSendMessageWorkItemRoutine(PFLT_GENERIC_WORKITEM pWorkItem,
                                    PVOID pFilterObject, PVOID pContext) {
  UNREFERENCED_PARAMETER(pFilterObject);

  NTSTATUS ntStatus;
  //LARGE_INTEGER liTimeOut = {0};

  // DbgBreakPoint();

  if (mfltData.pClientPort != NULL) {
    // liTimeOut.QuadPart = MAX_TIMEOUT;

    if (!mfltData.bIsComPortClosed) {
      ntStatus =
          FltSendMessage(mfltData.pFilter, &mfltData.pClientPort, pContext,
                         sizeof(MFLT_EVENT_RECORD), NULL, 0, NULL);
      if (ntStatus != STATUS_SUCCESS) {
        DbgPrint("Send event record failed 0x%08x\n", ntStatus);
      }
    }
  }
  ExFreePool(pContext);
  FltFreeGenericWorkItem(pWorkItem);
}
