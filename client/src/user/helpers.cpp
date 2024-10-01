#include "utils/helpers.hpp"

std::string wstrToStr(std::wstring wstr) {
  char pcBuffer[1024];
  memset(pcBuffer, 0, sizeof(pcBuffer));
  size_t ullTmpLen = 0;
  wcstombs_s(&ullTmpLen, pcBuffer, 1024, wstr.c_str(), wstr.length());
  return std::string(pcBuffer);
}