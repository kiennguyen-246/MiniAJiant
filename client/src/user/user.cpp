#include <cassert>
#include <fstream>
#include <future>
#include <iostream>

#include "FilterUser.hpp"

const WCHAR FILTER_NAME[] = L"MiniAJiant";
const WCHAR FILTER_COM_PORT_NAME[] = L"\\MAJPort";
const WCHAR ERROR_LOG_FILE[] = L".\\logs\\error.log";
const WCHAR INSTRUCTIONS[] =
    L"Type:\n"
    L"\t- 'install' to install the filter,\n"
    L"\t- 'help' to consult instructions,\n "
    L"\t- 'start' to start filtering,\n "
    L"\t- 'stop' to stop filtering\n"
    L"\t- 'quit' to exit\n";

int wmain(int argc, LPWSTR argv[]) {
  HRESULT hr = S_OK;

  //FILE* fErrLog;
  //_wfreopen_s(&fErrLog, ERROR_LOG_FILE, L"w", stderr);

  FilterUser fuMfltUser(FILTER_NAME, FILTER_COM_PORT_NAME);
  std::future<HRESULT> fMainRoutine;
  bool bHasStarted = false;

  if (argc < 3) {
    wprintf(L"Invalid arguments\n");
    return 1;
  }

  system("cls");

  wprintf(L"MINIAJIANT\n-----------------------------\n\n");
  wprintf(INSTRUCTIONS);

  while (1) {
    wprintf(L"MiniAJiant>> ");
    std::wstring wsCommand;
    std::wcin >> wsCommand;
    if (wsCommand == L"install") {
      wprintf(L"Command currently disabled\n");
    } else if (wsCommand == L"start") {
      hr = fuMfltUser.loadFilter();
      if (hr) {
        wprintf(L"Start failed 0x%08x\n", hr);
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
    } else if (wsCommand == L"stop") {
      if (!bHasStarted) {
        wprintf(L"The filter has not started\n");
        continue;
      }
      fuMfltUser.setShouldStop();
      hr = fMainRoutine.get();
      if (FAILED(hr)) {
        wprintf(L"An error occured while filtering. Error 0x%08x\n", hr);
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
    } else if (wsCommand == L"quit") {
      if (bHasStarted) {
        wprintf(L"Please stop the filter before exiting\n");
        continue;
      }
      break;
    } else if (wsCommand == L"help") {
      wprintf(INSTRUCTIONS);
    } else {
      wprintf(L"Invalid command. Type 'help' to consult instructions.\n");
    }
  }

  //fclose(fErrLog);

  return 0;
}