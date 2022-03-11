#include "pch.h"
#include "FileSystem.h"
#include "ShellFolder.h"
#include <string>
#include <Utils.h>
#include <NamedPipe.h>

CMigRavenFileItem::CMigRavenFileItem(CShellFolder* pFolder, PCIDLIST_RELATIVE pidlFolder, PCITEMID_CHILD pidlItem, BOOL bReleaseItem) :
  CNseFileItem(pFolder, pidlFolder, pidlItem, bReleaseItem)
{
  if (pidlItem == NULL)
  {
    // Create empty root item
    static MIG_FSS_DATA s_mfdEmpty = { 0 };
    m_pMfd = &s_mfdEmpty;
    m_pMfd->dwAttributes = MR_ATTRIBUTE_SYSTEM;
  }
  else
  {
    // Extract item migRaven data
    m_pMfd = &(reinterpret_cast<NSEFILEPIDLDATA*>(const_cast<PUITEMID_CHILD>(pidlItem)))->mfd;

    ATLASSERT(pidlItem->mkid.cb >= sizeof(NSEFILEPIDLDATA));
  }
}

SFGAOF CMigRavenFileItem::GetSFGAOF(SFGAOF dwMask)
{
  return CNseFileItem::GetSFGAOF(dwMask);
}

HRESULT CMigRavenFileItem::GetProperty(REFPROPERTYKEY pkey, CComPropVariant& v)
{
  return CNseFileItem::GetProperty(pkey, v);
}

HRESULT CMigRavenFileItem::SetProperty(REFPROPERTYKEY pkey, const CComPropVariant& v)
{
  return CNseFileItem::SetProperty(pkey, v);
}

CNseItem* CMigRavenFileItem::GenerateChild(CShellFolder* pFolder, PCIDLIST_RELATIVE pidlFolder, PCITEMID_CHILD pidlItem, BOOL bReleaseItem)
{
  // Validate that the child PIDL really is ours
  if (!CPidlMemPtr<NSEFILEPIDLDATA>(pidlItem).IsType(MIGRAVEN_MAGIC_ID)) return nullptr;

  // Spawn new NSE Item instance
  return new CMigRavenFileItem(pFolder, pidlFolder, pidlItem, bReleaseItem);
}

CNseItem* CMigRavenFileItem::GenerateChild(CShellFolder* pFolder, PCIDLIST_RELATIVE pidlFolder, const WIN32_FIND_DATA wfd)
{
  NSEFILEPIDLDATA data = { sizeof(NSEFILEPIDLDATA), MIGRAVEN_MAGIC_ID, 1, wfd };
  return new CMigRavenFileItem(pFolder, pidlFolder, GenerateITEMID(&data, sizeof(data)), TRUE);
}

HRESULT CMigRavenFileItem::GetChild(LPCWSTR pstrName, SHGNO ParseType, CNseItem** pItem)
{
  return E_OUTOFMEMORY;
}

HRESULT CMigRavenFileItem::EnumChildren(HWND hwndOwner, SHCONTF grfFlags, CSimpleValArray<CNseItem*>& aList)
{
  // Only directories have sub-items
  if (!IsFolder()) return E_HANDLE;

  // Request migRaven directories data
  wstring response = CNamedPipe::SendRequestToServer(L"{\"Type\":3,\"Data\":\"" + GetMigravenLocal() + L"\"}");

  // Check there is any data
  if (response == L"" || response == L"null")
  {
    MessageBox(hwndOwner, L"You are not connected to the migRaven 24/7.", L"Error", MB_ICONERROR);
    return S_OK;
  }

  // Parse the migRaven directories data
  const auto migRavenData = JSON::parse(response);
  if (migRavenData.empty()) return S_OK;

  // The migRaven categories e.g. Owner, Shared, and etc
  if (IsRoot())
  {
    return EnumRootCategories(hwndOwner, grfFlags, migRavenData["Categories"], aList);
  }

  // The migRaven directories
  if (m_pMfd->dwAttributes == MR_ATTRIBUTE_CATEGORY)
  {
    return EnumMigRavenFSS(hwndOwner, grfFlags, migRavenData["FssList"], aList);
  }

  // Sytem directories and files
  return EnumSystemItems(hwndOwner, grfFlags, aList);
}

