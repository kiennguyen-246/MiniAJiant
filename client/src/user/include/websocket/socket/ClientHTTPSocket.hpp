#ifndef CLIENT_HTTP_SOCKET_HPP
#define CLIENT_HTTP_SOCKET_HPP

#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>

#include "ClientSocket.hpp"
#include "utils/helpers.hpp"

const int CHS_MAX_MESSAGE_SIZE = 2048;
const int CHS_CONTENT_LENGTH = 1024;

class ClientHTTPSocket : public ClientSocket {
 private:
  // Hash table structure containing headers using for HTTP request
  std::unordered_map<std::string, std::string> umssRequestHeaders;

  // Hash table structure containing headers using for HTTP response
  std::unordered_map<std::string, std::string> umssResponseHeaders;

  // Buffer containing data for HTTP requests
  CHAR pcRequestBuffer[CHS_CONTENT_LENGTH];

  // Buffer containing data for HTTP responses
  CHAR pcResponseBuffer[CHS_CONTENT_LENGTH];

  // The returned HTTP status for responses
  int iResponseHTTPStatus;

 public:
  /**
   * Default constructor
   *
   * This function also initialize the socket. To get the result of the
   * initialization socket, call getSocketInitResult().
   */
  ClientHTTPSocket();

  /**
   * Default destructor
   *
   * This function also do the cleanup work for the socket.
   */
  ~ClientHTTPSocket();

  /**
   * Add a header to the saved set of request headers. These headers will be
   * used when a HTTP request is sent.
   *
   * @param sKey The key part of the headers. May be strings like "Host",
   * "Content-Length", ...
   * @param sKey The value part of the headers.
   *
   * @return S_OK if the operation is successful, an error code/HRESULT if fail.
   */
  HRESULT addRequestHeader(std::string sKey, std::string sValue);

  /**
   * Remove a header from the saved set of headers. These headers will be used
   * when a HTTP request is sent.
   *
   * @param sKey The key part of the headers. May be strings like "Host",
   * "Content-Length", ...
   *
   * @return S_OK if the operation is successful, an error code/HRESULT if fail.
   */
  HRESULT removeRequestHeader(std::string skey);

  /**
   * Get the response header of the last receiveRespond() call.
   *
   * @return The response headers of the last receiveRespond() call.
   */
  std::unordered_map<std::string, std::string> getResponseHeaders();

  /**
   * Get the response HTTP status of the last receiveRespond() call.
   *
   * @return The response HTTP status of the last receiveRespond() call.
   */
  int getResponseHTTPStatus();

  /**
   * Connect to a host socket at a given host address and port.
   *
   * This function do the same connection operations that a normal socket would
   * do, and only add one "Host" header line.
   *
   * @param wsHost The host address
   * @param wsPort The host port
   *
   * @return S_OK if the operation is successful, an error code/HRESULT if fail.
   */
  HRESULT connectToHost(std::wstring wsHost, std::wstring wsPort);

  /**
   * Disconnect from the host socket currently being connected by the socket.
   *
   * This function do the same connection operations that a normal socket would
   * do, and only add one "Host" header line.
   *
   * @return S_OK if the operation is successful, an error code/HRESULT if fail.
   */
  HRESULT disconnectFromCurrentHost();

  /**
   * Send a HTTP GET request to the host.
   *
   * This function only sends data and will fail only if the socket is forced to
   * be aborted by the local machine. Messages send this way is not
   * encrypted.
   *
   * @return S_OK if the operation is successful, an error code/HRESULT if fail.
   */
  HRESULT sendGETRequest();

  /**
   * Receive the data from the host through the socket.
   *
   * The receiving operation may fail if the server is not available. If timed
   * out, the current socket will be aborted. Messages send this way is not
   * encrypted.
   *
   * @return S_OK if the operation is successful, an error code/HRESULT if fail.
   */
  HRESULT receiveResponse();
};

#endif  // !CLIENT_HTTP_SOCKET_HPP
