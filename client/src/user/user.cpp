#include <cassert>
#include <fstream>
#include <future>
#include <iostream>

#include "FilterUser.hpp"
#include "utils/helpers.hpp"

const WCHAR FILTER_NAME[] = L"MiniAJiant";
const WCHAR FILTER_COM_PORT_NAME[] = L"\\MAJPort";
const WCHAR ERROR_LOG_FILE[] = L".\\logs\\error.log";
const WCHAR BANNER[] =
    L"/////////////////////////////////////////////////////////////////////////"
    L"////////////////////////////////\n"
    L"///_|      _|  _|_|_|  _|      _|  _|_|_|    _|_|          _|  _|_|_|    "
    L"_|_|    _|      _|  _|_|_|_|_|  \n"
    L"///_|_|  _|_|    _|    _|_|    _|    _|    _|    _|        _|    _|    "
    L"_|    _|  _|_|    _|      _|      \n"
    L"///_|  _|  _|    _|    _|  _|  _|    _|    _|_|_|_|        _|    _|    "
    L"_|_|_|_|  _|  _|  _|      _|      \n"
    L"///_|      _|    _|    _|    _|_|    _|    _|    _|  _|    _|    _|    "
    L"_|    _|  _|    _|_|      _|      \n"
    L"///_|      _|  _|_|_|  _|      _|  _|_|_|  _|    _|    _|_|    _|_|_|  "
    L"_|    _|  _|      _|      _|      \n"
    L"/////////////////////////////////////////////////////////////////////////"
    L"////////////////////////////////\n";
const WCHAR INSTRUCTIONS[] =
    L"Type:\n"
    L"\t- 'install' to install the filter,\n"
    L"\t- 'help' to consult instructions,\n "
    L"\t- 'start' to start filtering,\n "
    L"\t- 'stop' to stop filtering\n"
    L"\t- 'quit' to exit\n";

int wmain(int argc, LPWSTR argv[]) {
  HRESULT hr = S_OK;
  SECURITY_ATTRIBUTES saLogFolder;
  FilterUser fuMfltUser(FILTER_NAME, FILTER_COM_PORT_NAME);
  std::future<HRESULT> fMainRoutine;
  bool bHasStarted = false;
  bool bIsInstalledSuccessfully = false;
  int iResult = 0;

  saLogFolder = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};
  CreateDirectory(L".\\Logs", &saLogFolder);


  if (argc < 3) {
    wprintf(L"Usage: user [server host] [server port]\n");
    return 1;
  }

  system("cls");

  wprintf(L"%ws\n\n\n", BANNER);
  wprintf(INSTRUCTIONS);

  while (1) {
    wprintf(L"MiniAJiant>> ");
    std::wstring wsCommand;
    std::wcin >> wsCommand;
    if (wsCommand == L"install") {
      iResult = system(
          "rundll32 syssetup,SetupInfObjectInstallAction DefaultInstall 128 "
          ".\\Drivers\\MiniAJiant.inf");
      if (iResult) {
        wprintf(L"Installation failed 0x%08x\n", iResult);
        logNotification(
            std::format(L"Start failed 0x{:08x}", (unsigned)iResult),
            NOTIFICATION_ERROR_TYPE);
        continue;
      }
      wprintf(L"Successful\n");
      logNotification(std::format(L"Installed successful"),
                      NOTIFICATION_INFO_TYPE);
      bIsInstalledSuccessfully = true;
    } else if (wsCommand == L"start") {
      if (!bIsInstalledSuccessfully) {
        wprintf(L"The filter driver has not been installed.\n");
      }

      hr = fuMfltUser.loadFilter();
      if (FAILED(hr)) {
        wprintf(L"Start failed 0x%08x\n", hr);
        logNotification(std::format(L"Start failed 0x{:08x}", (unsigned)hr),
                        NOTIFICATION_ERROR_TYPE);
        continue;
      }

      hr = fuMfltUser.connectToServer(argv[1], argv[2]);

      // hr = fuMfltUser.attachFilterToVolume(L"E:");
      if (hr) {
        continue;
      }

      fMainRoutine = std::async(std::launch::async, &FilterUser::doMainRoutine,
                                &fuMfltUser);

      bHasStarted = true;
      wprintf(L"Successful\n");
      logNotification(std::format(L"Started successful"),
                      NOTIFICATION_INFO_TYPE);
    } else if (wsCommand == L"stop") {
      if (!bHasStarted) {
        wprintf(L"The filtering process has not started.\n");
        continue;
      }
      fuMfltUser.setShouldStop();
      hr = fMainRoutine.get();
      if (FAILED(hr)) {
        wprintf(L"An error occured while filtering. Error 0x%08x\n", hr);
        logNotification(
            std::format(L"An error occured while filtering 0x{:08x}",
                        (unsigned)hr),
            NOTIFICATION_ERROR_TYPE);
      }

      hr = fuMfltUser.unloadFilter();
      if (hr) {
        continue;
      }

      hr = fuMfltUser.disconnectFromServer();
      if (hr) {
        continue;
      }

      bHasStarted = false;

      wprintf(L"Successful\n");
      logNotification(std::format(L"Stop successful"), NOTIFICATION_INFO_TYPE);
    } else if (wsCommand == L"quit") {
      if (bHasStarted) {
        wprintf(L"Please stop the filtering process before exiting.\n");
        continue;
      }
      break;
    } else if (wsCommand == L"help") {
      wprintf(INSTRUCTIONS);
    } else {
      wprintf(L"Invalid command. Type 'help' to consult instructions.\n");
    }
  }

  // fclose(fErrLog);

  return 0;
}