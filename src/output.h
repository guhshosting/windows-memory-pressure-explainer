#ifndef WMPE_OUTPUT_H
#define WMPE_OUTPUT_H
#include <algorithm>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <windows.h>
inline bool write_output(const std::string &text) {
  const HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
  DWORD mode = 0;
  if (!GetConsoleMode(output, &mode)) {
    std::cout << text;
    std::cout.flush();
    return static_cast<bool>(std::cout);
  }
  if (text.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()))
    throw std::runtime_error("output exceeds conversion limit");
  const int length =
      MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(),
                          static_cast<int>(text.size()), nullptr, 0);
  if (length <= 0)
    return false;
  std::wstring wide(static_cast<std::size_t>(length), L'\0');
  if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(),
                          static_cast<int>(text.size()), wide.data(),
                          length) != length)
    return false;
  std::size_t offset = 0;
  while (offset < wide.size()) {
    const auto chunk =
        static_cast<DWORD>(std::min<std::size_t>(wide.size() - offset, 16000));
    DWORD written = 0;
    if (!WriteConsoleW(output, wide.data() + offset, chunk, &written,
                       nullptr) ||
        written == 0)
      return false;
    offset += written;
  }
  return true;
}

#endif
