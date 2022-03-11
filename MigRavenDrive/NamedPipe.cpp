#include "pch.h"
#include "NamedPipe.h"

wstring CNamedPipe::SendRequestToServer(wstring request)
{
  wstring serverAddress = L"\\\\.\\pipe\\migRavenShellPipe";
  ATLTRACE(L"CPipeClient::SendRequestToServer  server:%s, request:%s\n", serverAddress, request);

  auto hPipe = OpenNamedPipe(serverAddress);
  if (hPipe == INVALID_HANDLE_VALUE) return L"";

  if (!SendRequest(&hPipe, request))
  {
    CloseNamedPipe(&hPipe);
    return L"";
  }

  const auto response = ReceiveResponse(&hPipe);
  if (response.empty())
  {
    CloseNamedPipe(&hPipe);
    return L"";
  }

  CloseNamedPipe(&hPipe);
  return response;
}

HANDLE CNamedPipe::OpenNamedPipe(wstring serverAddress)
{
  ATLTRACE(L"CPipeClient::OpenNamedPipe  server:%s\n", serverAddress);

  HANDLE hPipe;
  while (true)
  {
    hPipe = CreateFile(
      serverAddress.c_str(),
      GENERIC_READ | GENERIC_WRITE,
      0,
      nullptr,
      OPEN_EXISTING,
      0,
      nullptr);

    if (hPipe != INVALID_HANDLE_VALUE) break;
    if (GetLastError() != ERROR_PIPE_BUSY) break;
    if (!WaitNamedPipe(serverAddress.c_str(), 10 * 1000)) break;
  }

  DWORD dwMode = PIPE_READMODE_BYTE;
  auto hr = SetNamedPipeHandleState(
    hPipe,
    &dwMode,
    nullptr,
    nullptr);

  return hPipe;
}

BOOL CNamedPipe::CloseNamedPipe(LPHANDLE hPipe)
{
  ATLTRACE(L"CPipeClient::CloseNamedPipe\n");
  return CloseHandle(*hPipe);
}

BOOL CNamedPipe::SendRequest(LPHANDLE hPipe, wstring requestString)
{
  ATLTRACE(L"CPipeClient::SendRequest  request:%s\n", requestString);

  DWORD cbWritten{ 0 };
  const DWORD cbToWrite = wcslen(requestString.c_str()) * sizeof(WCHAR);

  BYTE data = cbToWrite >> 24;
  auto result = WriteFile(
    *hPipe,
    &data,
    sizeof BYTE,
    &cbWritten,
    nullptr);
  if (!result) return result;

  data = cbToWrite >> 16 & 255;
  result = WriteFile(
    *hPipe,
    &data,
    sizeof BYTE,
    &cbWritten,
    nullptr);
  if (!result) return result;

  data = cbToWrite >> 8 & 255;
  result = WriteFile(
    *hPipe,
    &data,
    sizeof BYTE,
    &cbWritten,
    nullptr);
  if (!result) return result;

  data = cbToWrite & 255;
  result = WriteFile(
    *hPipe,
    &data,
    sizeof BYTE,
    &cbWritten,
    nullptr);
  if (!result) return result;

  result = WriteFile(
    *hPipe,
    requestString.c_str(),
    cbToWrite,
    &cbWritten,
    nullptr);
  return result;
}

wstring CNamedPipe::ReceiveResponse(LPHANDLE hPipe)
{
  ATLTRACE(L"CPipeClient::ReceiveResponse\n");

  BOOL bResult;
  DWORD cbRead{ 0 };
  BYTE dataRead{ 0 };

  do
  {
    bResult = ReadFile(
      *hPipe,
      &dataRead,
      sizeof BYTE,
      &cbRead,
      nullptr);
    if (!bResult) break;
    INT len = dataRead << 24;

    bResult = ReadFile(
      *hPipe,
      &dataRead,
      sizeof BYTE,
      &cbRead,
      nullptr);
    if (!bResult) break;
    len += dataRead << 16;

    bResult = ReadFile(
      *hPipe,
      &dataRead,
      sizeof BYTE,
      &cbRead,
      nullptr);
    if (!bResult) break;
    len += dataRead << 8;

    bResult = ReadFile(
      *hPipe,
      &dataRead,
      sizeof BYTE,
      &cbRead,
      nullptr);
    if (!bResult) break;
    len += dataRead + 2;

    const DWORD dwArraySize = len / sizeof WCHAR;
    auto* chBuf = new WCHAR[dwArraySize]{ '\0' };
    bResult = ReadFile(
      *hPipe,
      chBuf,
      len,
      &cbRead,
      nullptr);

    if (!bResult && GetLastError() != ERROR_MORE_DATA)
      break;

    return chBuf;
  } while (!bResult);
  return nullptr;
}
