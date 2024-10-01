#pragma once
#ifndef INCLUDE_H

#include <fltkernel.h>

#include "public.h"

#define MFLT_COM_PORT_NAME L"\\MAJPort"
#define DbgPrint(x, ...) DbgPrint("[MiniAJiant]" x, __VA_ARGS__)
#define MAX_TIMEOUT 50000000

typedef struct _MFLT_DATA {
  // The driver object
  PDRIVER_OBJECT pDriverObject;

  // The filter obtained from FltRegisterFilter
  PFLT_FILTER pFilter;

  // The server port. User mode connect to this
  PFLT_PORT pServerPort;

  // The client port. Only one user mode application is allowed at a time
  PFLT_PORT pClientPort;

  // A boolean keeping track of whether the communication port has been closed
  BOOLEAN bIsComPortClosed;

  // LIST_ENTRY leOutputBufferList;
} MFLT_DATA, *PMFLT_DATA;

MFLT_DATA mfltData;

DRIVER_INITIALIZE DriverEntry;

/**
 * The entry point of the driver. Called when the driver is loaded.
 *
 * @param pDriverObject The driver object
 * @param pusRegistryPath The registry path for the driver
 */
NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject,
                     PUNICODE_STRING pusRegistryPath);

/**
 * The unload function of the driver. Called when the driver is unloaded.
 *
 * @param fltUnloadFlags Unload flags
 */
NTSTATUS DriverUnload(FLT_FILTER_UNLOAD_FLAGS fltUnloadFlags);

/**
 * The PFLT_CONNECT_NOTIFY callback of the driver. Used for
 * FltCreateCommunicationPort.
 *
 * @param pClientPort, pServerPortCookie, pConnectionContext, uiSizeOfContext,
 * pConnectionCookie Refer to the
 * PFLT_CONNECT_NOTIFY entry on Microsoft Learn.
 */
NTSTATUS mfltComConnect(PFLT_PORT pClientPort, PVOID pServerPortCookie,
                        PVOID pConnectionContext, ULONG uiSizeOfContext,
                        PVOID *pConnectionCookie);

/**
 * The PFLT_DISCONNECT_NOTIFY callback of the driver. Used for
 * FltCreateCommunicationPort.
 *
 * @param pConnectionCookie Refer to the
 * PFLT_DISCONNECT_NOTIFY entry on Microsoft Learn
 */
NTSTATUS mfltComDisconnect(PVOID pConnectionCookie);

/**
 * The PCREATE_PROCESS_NOTIFY_ROUTINE_EX callback of the driver.
 *
 * In this function, any CreateProcess and TerminateProcess event on the user
 * mode will be captured, and processed to be sent back to the user mode through
 * the communication port. The sending operation uses system worker threads for
 * asynchronized work.
 *
 * @param pProcess, hPid, pCreateInfo Refer to the
 * PCREATE_PROCESS_NOTIFY_ROUTINE_EX entry on Microsoft Learn
 */
VOID mfltCreateProcessNotifyRoutine(PEPROCESS pProcess, HANDLE hPid,
                                    PPS_CREATE_NOTIFY_INFO pCreateInfo);

/**
 * The PFLT_GENERIC_WORKITEM_ROUTINE callback of the driver.
 *
 * In this function, the driver will attempt to send the information already
 * gathered from the previous steps to the user mode through the communication
 * port. The information gathered are as the context for the work item.
 *
 * @param pProcess, hPid, pCreateInfo Refer to the
 * PCREATE_PROCESS_NOTIFY_ROUTINE_EX entry on Microsoft Learn
 */
VOID mfltSendMessageWorkItemRoutine(PFLT_GENERIC_WORKITEM pWorkItem,
                                    PVOID pFilterObject, PVOID pContext);

const FLT_REGISTRATION fltRegistration = {sizeof(FLT_REGISTRATION),
                                          FLT_REGISTRATION_VERSION,
                                          FLTFL_REGISTRATION_SUPPORT_NPFS_MSFS,
                                          NULL,
                                          NULL,
                                          DriverUnload,
                                          NULL,
                                          NULL,
                                          NULL,
                                          NULL,
                                          NULL,
                                          NULL,
                                          NULL,
                                          NULL,
                                          NULL};

#endif  // INCLUDE_H