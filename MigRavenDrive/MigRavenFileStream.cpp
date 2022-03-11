#include "pch.h"
#include "FileSystem.h"

CMigRavenFileStream::CMigRavenFileStream(CMigRavenFileSystem* pFS, LPCWSTR pstrFilename, UINT uAccess)
{
}

CMigRavenFileStream::~CMigRavenFileStream()
{
}

HRESULT CMigRavenFileStream::Init()
{
  return E_NOTIMPL;
}

HRESULT CMigRavenFileStream::Read(LPVOID pData, ULONG dwSize, ULONG& dwRead)
{
  return E_NOTIMPL;
}

HRESULT CMigRavenFileStream::Write(LPCVOID pData, ULONG dwSize, ULONG& dwWritten)
{
  return E_NOTIMPL;
}

HRESULT CMigRavenFileStream::Seek(DWORD dwPos)
{
  return E_NOTIMPL;
}

HRESULT CMigRavenFileStream::GetCurPos(DWORD* pdwPos)
{
  return E_NOTIMPL;
}

HRESULT CMigRavenFileStream::GetFileSize(DWORD* pdwFileSize)
{
  return E_NOTIMPL;
}

HRESULT CMigRavenFileStream::SetFileSize(DWORD dwSize)
{
  return E_NOTIMPL;
}

HRESULT CMigRavenFileStream::Commit()
{
  return E_NOTIMPL;
}

HRESULT CMigRavenFileStream::Close()
{
  return E_NOTIMPL;
}
