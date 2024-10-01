#ifndef COM_PORT_HPP
#define COM_PORT_HPP

#include <Windows.h>
#include <fltUser.h>

#include <vector>

#include "public.h"
#include "utils/helpers.hpp"

typedef struct _COM_MESSAGE {
  FILTER_MESSAGE_HEADER header;
  MFLT_EVENT_RECORD eventRecord;
} COM_MESSAGE, *PCOM_MESSAGE;

class ComPort {
 public:
  /**
   * Default constructor.
   */
  ComPort();

  /**
   * Default destructor.
   */
  ~ComPort();

  /**
   * Connect to a kernel mode driver through a communication port.
   *
   * This function should be called before any message exchange operation, like
   * getRecord().
   *
   * @param sPortName The name of the communication port, in the form of
   * "\\<portname>"
   *
   * @return S_OK if the operation is successful, an error code/HRESULT if fail.
   */
  HRESULT connectToKernelNode(std::wstring sPortName);

  /**
   * Get a single event record from the kernel mode.
   *
   * While operating, the function will block at a FilterReceive() call until a
   * message is available at the kernel mode after being sent using FltSend().
   * If success, a message will be obtained and recorded to pEventRecord.
   *
   * @param pEventRecord An opaque pointer to a MFLT_EVENT_RECORD structure,
   * containing the event record obtained from the kernel mode at the end of the
   * execution of this funcion. See public.h for the definition of this struct.
   *
   * @return S_OK if the operation is successful, an error code/HRESULT if fail.
   */
  HRESULT getRecord(PMFLT_EVENT_RECORD pEventRecord);

  /**
   * Disconnect the user mode application from the kernel mode communication
   * port.
   *
   * This function should be called while cleaning up to free up resources taken
   * by the user mode communication port handler.
   *
   * @return S_OK if the operation is successful, an error code/HRESULT if fail.
   */
  HRESULT disconnectFromKernelMode();

 private:
  /// The name of the communication port.
  std::wstring wsComPortName;

  /// The user mode handle for the communication port. Should be freed when no
  /// longer needed.
  HANDLE hComPort;
};

#endif