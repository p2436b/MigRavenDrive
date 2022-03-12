#pragma once

#include "pch.h"
#include <codecvt>
#include <string>
#include <fstream>

using namespace std;

wstring GetMigravenLocal()
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
wstring utf8_to_wstring(const string& str)
{
  wstring_convert<codecvt_utf8<wchar_t>> myconv;
  return myconv.from_bytes(str);
}

// convert wstring to UTF-8 string
string wstring_to_utf8(const wstring& str)
{
  wstring_convert<codecvt_utf8<wchar_t>> myconv;
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

wstring ReadFile(const wstring& filename)
{
  wifstream wif(filename);
  wstringstream wss;
  wss << wif.rdbuf();
  return wss.str();
}

void SaveFile(const wstring& filename, const wstring& data)
{
  wofstream wof(filename);
  wstringstream wss;
  wof << data;
}

void ReplaceAll(wstring& str, const wstring& from, const wstring& to) {
  if (from.empty())
    return;
  size_t start_pos = 0;
  while ((start_pos = str.find(from, start_pos)) != wstring::npos) {
    str.replace(start_pos, from.length(), to);
    start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
  }
}