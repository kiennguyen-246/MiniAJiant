#ifndef HELPERS_HPP
#define HELPERS_HPP

#include <Windows.h>

#include <iostream>
#include <format>
#include <fstream>
#include <chrono>
#include <ctime>

typedef enum _NOTIFICATION_TYPE {
  NOTIFICATION_INFO_TYPE,
  NOTIFICATION_WARNING_TYPE,
  NOTIFICATION_ERROR_TYPE,
  NOTIFICATION_DEBUG_TYPE,
} NOTIFICATION_TYPE,
    *PNOTIFICATION_TYPE;

const WCHAR NOTIFICATION_LOG_FILE[] = L".\\logs\\notification.log";

/**
* Convert a object of type std::wstring to an object of type std::string.
* 
* @param wstr The std::wstring object needs converting.
* 
* @return The result std::string object.
*/
std::string wstrToStr(std::wstring wstr);

/**
 * Log a notification to a designated log file.
 *
 * @param wsNotification The notification string.
 * @param ntNotificationType Type of the notification.
 */
void logNotification(std::wstring wsNotification, NOTIFICATION_TYPE ntNotificationType);

#endif  // !HELPERS_HPP