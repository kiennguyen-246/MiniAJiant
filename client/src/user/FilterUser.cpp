#include "FilterUser.hpp"

FilterUser::FilterUser(std::wstring ws__FilterName,
                       std::wstring ws__ComPortName) {
  srand((int)time(0));
  wsFilterName = ws__FilterName;
  wsComPortName = ws__ComPortName;
  wsLogFilePath = DEFAULT_LOG_FILE_PATH;
  wfsLog.open(wsLogFilePath);
  bIsFilterLoaded = false;
  bIsComPortConnected = false;
  bShouldStop = false;

  int u = rand();
  int v = rand();
  int w = rand();
  uiEventId = (1ll * u * v + w) % (int)1e9;
}

FilterUser::~FilterUser() {
  bShouldStop = false;
  wfsLog.close();
  if (bIsComPortConnected) {
    cp.disconnectFromKernelMode();
  }
  if (bIsFilterLoaded) {
    unloadFilter();
  }
}

HRESULT FilterUser::loadFilter() {
  HRESULT hr = S_OK;

  HANDLE hToken = NULL;
  if (!OpenProcessToken(GetCurrentProcess(),
                        TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, &hToken)) {
    hr = HRESULT_FROM_WIN32(GetLastError());
    fwprintf(stderr, L"Get access token failed 0x%08x\n", hr);
    return hr;
  }
  setPrivilege(hToken, SE_LOAD_DRIVER_NAME, TRUE);

  hr = FilterLoad(wsFilterName.c_str());
  if (FAILED(hr)) {
    if (hr == HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS)) {
      //fwprintf(stderr,
      //         L"Driver already loaded and will no longer need loading.\n");
    } else {
      fwprintf(stderr, L"Load filter failed 0x%08x\n", hr);
      return hr;
    }
  }
  bIsFilterLoaded = 1;
  fwprintf(stderr, L"Filter loaded\n");
  fflush(stdout);

  hr = cp.connectToKernelNode(wsComPortName.c_str());
  if (FAILED(hr)) {
    fwprintf(stderr, L"Connection to kernel mode failed 0x%08x\n", hr);
    return hr;
  }
  bIsComPortConnected = 1;
  //fwprintf(stderr, L"Connection to kernel mode established\n");
  fflush(stdout);

  return hr;
}

HRESULT FilterUser::connectToServer(std::wstring wsHost, std::wstring wsPort) {
  HRESULT hr = S_OK;

  hr = wsc.init();
  if (FAILED(hr)) {
    fwprintf(stderr, L"WebSocket initialization failed 0x%08x", hr);
    return hr;
  }

  hr = wsc.handshake(wsHost, wsPort);
  if (FAILED(hr)) {
    return hr;
  }

  return hr;
}

HRESULT FilterUser::disconnectFromServer() {
  HRESULT hr = S_OK;

  hr = wsc.cleanup();
  if (FAILED(hr)) {
    return hr;
  }

  return hr;
}

HRESULT FilterUser::unloadFilter() {
  HRESULT hr = S_OK;

  hr = cp.disconnectFromKernelMode();
  bIsComPortConnected = 0;

  hr = FilterUnload(wsFilterName.c_str());
  if (FAILED(hr)) {
    fwprintf(stderr, L"FilterUnload failed 0x%08x\n", hr);
    return hr;
  }
  bIsFilterLoaded = 0;
  return hr;
}

HRESULT FilterUser::doMainRoutine() {
  HRESULT hr = S_OK;
  MFLT_EVENT_RECORD eventRecord;
  while (1) {
    if (bShouldStop) {
      break;
    }

    ZeroMemory(&eventRecord, sizeof(eventRecord));

    hr = cp.getRecord(&eventRecord);
    if (FAILED(hr)) {
      return hr;
    }

    std::wstring wsMsg;
    hr = composeEventLog(&eventRecord, &wsMsg);

     //fwprintf(stderr, L"%ws\n", wsMsg.c_str());
     //fflush(stdout);

    wfsLog << wsMsg << L"\n";
    wfsLog.flush();

    if (wsc.isHandshakeSuccessful()) {
      wsc.queueMsg(wsMsg);
    }
  }

  bShouldStop = false;

  return hr;
}

