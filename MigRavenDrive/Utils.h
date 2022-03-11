#pragma once

#include "pch.h"
#include <codecvt>
#include <string>

std::wstring GetMigravenLocal()
{
  DWORD data{};
  DWORD dataSize = sizeof(data);
  auto retCode = RegGetValue(HKEY_CURRENT_USER, L"SOFTWARE\\migRavenDesktop", L"Language", RRF_RT_DWORD, nullptr, &data, &dataSize);

  switch (data)
  {
  case 0: return L"en-US";
  case 1: return L"de-DE";
  case 2: return L"fa-IR";
  default: return L"en-US";
  }
}

// convert UTF-8 string to wstring
std::wstring utf8_to_wstring(const std::string& str)
{
  std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
  return myconv.from_bytes(str);
}

// convert wstring to UTF-8 string
std::string wstring_to_utf8(const std::wstring& str)
{
  std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
  return myconv.to_bytes(str);
}

FILETIME TimestampToFiletime(time_t t)
{
  FILETIME ft;
  ULARGE_INTEGER time_value;
  time_value.QuadPart = (t * 10000000LL) + 116444736000000000LL;
  ft.dwLowDateTime = time_value.LowPart;
  ft.dwHighDateTime = time_value.HighPart;
  return ft;
}