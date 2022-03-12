#pragma once

using namespace std;

wstring GetMigravenLocal();

// convert UTF-8 string to wstring
wstring utf8_to_wstring(const string& str);

// convert wstring to UTF-8 string
string wstring_to_utf8(const wstring& str);

FILETIME TimestampToFiletime(time_t t);

wstring ReadFile(const wstring& filename);

void SaveFile(const wstring& filename, const wstring& data);

void ReplaceAll(wstring& str, const wstring& from, const wstring& to);