#include "websocket/WebSocketClient.hpp"

WebSocketClient::WebSocketClient() {
  wsCurrentServerHost = L"";
  wsCurrentServerPort = L"";
  bIsHandshakeSuccessful = 0;
  bShouldStop = 0;
  wshClient = NULL;
}

WebSocketClient::~WebSocketClient() {
  if (wshClient != NULL) {
    if (bIsHandshakeSuccessful) {
      WebSocketAbortHandle(wshClient);
    }
    WebSocketDeleteHandle(wshClient);
    wshClient = NULL;
  }
  wshClient = NULL;
}

HRESULT WebSocketClient::init() {
  HRESULT hr = S_OK;

  WEB_SOCKET_HANDLE wshTempClient;

  bShouldStop = FALSE;

  pChs = new ClientHTTPSocket();
  hr = pChs->getSocketInitResult();
  if (FAILED(hr)) {
    return hr;
  }

  hr = WebSocketCreateClientHandle(NULL, 0, &wshTempClient);
  if (FAILED(hr)) {
    logNotification(
        std::format(L"WebSocketCreateClientHandle() failed 0x{:08x}",
                    (unsigned)hr),
        NOTIFICATION_ERROR_TYPE);
    if (wshTempClient != NULL) {
      WebSocketDeleteHandle(wshTempClient);
      wshTempClient = NULL;
    }
    return hr;
  }

  wshClient = wshTempClient;
  wshTempClient = NULL;

  fSendMsgFuture = std::async(&WebSocketClient::send, this);

  return hr;
}

HRESULT WebSocketClient::handshake(std::wstring wsHost, std::wstring wsPort) {
  HRESULT hr = S_OK;
  ULONG uiClientAdditionalHeadersCount = 0;
  PWEB_SOCKET_HTTP_HEADER pClientAdditionalHeaders = NULL;
  ULONG uiServerAdditionalHeadersCount = 0;
  PWEB_SOCKET_HTTP_HEADER pServerAdditionalHeaders = NULL;
  std::string sFullHostName;

  pChs->connectToHost(wsHost, wsPort);
  sFullHostName = wstrToStr(wsHost + L":" + wsPort);

  wsCurrentServerHost = wsHost;
  wsCurrentServerPort = wsPort;

  hr = WebSocketBeginClientHandshake(wshClient, NULL, 0, NULL, 0, NULL, 0,
                                     &pClientAdditionalHeaders,
                                     &uiClientAdditionalHeadersCount);
  if (FAILED(hr)) {
    logNotification(
        std::format(L"WebSocketBeginClientHandshake() failed 0x{:08x}",
                    (unsigned)hr),
        NOTIFICATION_ERROR_TYPE);
    return hr;
  }

  for (ULONG uiClientAdditionalHeaderCurrentId = 0;
       uiClientAdditionalHeaderCurrentId < uiClientAdditionalHeadersCount;
       uiClientAdditionalHeaderCurrentId++) {
    std::string key =
        pClientAdditionalHeaders[uiClientAdditionalHeaderCurrentId].pcName;
    std::string value =
        pClientAdditionalHeaders[uiClientAdditionalHeaderCurrentId].pcValue;
    value = value.substr(
        0, pClientAdditionalHeaders[uiClientAdditionalHeaderCurrentId]
               .ulValueLength);
    pChs->addRequestHeader(key, value);
  }

  pChs->sendGETRequest();
  pChs->receiveResponse();

  int iHTTPStatus = pChs->getResponseHTTPStatus();
  if (iHTTPStatus != 101) {
    logNotification(
        std::format(L"Handshaking failed with HTTP status {}", iHTTPStatus),
        NOTIFICATION_ERROR_TYPE);
    return E_FAIL;
  }

  auto umssResponseHeader = pChs->getResponseHeaders();
  uiServerAdditionalHeadersCount = (ULONG)umssResponseHeader.size();
  pServerAdditionalHeaders =
      new WEB_SOCKET_HTTP_HEADER[uiServerAdditionalHeadersCount];

  std::pair<std::string, std::string>* ppssKeyValuePairs =
      new std::pair<std::string, std::string>[uiServerAdditionalHeadersCount];
  uiServerAdditionalHeadersCount = 0;
  for (auto pssKeyValuePair : umssResponseHeader) {
    ppssKeyValuePairs[uiServerAdditionalHeadersCount++] = pssKeyValuePair;
  }

  for (ULONG i = 0; i < uiServerAdditionalHeadersCount; i++) {
    pServerAdditionalHeaders[i] = {&ppssKeyValuePairs[i].first[0],
                                   (ULONG)ppssKeyValuePairs[i].first.length(),
                                   &ppssKeyValuePairs[i].second[0],
                                   (ULONG)ppssKeyValuePairs[i].second.length()};
  }

  hr = WebSocketEndClientHandshake(wshClient, pServerAdditionalHeaders,
                                   uiServerAdditionalHeadersCount, NULL, 0,
                                   NULL);
  if (FAILED(hr)) {
    logNotification(
        std::format(L"WebSocketEndClientHandshake() failed 0x{:08x}",
                    (unsigned)hr),
        NOTIFICATION_ERROR_TYPE);
    return hr;
  }

  bIsHandshakeSuccessful = 1;
  logNotification(std::format(L"Established WebSocket connection."),
                  NOTIFICATION_INFO_TYPE);
  return hr;
}

