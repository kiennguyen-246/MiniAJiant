#include "utils/helpers.hpp"

std::string wstrToStr(std::wstring wstr) {
  char pcBuffer[1024];
  memset(pcBuffer, 0, sizeof(pcBuffer));
  size_t ullTmpLen = 0;
  wcstombs_s(&ullTmpLen, pcBuffer, 1024, wstr.c_str(), wstr.length());
  return std::string(pcBuffer);
}

void logNotification(
    std::wstring wsNotification,
    NOTIFICATION_TYPE ntNotificationType = NOTIFICATION_INFO_TYPE) {
  std::wofstream wfsNotificationLog(NOTIFICATION_LOG_FILE, std::ios_base::app);
  time_t tNow =
      std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  std::wstring wsNotificationType = L"";
  std::wstring wsNow;

  if (ntNotificationType == NOTIFICATION_DEBUG_TYPE) {
    return;
  }

  wsNow.resize(26);
  _wctime_s(&wsNow[0], 26, &tNow);
  while (wsNow.back() < L'0' || wsNow.back() > L'9') {
    wsNow.pop_back();
  }

  if (wfsNotificationLog.is_open()) {
    switch (ntNotificationType) {
      case NOTIFICATION_INFO_TYPE:
        wsNotificationType = L"INFO";
        break;
      case NOTIFICATION_WARNING_TYPE:
        wsNotificationType = L"WARNING";
        break;
      case NOTIFICATION_ERROR_TYPE:
        wsNotificationType = L"ERROR";
        break;
      case NOTIFICATION_DEBUG_TYPE:
        wsNotificationType = L"DEBUG";
        break;
      default:
        break;
    }

    wfsNotificationLog << std::format(L"[{}][{}] {}\n", wsNow,
                                      wsNotificationType, wsNotification);
    wfsNotificationLog.close();
  }
}