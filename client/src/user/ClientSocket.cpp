#include "websocket/socket/ClientSocket.hpp"

ClientSocket::ClientSocket() {
  hrSocketInitResult = S_OK;
  WSADATA wsaData;
  memset(&wsaData, 0, sizeof(&wsaData));

  int iResult = 0;
  iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
  if (iResult) {
    hrSocketInitResult = HRESULT_FROM_WIN32(iResult);
    fwprintf(stderr, L"WSAStartup failed: 0x%08x\n", hrSocketInitResult);
  }

  connectSocket = INVALID_SOCKET;
}

ClientSocket::~ClientSocket() {
  if (connectSocket != INVALID_SOCKET) {
    closesocket(connectSocket);
  }
  connectSocket = INVALID_SOCKET;
  WSACleanup();
}

HRESULT ClientSocket::getSocketInitResult() { return hrSocketInitResult; }

HRESULT ClientSocket::connectToHost(std::wstring wsHost, std::wstring wsPort) {
  int iResult = 0;
  ADDRINFOW *pResult = NULL, *ptr = NULL, hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;

  iResult = GetAddrInfo(wsHost.c_str(), wsPort.c_str(), &hints, &pResult);
  if (iResult) {
    fwprintf(stderr, L"GetAddrInfo failed: 0x%08x\n", HRESULT_FROM_WIN32(iResult));
    return E_FAIL;
  }

  ptr = pResult;
  connectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);

  if (connectSocket == INVALID_SOCKET) {
    HRESULT hrLastError = HRESULT_FROM_WIN32(WSAGetLastError());
    fwprintf(stderr, L"Error at socket(): 0x%08x\n", hrLastError);
    FreeAddrInfo(pResult);
    return hrLastError;
  }

  iResult = connect(connectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);

  if (iResult) {
    HRESULT hrLastError = HRESULT_FROM_WIN32(WSAGetLastError());
    fwprintf(stderr, L"Connect to server failed: 0x%08x\n", hrLastError);
    FreeAddrInfo(pResult);
    return hrLastError;
  }
  FreeAddrInfo(pResult);
  return S_OK;
}

HRESULT ClientSocket::disconnectFromCurrentHost() {
  int iResult = 0;
  iResult = shutdown(connectSocket, SD_SEND);
  if (iResult) {
    HRESULT hrLastError = HRESULT_FROM_WIN32(WSAGetLastError());
    fwprintf(stderr, L"Connection shutdown failed 0x%08x\n", hrLastError);
    return hrLastError;
  }
  return S_OK;
}

HRESULT ClientSocket::sendMsg(std::wstring* pwsMsg) {
  int iResult = 0;
  iResult = send(connectSocket, (LPSTR)pwsMsg->c_str(),
                 (int)pwsMsg->length() * sizeof(WCHAR), 0);
  if (iResult == SOCKET_ERROR) {
    HRESULT hrLastError = HRESULT_FROM_WIN32(WSAGetLastError());
    fwprintf(stderr, L"Send message failed 0x%08x\n", hrLastError);
    return hrLastError;
  } else {
    // fwprintf(stderr, L"Message sent: %ws", pwsMsg->c_str());
  }
  return S_OK;
}

HRESULT ClientSocket::receiveMsg(std::wstring* pwsMsg) {
  HRESULT hr = S_OK;
  int iResult = 0;
  CHAR cBuffer[1024];

  ZeroMemory(cBuffer, sizeof(cBuffer));
  iResult = recv(connectSocket, cBuffer, 1024, 0);
  if (iResult == 0) {
    hr = E_FAIL;
    fwprintf(stderr, L"Receiving failed due to connection closed\n");
    return hr;
  } else if (iResult < 0) {
    hr = HRESULT_FROM_WIN32(WSAGetLastError());
    fwprintf(stderr, L"Receive HTTP response failed 0x%08x", hr);
    return hr;
  }
  *pwsMsg = (LPWSTR)cBuffer;
  return hr;
}

HRESULT ClientSocket::sendData(LPSTR pcBuffer, PULONG puiBufferLengthSend) {
  int iResult = 0;
  iResult = send(connectSocket, pcBuffer, *puiBufferLengthSend, 0);
  if (iResult == SOCKET_ERROR) {
    HRESULT hrLastError = HRESULT_FROM_WIN32(WSAGetLastError());
    fwprintf(stderr, L"Send message failed 0x%08x\n", hrLastError);
    return hrLastError;
  } else {
    // fwprintf(stderr, L"Message sent: %ws", pwsMsg->c_str());
  }
  return S_OK;
}

HRESULT ClientSocket::receiveData(LPSTR pcBuffer, PULONG puiBufferMaximumLength,
                                  PULONG puiBufferLengthReceived) {
  HRESULT hr = S_OK;
  int iResult = 0;

  iResult = recv(connectSocket, pcBuffer, *puiBufferMaximumLength, 0);
  if (iResult == 0) {
    hr = E_FAIL;
    fwprintf(stderr, L"Receiving failed due to connection closed\n");
    return hr;
  } else if (iResult < 0) {
    hr = HRESULT_FROM_WIN32(WSAGetLastError());
    fwprintf(stderr, L"Receive HTTP response failed 0x%08x", hr);
    return hr;
  }
  *puiBufferLengthReceived = iResult;
  return hr;
}