HRESULT WebSocketClient::queueMsg(std::wstring wsMsg) {
  HRESULT hr = S_OK;

  qwsMsgQueue.push(wsMsg);

  return hr;
}

bool WebSocketClient::isHandshakeSuccessful() { return bIsHandshakeSuccessful; }

HRESULT WebSocketClient::setShouldStop() {
  logNotification(
      std::format(L"WebSocket received a stop request from the manager."),
      NOTIFICATION_INFO_TYPE);
  bShouldStop = true;
  return S_OK;
}

HRESULT WebSocketClient::runGetActionLoop() {
  HRESULT hr = S_OK, hr1 = S_OK;
  WEB_SOCKET_BUFFER pwsbBuffers[WSC_MAX_NUMBER_OF_BUFFERS] = {0};
  ULONG uiBufferCount = WSC_MAX_NUMBER_OF_BUFFERS;
  ULONG uiBytesTransfered = 0, uiBytesNeedTransferingToCurrentBuffer = 0;
  WEB_SOCKET_BUFFER_TYPE bufferType;
  WEB_SOCKET_ACTION action;
  PVOID actionContext;
  std::wstring wsMsg;

  do {
    uiBufferCount = ARRAYSIZE(pwsbBuffers);
    uiBytesTransfered = 0;

    hr = WebSocketGetAction(wshClient, WEB_SOCKET_ALL_ACTION_QUEUE, pwsbBuffers,
                            &uiBufferCount, &action, &bufferType, NULL,
                            &actionContext);
    if (FAILED(hr)) {
      logNotification(
          std::format(L"WebSocketGetAction() failed 0x{:08x}", (unsigned)hr),
                      NOTIFICATION_ERROR_TYPE);
      WebSocketAbortHandle(wshClient);
    }

    switch (action) {
      case WEB_SOCKET_NO_ACTION:
        break;
      case WEB_SOCKET_RECEIVE_FROM_NETWORK_ACTION:

        assert(uiBufferCount >= 1);

        hr1 = pChs->receiveData((LPSTR)pwsbBuffers[0].Data.pbBuffer,
                                &pwsbBuffers[0].Data.ulBufferLength,
                                &uiBytesTransfered);
        if (FAILED(hr1)) {
          uiBytesTransfered = pwsbBuffers[0].Data.ulBufferLength;
          break;
        }

        break;

      case WEB_SOCKET_INDICATE_RECEIVE_COMPLETE_ACTION:
        if (uiBufferCount != 1) {
          assert(0);
          hr = E_FAIL;
          return hr;
        }
        break;

      case WEB_SOCKET_SEND_TO_NETWORK_ACTION:
        hr1 = pChs->sendData((LPSTR)pwsbBuffers[0].Data.pbBuffer,
                             &pwsbBuffers[0].Data.ulBufferLength);
        if (FAILED(hr1)) {
          break;
        }
        uiBytesTransfered = pwsbBuffers[0].Data.ulBufferLength;
        break;

      case WEB_SOCKET_INDICATE_SEND_COMPLETE_ACTION:
        break;

      default:
        assert(0);
        hr = E_FAIL;
        break;
    }
    WebSocketCompleteAction(wshClient, actionContext, uiBytesTransfered);
  } while (action != WEB_SOCKET_NO_ACTION);
  return (FAILED(hr) ? hr : hr1);
}