HRESULT CMigRavenFileItem::GetStream(const VFS_STREAM_REASON& Reason, CNseFileStream** ppFile)
{
  MessageBox(0, L"CMigRavenFileItem::GetStream", L"Info", 0);
  return S_OK;
}

HRESULT CMigRavenFileItem::CreateFolder()
{
  MessageBox(0, L"CMigRavenFileItem::CreateFolder", L"Info", 0);
  return S_OK;
}

HRESULT CMigRavenFileItem::Rename(LPCWSTR pstrNewName, LPWSTR pstrOutputName)
{
  MessageBox(0, L"CMigRavenFileItem::Rename", L"Info", 0);
  return S_OK;
}

HRESULT CMigRavenFileItem::Delete()
{
  MessageBox(0, L"CMigRavenFileItem::Delete", L"Info", 0);
  return S_OK;
}

HMENU CMigRavenFileItem::GetMenu()
{
  return GetFSSMenu(m_pMfd->dwAttributes, IsFolder());
}

HRESULT CMigRavenFileItem::ExecuteMenuCommand(VFS_MENUCOMMAND& Cmd)
{
  MessageBox(0, L"CMigRavenFileItem::ExecuteMenuCommand", L"Info", 0);
  return S_OK;
}


///////////////////////////////////////////////////////////////////////////////
// Private functions

CNseItem* CMigRavenFileItem::GenerateChild(CShellFolder* pFolder, PCIDLIST_RELATIVE pidlFolder, const WIN32_FIND_DATA wfd, const MIG_FSS_DATA mfd)
{
  NSEFILEPIDLDATA data = { sizeof(NSEFILEPIDLDATA), MIGRAVEN_MAGIC_ID, 1, wfd, mfd };
  return new CMigRavenFileItem(pFolder, pidlFolder, GenerateITEMID(&data, sizeof(data)), TRUE);
}

HRESULT CMigRavenFileItem::EnumRootCategories(HWND hwndOwner, SHCONTF grfFlags, const JSON& categories, CSimpleValArray<CNseItem*>& aList)
{
  SYSTEMTIME st;
  GetSystemTime(&st);
  FILETIME ft;
  SystemTimeToFileTime(&st, &ft);

  for (auto const category : categories)
  {
    WIN32_FIND_DATA wfd{ FILE_ATTRIBUTE_DIRECTORY, ft, ft, ft };
    wcscpy_s(wfd.cFileName, utf8_to_wstring(category).c_str());

    MIG_FSS_DATA mfd{};
    mfd.dwAttributes = MR_ATTRIBUTE_CATEGORY;

    aList.Add(GenerateChild(m_pFolder, m_pFolder->m_pidlFolder, wfd, mfd));
  }
  return S_OK;
}