HRESULT FilterUser::setShouldStop() {
  bShouldStop = true;
  wsc.setShouldStop();
  return S_OK;
}

HRESULT FilterUser::setPrivilege(HANDLE hToken, LPCWSTR pwcPrivilege,
                                 BOOL bIsPrivilegeEnabled) {
  TOKEN_PRIVILEGES tp;
  LUID luid;
  HRESULT hr = S_OK;

  if (!LookupPrivilegeValue(NULL, pwcPrivilege, &luid)) {
    hr = HRESULT_FROM_WIN32(GetLastError());
    fwprintf(stderr, L"LoogupPrivilegeValue error 0x%08x\n", hr);
    return hr;
  };

  tp.PrivilegeCount = 1;
  tp.Privileges[0].Luid = luid;
  if (bIsPrivilegeEnabled) {
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
  } else {
    tp.Privileges[0].Attributes = 0;
  }

  if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES),
                             (PTOKEN_PRIVILEGES)NULL, (PDWORD)NULL)) {
    hr = HRESULT_FROM_WIN32(GetLastError());
    fwprintf(stderr, L"AdjustTokenPrivileges error 0x%08x\n", hr);
    return hr;
  };
  if (GetLastError() == ERROR_NOT_ALL_ASSIGNED) {
    std::wcout << L"The token does not have the specified privilege. \n";
    return HRESULT_FROM_WIN32(ERROR_NOT_ALL_ASSIGNED);
  }

  return S_OK;
}

HRESULT FilterUser::composeEventLog(PMFLT_EVENT_RECORD pEventRecord,
                                    std::wstring* pwsMsg) {
  HRESULT hr;
  FILETIME ftEventTime;
  SYSTEMTIME stEventTime;
  JSONObj jsObj;
  WCHAR pwcComputerName[MAX_PATH];
  ULONG uiComputerNameLength = MAX_PATH;

  pwsMsg->clear();

  jsObj.addSingleObj(L"eventId", std::format(L"{}", ++uiEventId));

  ZeroMemory(pwcComputerName, sizeof(pwcComputerName));
  hr = GetComputerName(pwcComputerName, &uiComputerNameLength);
  if (FAILED(hr)) {
    fwprintf(stderr, L"GetComputerName failed 0x%08x", hr);
    // return hr;
  }
  jsObj.addSingleObj(L"computerName", pwcComputerName);

  ftEventTime = {pEventRecord->uliSysTime.LowPart,
                 pEventRecord->uliSysTime.HighPart};
  FileTimeToSystemTime(&ftEventTime, &stEventTime);
  jsObj.addSingleObj(
      L"eventCalendarTime",
      std::format(L"{:02d}/{:02d}/{:04d} {:02d}:{:02d}:{:02d} UTC+7",
                  stEventTime.wDay, stEventTime.wMonth, stEventTime.wYear,
                  stEventTime.wHour, stEventTime.wMinute, stEventTime.wSecond));

  switch (pEventRecord->eventType) {
    case MFLT_CREATE:
    case MFLT_CLOSE:
    case MFLT_DELETE:
    case MFLT_OPEN:
    case MFLT_WRITE:
      break;
    case MFLT_PROCESS_CREATE:
    case MFLT_PROCESS_TERMINATE:
      logCreateProcessEvent(pEventRecord, &jsObj);

      break;
    default:
      break;
  }

  *pwsMsg = jsObj.toString();
  return S_OK;
}