HRESULT WebSocketClient::cleanup() {
  HRESULT hr;

  fSendMsgFuture.get();

  if (wshClient != NULL) {
    if (bIsHandshakeSuccessful) {
      WebSocketAbortHandle(wshClient);
    }
    WebSocketDeleteHandle(wshClient);
    wshClient = NULL;
  }
  hr = pChs->disconnectFromCurrentHost();
  if (FAILED(hr)) {
    return hr;
  }
  bIsHandshakeSuccessful = 0;
  logNotification(
      std::format(L"WebSocket connection successfully closeed."),
      NOTIFICATION_INFO_TYPE);
  return S_OK;
}

HRESULT WebSocketClient::send() {
  HRESULT hr = S_OK;
  std::wstring wsMsg;
  WEB_SOCKET_BUFFER wsbSendBuffer;

  logNotification(
      std::format(L"WebSocket message sending thread has started."),
      NOTIFICATION_INFO_TYPE);

  while (!bShouldStop) {
    if (qwsMsgQueue.empty()) {
      continue;
    }
    wsMsg = qwsMsgQueue.front();

    wsbSendBuffer.Data.pbBuffer = (PBYTE)wsMsg.c_str();
    wsbSendBuffer.Data.ulBufferLength = (ULONG)wsMsg.size() * sizeof(WCHAR);

    // Note that WebSocketSend does not actually send message, but queues a
    // WEB_SOCKET_SEND_TO_NETWORK_CONNECTION and must be processed by
    // runGetActionLoop()
    hr = WebSocketSend(wshClient, WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE,
                       &wsbSendBuffer, NULL);
    if (FAILED(hr)) {
      logNotification(
          std::format(L"WebSocketSend() failed 0x{:08x}", (unsigned)hr),
                      NOTIFICATION_ERROR_TYPE);
      return hr;
    }

    hr = runGetActionLoop();
    if (FAILED(hr)) {
      while (reconnect() && !bShouldStop) {
        Sleep(WSC_RETRY_INTERVAL);
      }
      continue;
      // return hr;
    }

    // Note that WebSocketReceive does not actually send message, but queue a
    // WEB_SOCKET_RECEIVE_FROMs_NETWORK_CONNECTION and must be processed by
    // runGetActionLoop()
    hr = WebSocketReceive(wshClient, NULL, NULL);
    if (FAILED(hr)) {
      logNotification(
          std::format(L"WebSocketReceive() failed 0x{:08x}", (unsigned)hr),
                      NOTIFICATION_ERROR_TYPE);
      return hr;
    }

    hr = runGetActionLoop();
    if (FAILED(hr)) {
      while (reconnect() && !bShouldStop) {
        Sleep(WSC_RETRY_INTERVAL);
      }
      continue;
      // return hr;
    }

    qwsMsgQueue.pop();
  }
  logNotification(
      std::format(L"WebSocket message sending thread has stopped."),
      NOTIFICATION_INFO_TYPE);
  return hr;
}

HRESULT WebSocketClient::reconnect() {
  HRESULT hr = S_OK;
  WEB_SOCKET_HANDLE wshTempClient;

  logNotification(std::format(L"Attempting to reconnect to the server"),
                  NOTIFICATION_INFO_TYPE);

  delete pChs;
  pChs = new ClientHTTPSocket();
  hr = pChs->getSocketInitResult();
  if (FAILED(hr)) {
    return hr;
  }

  WebSocketAbortHandle(wshClient);
  WebSocketDeleteHandle(wshClient);

  hr = WebSocketCreateClientHandle(NULL, 0, &wshTempClient);
  if (FAILED(hr)) {
    logNotification(
        std::format(L"WebSocketCreateClientHandle() failed 0x{:08x}",
                    (unsigned)hr),
        NOTIFICATION_ERROR_TYPE);
    if (wshTempClient != NULL) {
      WebSocketDeleteHandle(wshTempClient);
      wshTempClient = NULL;
    }
    return hr;
  }

  wshClient = wshTempClient;

  hr = handshake(wsCurrentServerHost, wsCurrentServerPort);
  if (FAILED(hr)) {
    return hr;
  }

  return hr;
}