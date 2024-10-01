#include "utils/JSONObj.hpp"

JSONObj::JSONObj() { umSingleObjs.clear(); }

JSONObj::~JSONObj() {}

HRESULT JSONObj::addSingleObj(std::wstring wsKey, std::wstring wsValue) {
  if (umSingleObjs.find(wsKey) != umSingleObjs.end()) {
    return E_FAIL;
  }
  umSingleObjs.insert({wsKey, wsValue});
  return S_OK;
}

std::wstring JSONObj::toString() {
  std::wstring wsRes = L"{ ";
  for (auto p : umSingleObjs) {
    std::wstring wsKey = p.first;
    std::wstring wsValue = L"";

    /// Backslashes and double quotation marks have weird JSON representation
    for (auto& ch : p.second) {
      if (ch == '\\') {
        wsValue += L"\\\\\\\\";
        continue;
      }
      if (ch == '"') {
        wsValue += L"\\\\\\\"";
        continue;
      }
      wsValue.push_back(ch);
    }
    wsRes += std::format(L"\"{}\": \"{}\", ", wsKey, wsValue);
  }
  if (wsRes.length() > 2) {
    wsRes.pop_back();
    wsRes.pop_back();
  }
  wsRes += L" }";
  return wsRes;
}