#pragma once

#include <string>

using namespace std;

class CNamedPipe
{
public:
  static wstring SendRequestToServer(wstring request);

private:
  static HANDLE OpenNamedPipe(wstring serverAddress);

  static BOOL CloseNamedPipe(LPHANDLE hPipe);

  static BOOL SendRequest(LPHANDLE hPipe, wstring requestString);

  static std::wstring ReceiveResponse(LPHANDLE hPipe);
};