HRESULT FilterUser::logCreateProcessEvent(PMFLT_EVENT_RECORD pEventRecord,
                                          JSONObj* pJsObj) {
  HRESULT hr = S_OK;

  WCHAR pwcImageFileAbsolutePath[MAX_PATH];
  ULONG uiImageFileAbsolutePathBufferSize;
  HANDLE hImageFileHandle;
  WIN32_FILE_ATTRIBUTE_DATA imageFileAttributeData;
  SYSTEMTIME stImageFileCreateTime;
  LONGLONG llImageFileSize = 0;
  ULONG uiFileVersionInfoBlockSize = 0;
  LPVOID lpFileVersionInfoBlock;
  PLANGUAGE_CODE_PAGE_STRUCT lpLangCodePage;
  ULONG uiLangCodePageSize;

  if (pEventRecord->eventType == MFLT_PROCESS_CREATE) {
    pJsObj->addSingleObj(L"eventType", L"CreateProcess");
    pJsObj->addSingleObj(
        L"pid", std::format(L"{}", pEventRecord->objInfo.procInfo.uiPID));
    pJsObj->addSingleObj(
        L"parentPid",
        std::format(L"{}", pEventRecord->objInfo.procInfo.uiParentPID));
    pJsObj->addSingleObj(
        L"imageFileDir",
        std::wstring(pEventRecord->objInfo.procInfo.pwcImageName)
            .substr(0, pEventRecord->objInfo.procInfo.uiImageNameLength));
    pJsObj->addSingleObj(
        L"commandLine",
        std::wstring(pEventRecord->objInfo.procInfo.pwcCommandLine)
            .substr(0, pEventRecord->objInfo.procInfo.uiCommandLineLength));

    hImageFileHandle = CreateFile(pEventRecord->objInfo.procInfo.pwcImageName,
                                  GENERIC_READ, FILE_SHARE_READ, NULL,
                                  OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hImageFileHandle == INVALID_HANDLE_VALUE) {
      hr = HRESULT_FROM_WIN32(GetLastError());
      fwprintf(stderr, L"Cannot open file %ws, error 0x%08x\n",
               pEventRecord->objInfo.procInfo.pwcImageName, hr);
    }
    uiImageFileAbsolutePathBufferSize = GetFinalPathNameByHandle(
        hImageFileHandle, pwcImageFileAbsolutePath, MAX_PATH, VOLUME_NAME_DOS);
    // fwprintf(stderr, L"%ws\n", pwcImageFileAbsolutePath);
    CloseHandle(hImageFileHandle);

    /// Get image file attributes
    if (GetFileAttributesEx(pwcImageFileAbsolutePath, GetFileExInfoStandard,
                            &imageFileAttributeData)) {
      pJsObj->addSingleObj(
          L"imageFileAttributeMask",
          std::format(L"{}", imageFileAttributeData.dwFileAttributes));

      FileTimeToSystemTime(&imageFileAttributeData.ftCreationTime,
                           &stImageFileCreateTime);
      pJsObj->addSingleObj(
          L"imageFileCreationTime",
          std::format(L"{:02d}/{:02d}/{:04d} {:02d}:{:02d}:{:02d} UTC+7",
                      stImageFileCreateTime.wDay, stImageFileCreateTime.wMonth,
                      stImageFileCreateTime.wYear, stImageFileCreateTime.wHour,
                      stImageFileCreateTime.wMinute,
                      stImageFileCreateTime.wSecond));

      // llImageFileSize = (LONGLONG)imageFileAttributeData.nFileSizeHigh
      //                   << 32 + imageFileAttributeData.nFileSizeLow;
      // pJsObj->addSingleObj(L"imageFileSize",
      //                    std::format(L"{}", llImageFileSize));
    } else {
      hr = HRESULT_FROM_WIN32(GetLastError());
      fwprintf(stderr, L"GetFileAttributesEx failed 0x%08x\n", hr);
    }

    /// Get file version info
    uiFileVersionInfoBlockSize =
        GetFileVersionInfoSize(pwcImageFileAbsolutePath, NULL);
    if (uiFileVersionInfoBlockSize != 0) {
      // fwprintf(stderr, L"Retrieve successful for %ws\n",
      //         pEventRecord->objInfo.procInfo.pwcImageName);
      lpFileVersionInfoBlock = malloc(uiFileVersionInfoBlockSize);
      if (GetFileVersionInfo(pwcImageFileAbsolutePath, 0,
                             uiFileVersionInfoBlockSize,
                             lpFileVersionInfoBlock)) {
        VerQueryValue(lpFileVersionInfoBlock, L"\\VarFileInfo\\Translation",
                      (LPVOID*)&lpLangCodePage, (PUINT)&uiLangCodePageSize);

        pJsObj->addSingleObj(
            L"imageFileCompanyName",
            getFileVersionInfoEntry(lpFileVersionInfoBlock, lpLangCodePage,
                                    uiLangCodePageSize, L"CompanyName"));
        pJsObj->addSingleObj(
            L"imageFileVersion",
            getFileVersionInfoEntry(lpFileVersionInfoBlock, lpLangCodePage,
                                    uiLangCodePageSize, L"FileVersion"));
        pJsObj->addSingleObj(
            L"imageFileDescription",
            getFileVersionInfoEntry(lpFileVersionInfoBlock, lpLangCodePage,
                                    uiLangCodePageSize, L"FileDescription"));
        pJsObj->addSingleObj(
            L"imageFileOriginalName",
            getFileVersionInfoEntry(lpFileVersionInfoBlock, lpLangCodePage,
                                    uiLangCodePageSize, L"OriginalFilename"));
        // pJsObj->addSingleObj(
        //     L"imageFileProductName",
        //     getFileVersionInfoEntry(lpFileVersionInfoBlock, lpLangCodePage,
        //                             uiLangCodePageSize, L"ProductName"));
      } else {
        hr = HRESULT_FROM_WIN32(GetLastError());
        fwprintf(stderr, L"GetFileVersionInfo failed 0x%08x\n", hr);
      }
      free(lpFileVersionInfoBlock);
    } else {
      hr = HRESULT_FROM_WIN32(GetLastError());
      if (hr == HRESULT_FROM_WIN32(0x715)) {
        pJsObj->addSingleObj(L"imageFileCompanyName", L"Unknown");
        pJsObj->addSingleObj(L"imageFileVersion", L"Unknown");
        pJsObj->addSingleObj(L"imageFileDescription", L"Unknown");
        pJsObj->addSingleObj(L"imageFileOriginalName", L"Unknown");
      } else {
        fwprintf(stderr, L"GetFileVersionInfoSize failed 0x%08x at file %ws\n",
                 hr, pEventRecord->objInfo.procInfo.pwcImageName);
      }
    }

    /// Get file hashes
    pJsObj->addSingleObj(L"imageFilehashMD5",
                         getFileHash(pwcImageFileAbsolutePath, CALG_MD5));
    pJsObj->addSingleObj(L"imageFilehashSHA1",
                         getFileHash(pwcImageFileAbsolutePath, CALG_SHA1));
    pJsObj->addSingleObj(L"imageFilehashSHA256",
                         getFileHash(pwcImageFileAbsolutePath, CALG_SHA_256));
  }
  if (pEventRecord->eventType == MFLT_PROCESS_TERMINATE) {
    pJsObj->addSingleObj(L"eventType", L"TerminateProcess");
    pJsObj->addSingleObj(
        L"pid", std::format(L"{}", pEventRecord->objInfo.procInfo.uiPID));
    pJsObj->addSingleObj(
        L"exitcode",
        std::format(L"{}", pEventRecord->objInfo.procInfo.iExitcode));
  }
  return hr;
}

