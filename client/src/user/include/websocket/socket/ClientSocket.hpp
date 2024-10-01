#ifndef SOCKET_HPP
#define SOCKET_HPP

#include <winsock2.h>
#include <ws2tcpip.h>

#include <iostream>
#include <utils/helpers.hpp>

class ClientSocket {
 public:
  /**
   * Default constructor
   *
   * This function also initialize the socket. To get the result of the
   * initialization socket, call getSocketInitResult().
   */
  ClientSocket();

  /**
   * Default destructor
   *
   * This function also do the cleanup work for the socket.
   */
  ~ClientSocket();

  /**
   * Get the result of the initialization for the socket.
   *
   * @return S_OK if the initialization process is successful, an error
   * code/HRESULT if fail.
   */
  HRESULT getSocketInitResult();

  /**
   * Connect to a host socket at a given host address and port.
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
   * @return S_OK if the operation is successful, an error code/HRESULT if fail.
   */
  HRESULT disconnectFromCurrentHost();

  /**
   * Send some data through the socket to the host.
   *
   * This function only sends data and will fail only if the socket is forced to
   * be aborted by the local machine.
   *
   * @param pcBuffer Pointer to the buffer containing the data need sending
   * @param puiBufferLengthSend Pointer to the length of the data need sending
   * on the buffer
   *
   * @return S_OK if the operation is successful, an error code/HRESULT if fail.
   */
  HRESULT sendData(LPSTR pcBuffer, PULONG puiBufferLengthSend);

  /**
   * Receive the data from the host through the socket.
   *
   * The receiving operation may fail if the server is not available. If timed
   * out, the current socket will be aborted.
   *
   * @param pcBuffer Pointer to the buffer containing the data that will be
   * received
   * @param pcBufferMaximumLength Pointer to the maximum length of the buffer
   * used to receive the message. Error will occur if the received data is
   * larger than this value.
   * @param puiBufferLengthReceived Pointer to the length of the data received
   * through the buffer at the end of the receiving operation
   *
   * @return S_OK if the operation is successful, an error code/HRESULT if fail.
   */
  HRESULT receiveData(LPSTR pcBuffer, PULONG puiBufferMaximumLength,
                      PULONG puiBufferLengthReceived);

  /**
   * Send a single text message to the host.
   *
   * This function function the same as sendData, but is optimized for sending
   * message only. Message send this way will not be encrypted and is readable
   * through packet sniffing.
   *
   * @param pwsMsg Pointer to the message that need to be send
   *
   * @return S_OK if the operation is successful, an error code/HRESULT if fail.
   */
  HRESULT sendMsg(std::wstring* pwsMsg);

  /**
   * Receive a single text message from the host.
   *
   * This function function the same as receiveData, but is optimized for
   * sending message only. Message send this way will not be encrypted and is
   * readable through packet sniffing.
   *
   * @param pwsMsg Pointer to the message that will be received at the end of
   * the operation.
   *
   * @return S_OK if the operation is successful, an error code/HRESULT if fail.
   */
  HRESULT receiveMsg(std::wstring* pwsMsg);

 private:
  // Initialize result.
  HRESULT hrSocketInitResult;

 protected:
  // The socket object.
  SOCKET connectSocket;
};

#endif