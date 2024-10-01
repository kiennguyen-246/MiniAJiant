#include "websocket/socket/ClientSocket.hpp"

ClientSocket::ClientSocket() {
  hrSocketInitResult = S_OK;
  WSADATA wsaData;
  memset(&wsaData, 0, sizeof(&wsaData));

  int iResult = 0;
  iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
  if (iResult) {
    hrSocketInitResult = HRESULT_FROM_WIN32(iResult);
    logNotification(std::format(L"WSAStartup failed 0x{:08x}",
                                (unsigned)hrSocketInitResult),
        NOTIFICATION_ERROR_TYPE);
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
    logNotification(std::format(L"GetAddrInfo failed: 0x{:08x}",
                                (unsigned)HRESULT_FROM_WIN32(iResult)),
                    NOTIFICATION_ERROR_TYPE);
    return E_FAIL;
  }

  ptr = pResult;
  connectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);

  if (connectSocket == INVALID_SOCKET) {
    HRESULT hrLastError = HRESULT_FROM_WIN32(WSAGetLastError());
    logNotification(std::format(L"winsock2 socket() failed 0x{:08x}",
                                (unsigned)hrLastError),
        NOTIFICATION_ERROR_TYPE);
    FreeAddrInfo(pResult);
    return hrLastError;
  }

  iResult = connect(connectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);

  if (iResult) {
    HRESULT hrLastError = HRESULT_FROM_WIN32(WSAGetLastError());
    logNotification(std::format(L"winsock2 connect() failed 0x{:08x}",
                                (unsigned)hrLastError),
        NOTIFICATION_ERROR_TYPE);
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
    logNotification(std::format(L"winsock2 shutdown() failed 0x{:08x}",
                                (unsigned)hrLastError),
        NOTIFICATION_ERROR_TYPE);
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
    logNotification(
        std::format(L"winsock2 send() failed 0x{:08x}", (unsigned)hrLastError),
        NOTIFICATION_ERROR_TYPE);
    return hrLastError;
  } else {
    logNotification(std::format(L"Message sent {}", *pwsMsg),
                    NOTIFICATION_DEBUG_TYPE);
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
    logNotification(
        std::format(L"winsock2 recv() failed due to connection closed"),
        NOTIFICATION_ERROR_TYPE);
    return hr;
  } else if (iResult < 0) {
    hr = HRESULT_FROM_WIN32(WSAGetLastError());
    logNotification(
        std::format(L"winsock2 recv() failed 0x{:08x}", (unsigned)hr),
                    NOTIFICATION_ERROR_TYPE);
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
    logNotification(
        std::format(L"winsock2 send() failed 0x{:08x}", (unsigned)hrLastError),
        NOTIFICATION_ERROR_TYPE);
    return hrLastError;
  } else {
      // do nothing
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
    logNotification(
        std::format(L"winsock2 recv() failed due to connection closed"),
        NOTIFICATION_ERROR_TYPE);
    return hr;
  } else if (iResult < 0) {
    hr = HRESULT_FROM_WIN32(WSAGetLastError());
    logNotification(
        std::format(L"winsock2 recv() failed 0x{:08x}", (unsigned)hr),
                    NOTIFICATION_ERROR_TYPE);
    return hr;
  }
  *puiBufferLengthReceived = iResult;
  return hr;
}