std::wstring FilterUser::getFileVersionInfoEntry(
    PVOID lpFileVersionInfoBlock, PLANGUAGE_CODE_PAGE_STRUCT lpLangCodePage,
    ULONG uiLangCodePageSize, std::wstring wsQueryEntry) {
  LPWSTR pwcFileVersionInfoEntryResult;
  ULONG uiFileVersionInfoEntryResultSize;
  for (int i = 0; i < uiLangCodePageSize / sizeof(LANGUAGE_CODE_PAGE_STRUCT);
       i++) {
    if (VerQueryValue(lpFileVersionInfoBlock,
                      std::format(L"\\StringFileInfo\\{:04x}{:04x}\\{}",
                                  lpLangCodePage[i].wLanguage,
                                  lpLangCodePage[i].wCodePage, wsQueryEntry)
                          .c_str(),
                      (LPVOID*)&pwcFileVersionInfoEntryResult,
                      (PUINT)&uiFileVersionInfoEntryResultSize)) {
      if (uiFileVersionInfoEntryResultSize != 0) {
        return pwcFileVersionInfoEntryResult;
      }
    }
  }
  return L"Unknown";
}

std::wstring FilterUser::getFileHash(std::wstring wsFileName, ALG_ID hashAlg) {
  HRESULT hr = S_OK;

  HANDLE hFileHandle = NULL;
  HCRYPTPROV hProv = 0;
  HCRYPTHASH hHash = 0;
  BOOL bResult = FALSE;
  BYTE pucFileContentBuffer[MAX_BUFFER_SIZE];
  LPBYTE pucHash;
  ULONG uiHashLength = 0;
  ULONG uiCryptoProvider = 0;
  ULONG uiBytesRead = 0;
  ULONG uiBytesHashed = 0;
  std::wstring wsHash;

  switch (hashAlg) {
    case CALG_MD5:
      uiHashLength = MD5_HASH_LENGTH;
      uiCryptoProvider = PROV_RSA_FULL;
      break;
    case CALG_SHA1:
      uiHashLength = SHA1_HASH_LENGTH;
      uiCryptoProvider = PROV_RSA_FULL;
      break;
    case CALG_SHA_256:
      uiHashLength = SHA256_HASH_LENGTH;
      uiCryptoProvider = PROV_RSA_AES;
      break;
    default:
      break;
  }
  pucHash = new BYTE[uiHashLength];

  if (!CryptAcquireContext(&hProv, NULL, NULL, uiCryptoProvider,
                           CRYPT_VERIFYCONTEXT)) {
    hr = HRESULT_FROM_WIN32(GetLastError());
    fwprintf(stderr, L"CryptAcquireContext failed 0x%08x\n", hr);
    return L"";
  }

  if (!CryptCreateHash(hProv, hashAlg, 0, 0, &hHash)) {
    hr = HRESULT_FROM_WIN32(GetLastError());
    fwprintf(stderr, L"CryptCreateHash failed 0x%08x\n", hr);
    CryptReleaseContext(hProv, 0);
    return L"";
  }

  hFileHandle =
      CreateFile(wsFileName.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL,
                 OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
  if (hFileHandle == INVALID_HANDLE_VALUE) {
    hr = HRESULT_FROM_WIN32(GetLastError());
    fwprintf(stderr, L"Cannot open file %ws, error 0x%08x\n",
             wsFileName.c_str(), hr);
  }

  while (bResult = ReadFile(hFileHandle, pucFileContentBuffer, MAX_BUFFER_SIZE,
                            &uiBytesRead, NULL)) {
    if (!uiBytesRead) {
      break;
    }

    if (!CryptHashData(hHash, pucFileContentBuffer, uiBytesRead, 0)) {
      hr = HRESULT_FROM_WIN32(GetLastError());
      fwprintf(stderr, L"CryptHashData failed 0x%08x at file %ws\n", hr,
               wsFileName.c_str());
      CryptReleaseContext(hProv, 0);
      CryptDestroyHash(hHash);
      return L"";
    }
  }

  if (!bResult) {
    hr = HRESULT_FROM_WIN32(GetLastError());
    fwprintf(stderr, L"ReadFile failed 0x%08x at file %ws\n", hr,
             wsFileName.c_str());
    CryptReleaseContext(hProv, 0);
    CryptDestroyHash(hHash);
    return L"";
  }

  uiBytesHashed = uiHashLength;
  if (!CryptGetHashParam(hHash, HP_HASHVAL, pucHash, &uiBytesHashed, 0)) {
    hr = HRESULT_FROM_WIN32(GetLastError());
    fwprintf(stderr, L"CryptGetHashParam failed 0x%08x at file %ws\n", hr,
             wsFileName.c_str());
    CryptReleaseContext(hProv, 0);
    CryptDestroyHash(hHash);
    return L"";
  }

  for (ULONG i = 0; i < uiBytesHashed; ++i) {
    wsHash += std::format(L"{:x}{:x}", pucHash[i] >> 4, pucHash[i] & 0xf);
  }
  CryptReleaseContext(hProv, 0);
  CryptDestroyHash(hHash);
  return wsHash;
}