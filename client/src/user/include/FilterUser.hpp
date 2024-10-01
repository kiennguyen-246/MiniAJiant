#ifndef MFLT_USER_HPP
#define MFLT_USER_HPP

#include <WinSock2.h>

#include <format>
#include <fstream>
#include <future>
#include <map>

#include "driver/ComPort.hpp"
#include "utils/JSONObj.hpp"
#include "websocket/WebSocketClient.hpp"

const WCHAR DEFAULT_LOG_FILE_PATH[] = L".\\logs\\events.log";
const int MD5_HASH_LENGTH = 16;
const int SHA1_HASH_LENGTH = 20;
const int SHA256_HASH_LENGTH = 32;

typedef struct _LANGUAGE_CODE_PAGE_STRUCT {
  WORD wLanguage;
  WORD wCodePage;
} LANGUAGE_CODE_PAGE_STRUCT, *PLANGUAGE_CODE_PAGE_STRUCT;

class FilterUser {
 public:
  /**
   * Construct a FilterUser object from a given minifilter driver name and
   * communication port name.
   *
   * @param ws__FilterName The minifilter driver's name
   * @param ws__ComPortName The communication port's name, in the form of
   * "\\<name>"
   */
  FilterUser(std::wstring ws__FilterName, std::wstring ws__ComPortName);

  /**
   * Default destructor
   */
  ~FilterUser();

  /**
   * Load the filter.
   *
   * This function is equivalent to "fltmc load <filter>"
   *
   * @return S_OK if the operation is successful, an error code/HRESULT if fail.
   */
  HRESULT loadFilter();

  /**
   * Unload the filter.
   *
   * This function is equivalent to "fltmc unload <filter>"
   *
   * @return S_OK if the operation is successful, an error code/HRESULT if fail.
   */
  HRESULT unloadFilter();

  /**
   * Establishing a WebSocket connection to the server.
   *
   * @param wsHost The server's host address
   * @param wsPort The server's port
   *
   * @return S_OK if the operation is successful, an error code/HRESULT if fail.
   */
  HRESULT connectToServer(std::wstring wsHost, std::wstring wsPort);

  /**
   * Disconnecting from the current server.
   *
   * @return S_OK if the operation is successful, an error code/HRESULT if fail.
   */
  HRESULT disconnectFromServer();

  /**
   * Running a loop that do the main task of the server.
   *
   * Operations are done are: get a record of the event from the kernel mode ->
   * add more information to the record -> queuing this record to be sent to the
   * server.
   *
   * @return S_OK if the operation is successful, an error code/HRESULT if fail.
   */
  HRESULT doMainRoutine();

  /**
   * Attempting to stop the client.
   *
   * @return S_OK if the operation is successful, an error code/HRESULT if fail.
   */
  HRESULT setShouldStop();

 private:
  // The communication port object
  ComPort cp;

  // Name of the minifilter driver
  std::wstring wsFilterName;

  // Name of the communication port
  std::wstring wsComPortName;

  // Path to the log file
  std::wstring wsLogFilePath;

  // The std::wofstream object that open the log file
  std::wofstream wfsLog;

  // Check if the filter driver is sucessfully loaded
  bool bIsFilterLoaded;

  // Check if the application is connected to the communication port
  bool bIsComPortConnected;

  // Check if the main routine loop should stop by the manager
  bool bShouldStop;

  // Exclusive ID for each event record
  ULONG uiEventId;

  // WebSocketClient object, monitoring WebSocket connection
  WebSocketClient wsc;

  /**
   * Gain privileges for the application to load the driver.
   *
   * By default, MSVC applications cannot load drivers by calling FilterLoad.
   * This function must be called to make this possible.
   *
   * @param hToken Handle for the access token of the process running the
   * driver.
   * @param pwcPrivilege WCHAR* string represent the privilege needed. To load
   * driver, it should be SE_LOAD_DRIVER_NAME.
   * @param bIsPrivilegeEnabled New value for the privilege. Set to TRUE to load
   * driver.
   *
   * @return S_OK if the operation is successful, an error code/HRESULT if fail.
   */
  HRESULT setPrivilege(HANDLE hToken, LPCWSTR pwcPrivilege,
                       BOOL bIsPrivilegeEnabled);

  /**
   * Gather more information about a CreateProcess event, then compose a JSON
   * stringified message to send to the server.
   *
   * Additional information gathered: current computer's name, process image
   * file attributes (creation time, attribute mask), process image file version
   * info (producer, version number, description, original file name), file hash
   * (MD5, SHA1, SHA256)
   *
   * @param pEventRecord Record of the event.
   * @param pwsMsg The returned JSON stringified message composed.
   *
   * @return S_OK if the operation is successful, an error code/HRESULT if fail.
   */
  HRESULT composeEventLog(PMFLT_EVENT_RECORD pEventRecord,
                          std::wstring* pwsMsg);

  /**
   * Set some additional information for CreateProcess logging event.
   *
   * @param pEventRecord Record of the event.
   * @param pJsObj The JSON object containing the information being recorded.
   *
   * @return S_OK if the operation is successful, an error code/HRESULT if fail.
   */
  HRESULT logCreateProcessEvent(PMFLT_EVENT_RECORD pEventRecord,
                                JSONObj* pJsObj);

  /**
   * Get the file version infomation of some file, given a file version
   * information block obtained from GetFileVersionInfo and the first language
   * get from using VerQueryValue for the directory of
   * "\\VarFileInfo\\Translatrion"
   *
   * To get an entry of the file version information, users should call
   * GetFileVersionInfoSize first to get the size of the information block, then
   * call GetFileVersionInfo to obtain the block. The block contains the
   * information in multiple languages, as well as code pages. Users then need
   * to call VerQueryInfo at the directory of "\\VarFileInfo\\Translatrion" to
   * get the list of the language-code page pairs available. Finally, call this
   * function to get the information needed.
   *
   * @param lpFileVersionInfoBlock The file version information block obtained
   * from GetFileVersionInfo
   * @param lpLangCodePage A pointer to the LANGUAGE_CODE_PAGE_STRUCT struct
   * (see above) containing the language/code page used for querying the file
   * version information entries.
   * @param uiLangCodePageSize Size of the object that lpLangCodePage points to
   * @param wsQueryEntry The file version info entry need querying. Refer to
   * VerQueryInfo on Microsoft Learn for strings that can be used for this
   * parameter.
   *
   * @return A string contain the provided entry of the file version
   * information. If the function fails, the return value is an empty string.
   */
  std::wstring getFileVersionInfoEntry(
      PVOID lpFileVersionInfoBlock, PLANGUAGE_CODE_PAGE_STRUCT lpLangCodePage,
      ULONG uiLangCodePageSize, std::wstring wsQueryEntry);

  /**
   * Get the hash value of a file.
   *
   * @param wsFileName The absolute path to the file.
   * @param hashAlg The hashing algorithm. Refer to ALG_ID on Microsoft Learn
   * for suitable values.
   *
   * @return A string containing the hash value for the file.
   */
  std::wstring getFileHash(std::wstring wsFileName, ALG_ID hashAlg);
};

#endif