HRESULT CMigRavenFileItem::EnumMigRavenFSS(HWND hwndOwner, SHCONTF grfFlags, const JSON& FSS, CSimpleValArray<CNseItem*>& aList)
{
  for (auto const& fss : FSS)
  {
    auto isOwnerOrShared = false;
    if ((fss["UserIsDataOwner"] && wcscmp(m_pWfd->cFileName, L"Owner") == 0) || (!fss["UserIsDataOwner"] && wcscmp(m_pWfd->cFileName, L"Shared") == 0))
    {
      isOwnerOrShared = true;
    }

    const auto fssCategories = fss["Categories"];
    if (!isOwnerOrShared && find(fssCategories.begin(), fssCategories.end(), wstring_to_utf8(m_pWfd->cFileName)) == fssCategories.end()) continue;

    MIG_FSS_DATA mfd{};
    wcscpy_s(mfd.wszUncPath, utf8_to_wstring(fss["DirectoryUNCPath"]).c_str());
    wcscpy_s(mfd.wszIdent, utf8_to_wstring(fss["DirectoryIdent"]).c_str());
    wcscpy_s(mfd.wszFolderAge, utf8_to_wstring(fss["YoungestFiles"]).c_str());
    wcscpy_s(mfd.wszPermissions, utf8_to_wstring(fss["MyPermissions"]).c_str());
    wcscpy_s(mfd.wszDescription, utf8_to_wstring(fss["Description"]).c_str());
    wcscpy_s(mfd.wszInstruction, utf8_to_wstring(fss["InstructionForUse"]).c_str());
    mfd.dwAgeBarCategory = fss["AgeBarCategory"];
    mfd.dwLifetimePercentage = fss["LifetimePercentage"] > 100 ? 100 : fss["LifetimePercentage"];
    mfd.dwAccountCount = fss["AccountCount"];
    mfd.dwFilesCount = fss["FileCount"];
    mfd.dwStatusIcon = fss["StatusIcon"];
    mfd.ftScanDate = TimestampToFiletime(fss["ScanDate"]);
    mfd.ftCreationDate = TimestampToFiletime(fss["CreationDate"]);
    mfd.ftModifyDate = TimestampToFiletime(fss["ModifyDate"]);
    mfd.ftDeleteDate = TimestampToFiletime(fss["DeleteDate"]);
    mfd.uiSize = fss["Size"];
    mfd.dwAttributes = MR_ATTRIBUTE_MIGFOLDER;

    WIN32_FIND_DATA wfd;
    auto hFind = FindFirstFile(mfd.wszUncPath, &wfd);
    if (hFind != INVALID_HANDLE_VALUE)
      aList.Add(GenerateChild(m_pFolder, m_pFolder->m_pidlFolder, wfd, mfd));

    FindClose(hFind);
  }
  return S_OK;
}

HRESULT CMigRavenFileItem::EnumSystemItems(HWND hwndOwner, SHCONTF grfFlags, CSimpleValArray<CNseItem*>& aList)
{
  wstring findPath = m_pMfd->wszUncPath + L"\\*"s;

  WIN32_FIND_DATA wfd;
  auto hFind = FindFirstFile(findPath.c_str(), &wfd);

  MIG_FSS_DATA mfd{};
  mfd.dwAttributes = MR_ATTRIBUTE_SYSTEM;

  do
  {
    if (wcscmp(wfd.cFileName, L".") == 0 || wcscmp(wfd.cFileName, L"..") == 0) continue;

    wstring filePath = m_pMfd->wszUncPath + L"\\"s + wfd.cFileName;
    wcscpy_s(mfd.wszUncPath, filePath.c_str());
    aList.Add(GenerateChild(m_pFolder, m_pFolder->m_pidlFolder, wfd, mfd));

  } while (FindNextFile(hFind, &wfd) != FALSE);

  FindClose(hFind);
  return S_OK;
}

