#include "pch.h"
#include "FileSystem.h"
#include <PathCch.h>
#include <Utils.h>

LONG CMigRavenModule::GetConfigInt(VFS_CONFIG Item)
{
  switch (Item) {

  case VFS_INT_LOCATION:
    return VFS_LOCATION_MYCOMPUTER;

  case VFS_INT_MAX_FILENAME_LENGTH:
    return MAX_PATH;

  case VFS_INT_MAX_PATHNAME_LENGTH:
    return MAX_PATH;

  case VFS_INT_SHELLROOT_SFGAO:
    return SFGAO_STREAM
      | SFGAO_FOLDER
      | SFGAO_BROWSABLE
      | SFGAO_HASSUBFOLDER
      | SFGAO_FILESYSANCESTOR
      | SFGAO_STORAGEANCESTOR;
  }
  return 0;
}

BOOL CMigRavenModule::GetConfigBool(VFS_CONFIG Item)
{
  switch (Item) {

  case VFS_INSTALL_DROPTARGET:
  case VFS_INSTALL_CONTEXTMENU:
    return TRUE;

  case VFS_CAN_SLOW_COPY:
  case VFS_CAN_SLOW_ENUM:
  case VFS_CAN_PROGRESSUI:
  case VFS_CAN_ATTACHMENTSERVICES:
    return TRUE;

  case VFS_SHOW_RIBBON_PANEL:
    return TRUE;

  case VFS_HAVE_NAMEOF:
  case VFS_HAVE_SYSICONS:
  case VFS_HAVE_UNIQUE_NAMES:
  case VFS_HAVE_VIRTUAL_FILES:
    return TRUE;
  }
  return FALSE;
}

LPCWSTR CMigRavenModule::GetConfigStr(VFS_CONFIG Item)
{
  switch (Item) {
  case VFS_STR_FILENAME_CHARS_NOTALLOWED:
    return L":<>\\/|\"'*?[]";
  }
  return nullptr;
}

HRESULT CMigRavenModule::CreateFileSystem(PCIDLIST_ABSOLUTE pidlRoot, CNseFileSystem** ppFS)
{
  *ppFS = new CMigRavenFileSystem();
  return *ppFS != nullptr ? S_OK : E_OUTOFMEMORY;
}

HRESULT CMigRavenModule::DllInstall()
{
  if (FAILED(RegisterPropdesc(TRUE)))
    MessageBox(nullptr, L"Failed to register PropertySchema.", L"Error", MB_ICONERROR);
  return S_OK;
}

HRESULT CMigRavenModule::DllUninstall()
{
  if (FAILED(RegisterPropdesc(FALSE)))
    MessageBox(nullptr, L"Failed to unregister PropertySchema.", L"Error", MB_ICONERROR);
  return S_OK;
}

HRESULT CMigRavenModule::ShellAction(LPCWSTR pstrType, LPCWSTR pstrCmdLine)
{
  return S_OK;
}

BOOL CMigRavenModule::DllMain(DWORD dwReason, LPVOID lpReserved)
{
  return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
// Private functions

HRESULT CMigRavenModule::RegisterPropdesc(BOOL regPropdesc)
{
  auto moduleDir = new WCHAR[MAX_PATH];
  GetModuleFileName(_pModule->GetModuleInstance(), moduleDir, MAX_PATH);
  PathCchRemoveFileSpec(moduleDir, MAX_PATH);

  wstring modulePath = moduleDir + L"\\MigRavenDrive.dll"s;
  wstring propDescPath = moduleDir + L"\\MigRavenDrive.propdesc"s;

  auto hr = S_OK;
  if (regPropdesc) {
    wstring data = ReadFile(propDescPath);
    ReplaceAll(data, L"DllFilePath", modulePath);
    SaveFile(propDescPath, data);

    hr = PSRegisterPropertySchema(propDescPath.c_str());
    if (FAILED(hr)) return hr;
  }
  else
  {
    hr = PSUnregisterPropertySchema(propDescPath.c_str());
    if (FAILED(hr)) return hr;
  }

  hr = PSRefreshPropertySchema();
  if (FAILED(hr)) return hr;

  SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);
  return S_OK;
}


CMigRavenFileSystem::CMigRavenFileSystem() :m_cRef(1)
{
}

VOID CMigRavenFileSystem::AddRef()
{
  InterlockedIncrement(&m_cRef);
}

VOID CMigRavenFileSystem::Release()
{
  if (InterlockedDecrement(&m_cRef) == 0)
    delete this;
}

CNseItem* CMigRavenFileSystem::GenerateRoot(CShellFolder* pShellFolder)
{
  return new CMigRavenFileItem(pShellFolder, nullptr, nullptr, FALSE);
}