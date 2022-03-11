#include "pch.h"
#include "FileSystem.h"

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

  case VFS_INSTALL_SHELLNEW:
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
  return S_OK;
}

HRESULT CMigRavenModule::DllUninstall()
{
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