#include "websocket/socket/ClientHTTPSocket.hpp"

ClientHTTPSocket::ClientHTTPSocket() {
  umssRequestHeaders.clear();
  umssResponseHeaders.clear();
  iResponseHTTPStatus = 0;

  // umssRequestHeaders.insert(
  //     {"Content-Length", std::to_string(CHS_CONTENT_LENGTH)});
}

ClientHTTPSocket::~ClientHTTPSocket() {
  umssRequestHeaders.clear();
  umssResponseHeaders.clear();
}

HRESULT ClientHTTPSocket::addRequestHeader(std::string key, std::string value) {
  umssRequestHeaders.insert({key, value});
  return S_OK;
}

HRESULT ClientHTTPSocket::removeRequestHeader(std::string key) {
  if (umssRequestHeaders.find(key) != umssRequestHeaders.end()) {
    umssRequestHeaders.erase(key);
  }
  return S_OK;
}

std::unordered_map<std::string, std::string>
ClientHTTPSocket::getResponseHeaders() {
  return umssResponseHeaders;
}

int ClientHTTPSocket::getResponseHTTPStatus() { return iResponseHTTPStatus; }

HRESULT ClientHTTPSocket::connectToHost(std::wstring wsHost,
                                        std::wstring wsPort) {
  HRESULT hr = S_OK;
  ClientSocket::connectToHost(wsHost, wsPort);
  if (FAILED(hr)) {
    return hr;
  }
  int iStrlen = 0;
  std::string sHost = wstrToStr(wsHost);
  std::string sPort = wstrToStr(wsPort);
  umssRequestHeaders.insert({"Host", sHost + ":" + sPort});
  return hr;
}

HRESULT ClientHTTPSocket::disconnectFromCurrentHost() {
  HRESULT hr = S_OK;
  hr = ClientSocket::disconnectFromCurrentHost();
  if (FAILED(hr)) {
    return hr;
  }
  umssRequestHeaders.clear();
  umssResponseHeaders.clear();
  return hr;
}

HRESULT ClientHTTPSocket::sendGETRequest() {
  HRESULT hr = S_OK;

  CHAR pcMsg[CHS_MAX_MESSAGE_SIZE];
  memset(pcMsg, 0, sizeof(pcMsg));

  std::string sReqLine = "GET / HTTP/1.1\r\n";
  strcat_s(pcMsg, CHS_MAX_MESSAGE_SIZE, sReqLine.c_str());
  for (auto pssEntry : umssRequestHeaders) {
    std::string sCurHeaderLine =
        pssEntry.first + ": " + pssEntry.second + "\r\n";
    strcat_s(pcMsg, CHS_MAX_MESSAGE_SIZE, sCurHeaderLine.c_str());
  }
  strcat_s(pcMsg, CHS_MAX_MESSAGE_SIZE, "\r\n");

  int iResult = send(connectSocket, pcMsg, (int)strlen(pcMsg), 0);
  if (iResult == SOCKET_ERROR) {
    hr = HRESULT_FROM_WIN32(WSAGetLastError());
    logNotification(std::format(L"winsock2 send() failed 0x{:08x}", (unsigned)hr),
                    NOTIFICATION_ERROR_TYPE);
    return hr;
  } else {
    //logNotification(std::format(L"{}", pcMsg),
    //                NOTIFICATION_INFO_TYPE);
  }
  return hr;
}

HRESULT ClientHTTPSocket::receiveResponse() {
  HRESULT hr = S_OK;
  CHAR pcBuffer[CHS_MAX_MESSAGE_SIZE];
  int iResult = recv(connectSocket, pcBuffer, CHS_MAX_MESSAGE_SIZE, 0);
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

  std::istringstream issBuffer(pcBuffer);
  std::string sLine;
  int iLineId = 0;
  umssResponseHeaders.clear();
  while (std::getline(issBuffer, sLine, '\n')) {
    if ((int)sLine.length() == 1) {
      break;
    }
    std::istringstream issLine(sLine);
    if (!iLineId) {
      std::string sHTTPVersion, sStatus;
      issLine >> sHTTPVersion;
      issLine >> sStatus;
      iResponseHTTPStatus = atoi(&sStatus[0]);
    } else {
      std::string sKey, sValue = "", sValueWord;
      issLine >> sKey;
      sKey.pop_back();
      while (issLine >> sValueWord) {
        sValue += sValueWord;
      }
      umssResponseHeaders.insert({sKey, sValue});
    }

    ++iLineId;
  }
  return hr;
}