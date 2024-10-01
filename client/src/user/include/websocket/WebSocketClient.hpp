#ifndef WEB_SOCKET_CLIENT_HPP
#define WEB_SOCKET_CLIENT_HPP

#include <winsock2.h>
#include <websocket.h>

#include <future>
#include <iostream>
#include <queue>
#include <cassert>

#include "socket/ClientHTTPSocket.hpp"

const int WSC_MAX_BUFFER_SIZE = 1024;
const int WSC_MAX_NUMBER_OF_BUFFERS = 2;
const int WSC_RETRY_INTERVAL = 1000;

class WebSocketClient {
 private:
  // The current server host name.
  std::wstring wsCurrentServerHost;

  // The current server host port.
  std::wstring wsCurrentServerPort;

  // The handle for the client WebSocket socket. It does not actually send
  // message, but only monitoring the connection based information (websocket
  // key, headers, actions, ...).
  WEB_SOCKET_HANDLE wshClient;

  // Pointer to the socket object for the client. Used to send messages.
  ClientHTTPSocket *pChs;

  // Check if the handshake was successful and the client is ready to send
  // messages
  bool bIsHandshakeSuccessful;

  // Check if a stop request is send to the web socket client.
  bool bShouldStop;

  // The queue containing message pending to be sent.
  std::queue<std::wstring> qwsMsgQueue;

  // The future object monitoring the message-sending thread.
  std::future<HRESULT> fSendMsgFuture;

  /**
   * A loop function that do the jobs queued by WebSocketSend and
   * WebSocketReceive.
   *
   * In this loop, first, WebSocketGetAction is called. It will get the action
   * that should be done in favor of a previous WebSocketSend or
   * WebSocketReceive call. The function then process them, by sending messages
   * through the socket for WebSocketSend and receiving messages through the
   * socket for WebSocketReceive. Finally, a WebSocketCompleteAction call will
   * notify the WebSocket library that the action is complete, and it will
   * decide the next action should be done. The operation is completed when no
   * more action queued by the two previous functions left.
   *
   * Any call of WebSocketSend and WebSocketReceive should ALWAYS be followed by
   * a call of this function.
   *
   * No parameter is needed as any data used for processing are monitored by the
   * WebSocket library.
   *
   * @return S_OK if the operation is successful, an error code/HRESULT if fail.
   */
  HRESULT runGetActionLoop();

  /**
   * Send messages whenever the message queue is not empty.
   *
   * This function is run in an independent thread. It performs a loop that will
   * send messages whenever qwsMsgQueue is not empty. Message sending operations
   * are WebSocketSend -> runGetActionLoop -> WebSocketReceive ->
   * runGetActionLoop
   *
   * @return S_OK if the operation is successful, an error code/HRESULT if fail.
   */
  HRESULT send();

  /**
   * Trying to reconnect when there are problems with the connection or the
   * socket.
   *
   * This function will actually abort the current socket and WebSocket handle,
   * trying to create new ones then finally perform handshake again. Everything
   * will be done once, so an additional wrapping loop should be used to retry
   * many times.
   *
   * @return S_OK if the operation is successful, an error code/HRESULT if fail.
   */
  HRESULT reconnect();

 public:
  /**
   * Default constructor
   */
  WebSocketClient();

  /**
   * Default destructor
   */
  ~WebSocketClient();

  /**
   * Initialize the components needed for a connection.
   *
   * Components initialized are: Windows socket (ClientHTTPSocket object), the
   * WebSocket handler, and the message sending thread.
   *
   * @return S_OK if the operation is successful, an error code/HRESULT if fail.
   */
  HRESULT init();

  /**
   * Performing handshake with the server.
   *
   * The handshaking operations including two steps. First, a HTTP GET request
   * is send to the server, containing "Upgrade: WebSocket", "Sec-Key: <key>"
   * headers. Then, the server will respond with a status of 101, implying that
   * the handshking has sucessfully completed.
   *
   * @param wsHost The server's host address
   * @param wsPort The server's port
   *
   * @return S_OK if the operation is successful, an error code/HRESULT if fail.
   */
  HRESULT handshake(std::wstring wsHost, std::wstring wsPort);

  /**
   * Adding a message to the message queue. It will be send to the server when
   * the time comes.
   *
   * Messages sent this way is encrypted and cannot be read directly by packet
   * sniffing.
   *
   * @param wsMsg The message needs sending (not encrypted)
   *
   * @return S_OK if the operation is successful, an error code/HRESULT if fail.
   */
  HRESULT queueMsg(std::wstring wsMsg);

  /**
   * Cleaning up unused handles at the end of the connection.
   *
   * @return S_OK if the operation is successful, an error code/HRESULT if fail.
   */
  HRESULT cleanup();

  /**
   * Checking if the last handshake is successful.
   *
   * @return 1 if it was successful, 0 if it was unsucessful or no handshake was
   * performed.
   */
  bool isHandshakeSuccessful();

  /**
   * Attempting to close the connection and clean the socket.
   *
   * @return S_OK if the operation is successful, an error code/HRESULT if fail.
   */
  HRESULT setShouldStop();
};

#endif