#ifndef JSON_OBJ_HPP
#define JSON_OBJ_HPP

#include <Winerror.h>

#include <format>
#include <iostream>
#include <string>
#include <unordered_map>

class JSONObj {
 public:
  /**
   * Default constructor
   */
  JSONObj();

  /**
   * Default destructor
   */
  ~JSONObj();

  /**
   * Add a single key-value pair to the current JSON object. Both of them are
   * strings.
   *
   * @param wsKey A std::wstring object representing the key
   * @param wsValue A std::wstring object representing the value
   *
   * @return S_OK if the operation is successful, an error code/HRESULT if fail.
   */
  HRESULT addSingleObj(std::wstring wsKey, std::wstring wsValue);

  /**
   * Turn the whole JSON object to a std::wstring object,
   *
   * @return A std::wstring object represent the stringified JSON object
   */
  std::wstring toString();

 private:
  /// A hash table structure storing key-value pairs
  std::unordered_map<std::wstring, std::wstring> umSingleObjs;
};

#endif  // ! JSON_OBJ_HPP
