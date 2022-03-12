#include "pch.h"
#include "FileSystem.h"
#include "ShellFolder.h"
#include <string>
#include <Utils.h>
#include <NamedPipe.h>
#include "ShPropKeys.h"

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

HRESULT CMigRavenFileItem::GetColumnInfo(UINT iColumn, VFS_COLUMNINFO& Column)
{
  static vector<VFS_COLUMNINFO> aColumns = {
     { PKEY_ItemNameDisplay,             SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT,                    0 },
     { PKEY_ItemTypeText,                SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT | SHCOLSTATE_SLOW,  0 },
     { PKEY_Size,                        SHCOLSTATE_TYPE_INT | SHCOLSTATE_ONBYDEFAULT,                    0 },
     { PKEY_FilesCount,                  SHCOLSTATE_TYPE_INT | SHCOLSTATE_SECONDARYUI,                    0 },
     { PKEY_DateDelete,                  SHCOLSTATE_TYPE_DATE | SHCOLSTATE_SECONDARYUI,                   0 },
     { PKEY_ScanDate,                    SHCOLSTATE_TYPE_DATE | SHCOLSTATE_SECONDARYUI,                   0 },
     { PKEY_DateCreated,                 SHCOLSTATE_TYPE_DATE | SHCOLSTATE_ONBYDEFAULT,                   0 },
     { PKEY_DateModified,                SHCOLSTATE_TYPE_DATE | SHCOLSTATE_SECONDARYUI,                   0 },
     { PKEY_UIIconStatus,                SHCOLSTATE_SLOW | SHCOLSTATE_SECONDARYUI,                        0 },
     { PKEY_AgeIconStatus,               SHCOLSTATE_SLOW | SHCOLSTATE_SECONDARYUI,                        0 },
     { PKEY_StatusProgressBar,           SHCOLSTATE_SLOW | SHCOLSTATE_SECONDARYUI,                        0 },
     { PKEY_ItemPathDisplay,             SHCOLSTATE_TYPE_STR | SHCOLSTATE_SECONDARYUI,                    0 },
     { PKEY_AccountCount,                SHCOLSTATE_TYPE_INT | SHCOLSTATE_HIDDEN,                         0 },
     { PKEY_FolderAge,                   SHCOLSTATE_TYPE_STR | SHCOLSTATE_HIDDEN,                         0 },
     { PKEY_Instruction,                 SHCOLSTATE_TYPE_STR | SHCOLSTATE_HIDDEN,                         0 },
     { PKEY_Permissions,                 SHCOLSTATE_TYPE_STR | SHCOLSTATE_HIDDEN,                         0 },
     { PKEY_ItemType,                    SHCOLSTATE_TYPE_STR | SHCOLSTATE_HIDDEN,                         0 },
     { PKEY_ItemPathDisplayNarrow,       SHCOLSTATE_TYPE_STR | SHCOLSTATE_HIDDEN,                         0 },
     { PKEY_ItemFolderPathDisplayNarrow, SHCOLSTATE_TYPE_STR | SHCOLSTATE_HIDDEN,                         0 },
     { PKEY_FileName,                    SHCOLSTATE_TYPE_STR | SHCOLSTATE_HIDDEN,                         VFS_COLF_WRITEABLE },
     { PKEY_FileExtension,               SHCOLSTATE_TYPE_INT | SHCOLSTATE_HIDDEN,                         0 },
     { PKEY_ParsingName,                 SHCOLSTATE_TYPE_STR | SHCOLSTATE_HIDDEN,                         0 },
     { PKEY_ParsingPath,                 SHCOLSTATE_TYPE_STR | SHCOLSTATE_HIDDEN,                         0 },
     { PKEY_PerceivedType,               SHCOLSTATE_TYPE_INT | SHCOLSTATE_HIDDEN,                         VFS_COLF_NOTCOLUMN },
     { PKEY_SFGAOFlags,                  SHCOLSTATE_TYPE_INT | SHCOLSTATE_HIDDEN,                         VFS_COLF_NOTCOLUMN },
     { PKEY_Volume_IsRoot,               SHCOLSTATE_TYPE_INT | SHCOLSTATE_HIDDEN,                         VFS_COLF_NOTCOLUMN },
     { PKEY_PropList_InfoTip,            SHCOLSTATE_TYPE_STR | SHCOLSTATE_HIDDEN,                         VFS_COLF_NOTCOLUMN },
     { PKEY_PropList_TileInfo,           SHCOLSTATE_TYPE_STR | SHCOLSTATE_HIDDEN,                         VFS_COLF_NOTCOLUMN },
     { PKEY_PropList_FullDetails,        SHCOLSTATE_TYPE_STR | SHCOLSTATE_HIDDEN,                         VFS_COLF_NOTCOLUMN },
     { PKEY_PropList_PreviewTitle,       SHCOLSTATE_TYPE_STR | SHCOLSTATE_HIDDEN,                         VFS_COLF_NOTCOLUMN },
     { PKEY_PropList_PreviewDetails,     SHCOLSTATE_TYPE_STR | SHCOLSTATE_HIDDEN,                         VFS_COLF_NOTCOLUMN },
     { PKEY_PropList_ExtendedTileInfo,   SHCOLSTATE_TYPE_STR | SHCOLSTATE_HIDDEN,                         VFS_COLF_NOTCOLUMN },
  };

  if (iColumn >= aColumns.size())
    return E_FAIL;
  Column = aColumns[iColumn];
  return S_OK;
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
  wstring newFolder = L"New Folder";
  wstring editFolder = L"Edit Folder";
  wstring deleteFolder = L"Delete Folder";
  wstring archiveFolder = L"Archive Folder";
  wstring migrateFolder = L"Migrate Folder";
  wstring folderProperties = L"Folder Properties";
  wstring openWebClient = L"Open Web Client";

  auto language = GetMigravenLocal();
  if (language == LANGUAGE_PERSIAN)
  {
    newFolder = L"پوشه جـدید";
    editFolder = L"اصلاح پوشه";
    deleteFolder = L"حذف پوشه";
    archiveFolder = L"بایگانی پوشه";
    migrateFolder = L"تبدیل پوشه";
    folderProperties = L"مشخصات پوشه";
    openWebClient = L"بازکردن صفحه وب ";
  }

  if (language == LANGUAGE_GERMAN)
  {
    newFolder = L"Neuer Ordner";
    editFolder = L"Ordner Bearbeiten";
    deleteFolder = L"Ordner Löschen";
    archiveFolder = L"Ordner Archivieren";
    migrateFolder = L"Ordner Migrieren";
    folderProperties = L"Ordner Eigenschaften";
    openWebClient = L"Web Client öffnen";
  }

  if (m_pMfd->dwAttributes == MR_ATTRIBUTE_MIGFOLDER)
  {
    const vector<MENU_ITEM> vMenuItems = {
      { MF_STRING, ID_CONTEXT_FSS_NEWFOLDER, newFolder.c_str(), IDB_NEW_FOLDER},
      { MF_STRING, ID_CONTEXT_FSS_EDITFOLDER, editFolder.c_str(), IDB_EDIT_FOLDER},
      { MF_STRING, ID_CONTEXT_FSS_DELETEFOLDER, deleteFolder.c_str(), IDB_DELETE_FOLDER},
      { MF_SEPARATOR, 0, nullptr },
      { MF_STRING, ID_CONTEXT_FSS_ARCHIVEFOLDER, archiveFolder.c_str(),  IDB_ARCHIVE_FOLDER},
      { MF_SEPARATOR, 0, nullptr },
      { MF_STRING, ID_CONTEXT_FSS_WEBCLIENT, openWebClient.c_str(), IDB_WEB},
      { MF_SEPARATOR, 0, nullptr },
      { MF_STRING, ID_CONTEXT_MIGFOLDERPROPERTIES, folderProperties.c_str(),  IDB_MIGRAVEN},
    };
    return GenerateMenu(vMenuItems);
  }

  if (IsFolder() && !IsRoot() && m_pMfd->dwAttributes == MR_ATTRIBUTE_SYSTEM)
  {
    const vector<MENU_ITEM> vMenuItems = {
      { MF_STRING, ID_CONTEXT_FSS_NEWFOLDER, newFolder.c_str(), IDB_NEW_FOLDER},
      { MF_SEPARATOR, 0, nullptr },
      { MF_STRING, ID_CONTEXT_FSS_MIGRATEFOLDER, migrateFolder.c_str(), IDB_MIGRAVEN},
      { MF_SEPARATOR, 0, nullptr },
      { MF_STRING, ID_CONTEXT_FSS_WEBCLIENT, openWebClient.c_str(), IDB_WEB},
    };
    return GenerateMenu(vMenuItems);
  }

  const vector<MENU_ITEM> vMenuItems = {
    { MF_STRING, ID_CONTEXT_FSS_NEWFOLDER, newFolder.c_str(), IDB_NEW_FOLDER},
    { MF_SEPARATOR, 0, nullptr },
    { MF_STRING, ID_CONTEXT_FSS_WEBCLIENT, openWebClient.c_str(), IDB_WEB},
  };
  return GenerateMenu(vMenuItems);
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

HMENU CMigRavenFileItem::GenerateMenu(const vector<MENU_ITEM>& vMenuItems) const
{
  const auto hContextMenu = CreatePopupMenu();
  const auto hViewMenu = CreatePopupMenu();
  const auto hFSSMenu = CreatePopupMenu();

  for (const auto& menuItem : vMenuItems)
  {
    AppendMenu(hFSSMenu, menuItem.uFlags, menuItem.uIDNewItem, menuItem.lpNewItem);
    if (menuItem.bitmapIconId != 0) {
      const auto bitmapIcon = LoadBitmap(_pModule->GetResourceInstance(), MAKEINTRESOURCE(menuItem.bitmapIconId));
      SetMenuItemBitmaps(hFSSMenu, menuItem.uIDNewItem, MF_BYCOMMAND, bitmapIcon, bitmapIcon);
    }
  }

  const auto migravenBitmapIcon = LoadBitmap(_pModule->GetResourceInstance(), MAKEINTRESOURCE(IDB_MIGRAVEN));

  AppendMenu(hContextMenu, MF_STRING | MF_POPUP, (UINT)hFSSMenu, L"Folder Self Service");
  AppendMenu(hViewMenu, MF_STRING | MF_POPUP, (UINT)hFSSMenu, L"Folder Self Service");
  SetMenuItemBitmaps(hContextMenu, (UINT)hFSSMenu, MF_BYCOMMAND, migravenBitmapIcon, migravenBitmapIcon);
  SetMenuItemBitmaps(hViewMenu, (UINT)hFSSMenu, MF_BYCOMMAND, migravenBitmapIcon, migravenBitmapIcon);

  const auto hMenu = CreateMenu();
  AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT)hContextMenu, L"ContextMenu");
  AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT)hViewMenu, L"ViewMenu");

  return hMenu;
}
