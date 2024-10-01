#include "driver/ComPort.hpp"

ComPort::ComPort() { hComPort = NULL; }

ComPort::~ComPort() {
  if (hComPort != NULL) {
    // CloseHandle(hComPort);
  }
  hComPort = NULL;
}

HRESULT ComPort::connectToKernelNode(std::wstring sPortName) {
  HRESULT hr = S_OK;
  hr = FilterConnectCommunicationPort(sPortName.c_str(), 0, NULL, 0, NULL,
                                      &hComPort);
  if (FAILED(hr)) {
    fwprintf(stderr, L"Connect to kernel mode failed 0x%08x\n", hr);
    fflush(stderr);
    return hr;
  }
  return hr;
}

HRESULT ComPort::getRecord(PMFLT_EVENT_RECORD pEventRecord) {
  HRESULT hr = S_OK;
  COM_MESSAGE msg;
  memset(&msg, 0, sizeof(&msg));
  hr = FilterGetMessage(
      hComPort, &msg.header,
      sizeof(MFLT_EVENT_RECORD) + sizeof(FILTER_MESSAGE_HEADER), NULL);
  if (FAILED(hr)) {
    fwprintf(stderr, L"Get message failed 0x%08x\n", hr);
    fflush(stderr);
    return hr;
  }

  CopyMemory(pEventRecord, &msg.eventRecord, sizeof(MFLT_EVENT_RECORD));

  return hr;
}

HRESULT ComPort::disconnectFromKernelMode() {
  CloseHandle(hComPort);
  return S_OK;
}