HMENU CMigRavenFileItem::GetFSSMenu(DWORD itemType, BOOL isFolder) const
{

  wstring newFolder = L"New Folder";
  wstring editFolder = L"Edit Folder";
  wstring deleteFolder = L"Delete Folder";
  wstring archiveFolder = L"Archive Folder";
  wstring migrateFolder = L"Migrate Folder";
  wstring folderProperties = L"Folder Properties";
  wstring openWebClient = L"Open Web Client";

  auto language = GetMigravenLocal();
  if (language == L"fa-IR")
  {
    newFolder = L"پوشه جـدید";
    editFolder = L"اصلاح پوشه";
    deleteFolder = L"حذف پوشه";
    archiveFolder = L"بایگانی پوشه";
    migrateFolder = L"تبدیل پوشه";
    folderProperties = L"مشخصات پوشه";
    openWebClient = L"بازکردن صفحه وب ";
  }

  if (language == L"de-DE")
  {
    newFolder = L"Neuer Ordner";
    editFolder = L"Ordner Bearbeiten";
    deleteFolder = L"Ordner Löschen";
    archiveFolder = L"Ordner Archivieren";
    migrateFolder = L"Ordner Migrieren";
    folderProperties = L"Ordner Eigenschaften";
    openWebClient = L"Web Client öffnen";
  }

  HBITMAP hBmpMigraven = LoadBitmap(_pModule->GetResourceInstance(), MAKEINTRESOURCE(IDB_MIGRAVEN));
  HBITMAP hBmpNewFolder = LoadBitmap(_pModule->GetResourceInstance(), MAKEINTRESOURCE(IDB_NEW_FOLDER));
  HBITMAP hBmpEditFolder = LoadBitmap(_pModule->GetResourceInstance(), MAKEINTRESOURCE(IDB_EDIT_FOLDER));
  HBITMAP hBmpDeleteFolder = LoadBitmap(_pModule->GetResourceInstance(), MAKEINTRESOURCE(IDB_DELETE_FOLDER));
  HBITMAP hBmpArchiveFolder = LoadBitmap(_pModule->GetResourceInstance(), MAKEINTRESOURCE(IDB_ARCHIVE_FOLDER));
  HBITMAP hBmpWebClient = LoadBitmap(_pModule->GetResourceInstance(), MAKEINTRESOURCE(IDB_WEB));


  if (itemType == MR_ATTRIBUTE_MIGFOLDER)
  {
    auto hFSSMenu = CreatePopupMenu();
    AppendMenu(hFSSMenu, MF_STRING, ID_CONTEXT_FSS_NEWFOLDER, newFolder.c_str());
    AppendMenu(hFSSMenu, MF_STRING, ID_CONTEXT_FSS_EDITFOLDER, editFolder.c_str());
    AppendMenu(hFSSMenu, MF_STRING, ID_CONTEXT_FSS_DELETEFOLDER, deleteFolder.c_str());
    AppendMenu(hFSSMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenu(hFSSMenu, MF_STRING, ID_CONTEXT_FSS_ARCHIVEFOLDER, archiveFolder.c_str());
    AppendMenu(hFSSMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenu(hFSSMenu, MF_STRING, ID_CONTEXT_FSS_WEBCLIENT, openWebClient.c_str());
    AppendMenu(hFSSMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenu(hFSSMenu, MF_STRING, ID_CONTEXT_MIGFOLDERPROPERTIES, folderProperties.c_str());
    SetMenuItemBitmaps(hFSSMenu, ID_CONTEXT_FSS_NEWFOLDER, MF_BYCOMMAND, hBmpNewFolder, hBmpNewFolder);
    SetMenuItemBitmaps(hFSSMenu, ID_CONTEXT_FSS_EDITFOLDER, MF_BYCOMMAND, hBmpEditFolder, hBmpEditFolder);
    SetMenuItemBitmaps(hFSSMenu, ID_CONTEXT_FSS_DELETEFOLDER, MF_BYCOMMAND, hBmpDeleteFolder, hBmpDeleteFolder);
    SetMenuItemBitmaps(hFSSMenu, ID_CONTEXT_FSS_ARCHIVEFOLDER, MF_BYCOMMAND, hBmpArchiveFolder, hBmpArchiveFolder);
    SetMenuItemBitmaps(hFSSMenu, ID_CONTEXT_FSS_WEBCLIENT, MF_BYCOMMAND, hBmpWebClient, hBmpWebClient);
    SetMenuItemBitmaps(hFSSMenu, ID_CONTEXT_MIGFOLDERPROPERTIES, MF_BYCOMMAND, hBmpMigraven, hBmpMigraven);

    auto hContextMenu = CreatePopupMenu();
    auto hViewMenu = CreatePopupMenu();
    AppendMenu(hContextMenu, MF_STRING | MF_POPUP, (UINT)hFSSMenu, L"Folder Self Service");
    AppendMenu(hViewMenu, MF_STRING | MF_POPUP, (UINT)hFSSMenu, L"Folder Self Service");
    SetMenuItemBitmaps(hContextMenu, (UINT)hFSSMenu, MF_BYCOMMAND, hBmpMigraven, hBmpMigraven);
    SetMenuItemBitmaps(hViewMenu, (UINT)hFSSMenu, MF_BYCOMMAND, hBmpMigraven, hBmpMigraven);

    auto hMenu = CreateMenu();
    AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT)hContextMenu, L"ContextMenu");
    AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT)hViewMenu, L"ViewMenu");

    return hMenu;
  }

  if (isFolder)
  {
    auto hFSSMenu = CreatePopupMenu();
    AppendMenu(hFSSMenu, MF_STRING, ID_CONTEXT_FSS_NEWFOLDER, newFolder.c_str());
    AppendMenu(hFSSMenu, MF_SEPARATOR, 0, nullptr);
    //AppendMenu(hFSSMenu, MF_STRING, ID_CONTEXT_FSS_MIGRATEFOLDER, L"Migrate Folder");
    //AppendMenu(hFSSMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenu(hFSSMenu, MF_STRING, ID_CONTEXT_FSS_WEBCLIENT, openWebClient.c_str());
    SetMenuItemBitmaps(hFSSMenu, ID_CONTEXT_FSS_NEWFOLDER, MF_BYCOMMAND, hBmpNewFolder, hBmpNewFolder);
    //SetMenuItemBitmaps(hFSSMenu, ID_CONTEXT_FSS_MIGRATEFOLDER, MF_BYCOMMAND, hBmpMigrateFolder, hBmpMigrateFolder);
    SetMenuItemBitmaps(hFSSMenu, ID_CONTEXT_FSS_WEBCLIENT, MF_BYCOMMAND, hBmpWebClient, hBmpWebClient);

    auto hContextMenu = CreatePopupMenu();
    auto hViewMenu = CreatePopupMenu();
    AppendMenu(hContextMenu, MF_STRING | MF_POPUP, (UINT)hFSSMenu, L"Folder Self Service");
    AppendMenu(hViewMenu, MF_STRING | MF_POPUP, (UINT)hFSSMenu, L"Folder Self Service");
    SetMenuItemBitmaps(hContextMenu, (UINT)hFSSMenu, MF_BYCOMMAND, hBmpMigraven, hBmpMigraven);
    SetMenuItemBitmaps(hViewMenu, (UINT)hFSSMenu, MF_BYCOMMAND, hBmpMigraven, hBmpMigraven);

    auto hMenu = CreateMenu();
    AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT)hContextMenu, L"ContextMenu");
    AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT)hViewMenu, L"ViewMenu");

    return hMenu;
  }

  auto hFSSMenu = CreatePopupMenu();
  AppendMenu(hFSSMenu, MF_STRING, ID_CONTEXT_FSS_NEWFOLDER, newFolder.c_str());
  AppendMenu(hFSSMenu, MF_SEPARATOR, 0, nullptr);
  AppendMenu(hFSSMenu, MF_STRING, ID_CONTEXT_FSS_WEBCLIENT, openWebClient.c_str());
  SetMenuItemBitmaps(hFSSMenu, ID_CONTEXT_FSS_NEWFOLDER, MF_BYCOMMAND, hBmpNewFolder, hBmpNewFolder);
  SetMenuItemBitmaps(hFSSMenu, ID_CONTEXT_FSS_WEBCLIENT, MF_BYCOMMAND, hBmpWebClient, hBmpWebClient);

  auto hContextMenu = CreatePopupMenu();
  auto hViewMenu = CreatePopupMenu();
  AppendMenu(hContextMenu, MF_STRING | MF_POPUP, (UINT)hFSSMenu, L"Folder Self Service");
  AppendMenu(hViewMenu, MF_STRING | MF_POPUP, (UINT)hFSSMenu, L"Folder Self Service");
  SetMenuItemBitmaps(hContextMenu, (UINT)hFSSMenu, MF_BYCOMMAND, hBmpMigraven, hBmpMigraven);
  SetMenuItemBitmaps(hViewMenu, (UINT)hFSSMenu, MF_BYCOMMAND, hBmpMigraven, hBmpMigraven);

  auto hMenu = CreateMenu();
  AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT)hContextMenu, L"ContextMenu");
  AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT)hViewMenu, L"ViewMenu");
  return hMenu;
}
