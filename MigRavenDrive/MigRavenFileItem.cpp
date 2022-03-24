#include "pch.h"

#include <string>
#include "FileSystem.h"
#include "ShellFolder.h"
#include "Utils.h"
#include "NamedPipe.h"
#include "ShPropKeys.h"
#include "PathCch.h"

CMigRavenFileItem::CMigRavenFileItem(CShellFolder* pFolder, PCIDLIST_RELATIVE pidlFolder, PCITEMID_CHILD pidlItem, BOOL bReleaseItem) :
  CNseFileItem(pFolder, pidlFolder, pidlItem, bReleaseItem)
{
  if (pidlItem == nullptr)
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

HRESULT CMigRavenFileItem::GetNameOf(SHGDNF uFlags, LPWSTR* ppstrRetVal)
{
  if (!IsBitSet(SHGDN_INFOLDER, uFlags) && IsBitSet(SHGDN_FORPARSING, uFlags))
  {
    WCHAR wczPath[MAX_PATH]{ '\0' };
    GetPathQuick(m_pidlFolder, m_pidlItem, wczPath);
    return SHStrDup(wczPath, ppstrRetVal);
  }

  return SHStrDup(m_pWfd->cFileName, ppstrRetVal);
}

SFGAOF CMigRavenFileItem::GetSFGAOF(SFGAOF dwMask)
{
  if (m_pMfd->dwAttributes == MR_ATTRIBUTE_SYSTEM && !IsRoot())
    return CNseFileItem::GetSFGAOF(dwMask)
    | SFGAO_CANCOPY
    | SFGAO_CANMOVE
    | SFGAO_CANDELETE
    | SFGAO_CANRENAME
    | SFGAO_FILESYSTEM
    | SFGAO_HASPROPSHEET;

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
     { PKEY_PreviewPaneIconStatus,       SHCOLSTATE_SLOW | SHCOLSTATE_SECONDARYUI,                        VFS_COLF_NOTCOLUMN },
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
  if (IsEqualPropertyKey(pkey, PKEY_FilesCount))
  {
    return InitPropVariantFromUInt32(m_pMfd->dwFilesCount, &v);
  }

  if (IsEqualPropertyKey(pkey, PKEY_AccountCount))
  {
    return InitPropVariantFromUInt32(m_pMfd->dwAccountCount, &v);
  }

  if (IsEqualPropertyKey(pkey, PKEY_StatusProgressBar))
  {
    if (m_pMfd->dwLifetimePercentage == 0)
    {
      v.vt = VT_EMPTY;
      return E_NOTIMPL;
    }
    return InitPropVariantFromUInt32(m_pMfd->dwLifetimePercentage, &v);
  }

  if (IsEqualPropertyKey(pkey, PKEY_DateDelete))
  {
    FILETIME noTime{ 3577643008,27111902 };
    if (CompareFileTime(&m_pMfd->ftDeleteDate, &noTime) != 0)
      return InitPropVariantFromFileTime(&m_pMfd->ftDeleteDate, &v);
  }

  if (IsEqualPropertyKey(pkey, PKEY_ScanDate))
  {
    return InitPropVariantFromFileTime(&m_pMfd->ftScanDate, &v);
  }

  if (IsEqualPropertyKey(pkey, PKEY_FolderAge))
  {
    return InitPropVariantFromString(m_pMfd->wszFolderAge, &v);
  }

  if (IsEqualPropertyKey(pkey, PKEY_Permissions))
  {
    return InitPropVariantFromString(m_pMfd->wszPermissions, &v);
  }

  if (IsEqualPropertyKey(pkey, PKEY_Instruction))
  {
    return InitPropVariantFromString(m_pMfd->wszInstruction, &v);
  }

  if (IsEqualPropertyKey(pkey, PKEY_UIIconStatus))
  {
    const vector<PROPERTYKEY> propKeys{ PKEY_IconStatus };
    return GetStatusIcon(2, m_pMfd->dwStatusIcon, propKeys, L"prop:MigRavenDrive.IconStatus", v);
  }

  if (IsEqualPropertyKey(pkey, PKEY_AgeIconStatus))
  {
    const vector<PROPERTYKEY> propKeys
    {
      PKEY_AgeStatus1,
      PKEY_AgeStatus2,
      PKEY_AgeStatus3,
      PKEY_AgeStatus4,
      PKEY_AgeStatus5,
      PKEY_AgeStatus6,
      PKEY_AgeStatus7,
      PKEY_AgeStatus8
    };

    return GetStatusIcon(
      2,
      m_pMfd->dwAgeBarCategory,
      propKeys,
      LR"(prop:MigRavenDrive.AgeStatus1;MigRavenDrive.AgeStatus2;MigRavenDrive.AgeStatus3;MigRavenDrive.AgeStatus4;
      MigRavenDrive.AgeStatus5;MigRavenDrive.AgeStatus6;MigRavenDrive.AgeStatus7;MigRavenDrive.AgeStatus8;)",
      v);
  }

  if (IsEqualPropertyKey(pkey, PKEY_PreviewPaneIconStatus))
  {
    const vector<PROPERTYKEY> propKeys{ PKEY_IconStatus };
    return GetStatusIcon(3, m_pMfd->dwStatusIcon, propKeys, L"prop:MigRavenDrive.IconStatus", v);
  }

  if (m_pMfd->dwAttributes == MR_ATTRIBUTE_MIGFOLDER)
  {
    if (IsEqualPropertyKey(pkey, PKEY_PropList_PreviewDetails))
      return InitPropVariantFromString(L"prop:MigRavenDrive.ScanDate;MigRavenDrive.AccountCount;MigRavenDrive.Permissions;System.DateCreated;MigRavenDrive.DateDelete;MigRavenDrive.FolderAge;MigRavenDrive.FilesCount;System.ItemPathDisplay;MigRavenDrive.Instruction;MigRavenDrive.StatusProgressBar;MigRavenDrive.PreviewPaneIConStatus;", &v);

    if (IsEqualPropertyKey(pkey, PKEY_PropList_ContentViewModeForBrowse))
      return InitPropVariantFromString(L"prop:System.ItemNameDisplay;MigRavenDrive.StatusProgressBar;", &v);

    if (IsEqualPropertyKey(pkey, PKEY_PropList_TileInfo))
      return InitPropVariantFromString(L"prop:MigRavenDrive.StatusProgressBar;MigRavenDrive.UIIConStatus;", &v);

    if (IsEqualPropertyKey(pkey, PKEY_PropList_InfoTip))
      return InitPropVariantFromString(L"prop:MigRavenDrive.ScanDate;MigRavenDrive.AccountCount;MigRavenDrive.Permissions;System.DateCreated;MigRavenDrive.DateDelete;MigRavenDrive.FolderAge;MigRavenDrive.FilesCount;System.ItemPathDisplay;MigRavenDrive.Instruction;", &v);
  }

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
  wstring childPath = m_pMfd->wszUncPath + L"\\"s + pstrName;

  WIN32_FIND_DATA wfd;
  auto hFind = FindFirstFile(childPath.c_str(), &wfd);

  MIG_FSS_DATA mfd{};
  mfd.dwAttributes = MR_ATTRIBUTE_SYSTEM;

  if (hFind != INVALID_HANDLE_VALUE) {
    wcscpy_s(mfd.wszUncPath, childPath.c_str());
    wfd.cAlternateFileName[1] == VFS_HACK_SAVEAS_JUNCTION;
    *pItem = GenerateChild(m_pFolder, m_pFolder->m_pidlFolder, wfd, mfd);
  }
  FindClose(hFind);
  return *pItem != nullptr ? S_OK : E_OUTOFMEMORY;
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
  WCHAR wczPath[MAX_PATH]{ '\0' };
  GetPathQuick(m_pidlFolder, m_pidlItem, wczPath);

  DWORD size = m_pWfd->nFileSizeHigh * (MAXDWORD + 1) + m_pWfd->nFileSizeLow;
  *ppFile = new CMigRavenFileStream(dynamic_cast<CMigRavenFileSystem*>(m_pFolder->m_spFS.m_p), wczPath, Reason.uAccess, size);

  return *ppFile != nullptr ? S_OK : E_OUTOFMEMORY;
}

HRESULT CMigRavenFileItem::CreateFolder()
{
  // Create new directory in archive
  WCHAR wszFolderPath[MAX_PATH] = { '\0' };
  HR(GetPathQuick(m_pidlFolder, m_pidlItem, wszFolderPath));
  CreateDirectory(wszFolderPath, nullptr);

  // Update properties of our NSE Item
  auto hFind = FindFirstFile(wszFolderPath, m_pWfd);
  m_pWfd->cAlternateFileName[1] == VFS_HACK_SAVEAS_JUNCTION;
  FindClose(hFind);

  m_pMfd->dwAttributes = MR_ATTRIBUTE_SYSTEM;
  wcscpy_s(m_pMfd->wszUncPath, wszFolderPath);

  return S_OK;
}

HRESULT CMigRavenFileItem::Rename(LPCWSTR pstrNewName, LPWSTR pstrOutputName)
{
  WCHAR wszOldFileName[MAX_PATH] = { '\0' };
  HR(GetPathQuick(m_pidlFolder, m_pidlItem, wszOldFileName));

  // The Shell often doesn't include the filename extension in the renamed filename, so we'll append it now.
  if (wcschr(pstrOutputName, '.') == NULL)
    ::PathCchAddExtension(pstrOutputName, MAX_PATH, ::PathFindExtension(wszOldFileName));

  WCHAR wszNewFileName[MAX_PATH] = { '\0' };
  wcscpy_s(wszNewFileName, wszOldFileName);
  PathRemoveFileSpec(wszNewFileName);
  ::PathCchAppend(wszNewFileName, MAX_PATH, pstrOutputName);

  // Rename the item
  if (_wrename(wszOldFileName, wszNewFileName) != 0)
    MessageBox(0, L"Could not rename the selected file/folder.", L"Error", MB_ICONERROR);

  return S_OK;
}

HRESULT CMigRavenFileItem::Delete()
{
  WCHAR wszFileName[MAX_PATH] = { '\0' };
  HR(GetPathQuick(m_pidlFolder, m_pidlItem, wszFileName));

  // Must be double null terminated
  wszFileName[wcslen(wszFileName) + 1] = '\0';

  SHFILEOPSTRUCT shfo = {
      nullptr,
      FO_DELETE,
      wszFileName,
      nullptr,
      FOF_NO_UI,
      FALSE
  };

  if (SHFileOperation(&shfo) != 0)
    MessageBox(0, L"Could not delete the selected files / folders.", L"Error", MB_ICONERROR);

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
  switch (Cmd.wMenuID)
  case ID_CONTEXT_FSS_NEWFOLDER: {
    ExecuteFSSCommand(Cmd.hWnd, L"create", nullptr);
    return S_OK;
  case ID_CONTEXT_FSS_EDITFOLDER:
    ExecuteFSSCommand(Cmd.hWnd, L"edit", m_pMfd->wszUncPath);
    return S_OK;
  case ID_CONTEXT_FSS_DELETEFOLDER:
    ExecuteFSSCommand(Cmd.hWnd, L"delete", m_pMfd->wszUncPath);
    return S_OK;
  case ID_CONTEXT_MIGFOLDERPROPERTIES:
    ExecuteFSSCommand(Cmd.hWnd, L"properties", m_pMfd->wszUncPath);
    return S_OK;
  case ID_CONTEXT_FSS_ARCHIVEFOLDER:
    ExecuteFSSCommand(Cmd.hWnd, L"archive", m_pMfd->wszUncPath);
    return S_OK;
  case ID_CONTEXT_FSS_MIGRATEFOLDER:
    ExecuteFSSCommand(Cmd.hWnd, L"migrate", m_pMfd->wszUncPath);
    return S_OK;
  case ID_CONTEXT_FSS_WEBCLIENT:
    ExecuteFSSCommand(Cmd.hWnd, L"webclient", nullptr);
    return S_OK;
  case DFM_CMD_PASTE:
    auto hr = _DoPasteFiles(Cmd);
    return hr;
  }

  //MessageBox(0, L"CMigRavenFileItem::ExecuteMenuCommand", L"Info", 0);
  return E_NOTIMPL;
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
    wfd.cAlternateFileName[1] == VFS_HACK_SAVEAS_JUNCTION;
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

HRESULT CMigRavenFileItem::GetStatusIcon(BYTE type, BYTE iconIndex, const vector<PROPERTYKEY>& propKeys, PCWSTR propStr, CComPropVariant& v)
{
  PROPVARIANT tmp;
  CComPtr<IPropertyStore> pPropertyStore;
  CComPtr<IPersistSerializedPropStorage> pPSP;
  SERIALIZEDPROPSTORAGE* pSP;
  DWORD cbSize = 0;

  auto hr = PSCreateMemoryPropertyStore(IID_PPV_ARGS(&pPropertyStore));

  hr = InitPropVariantFromString(propStr, &tmp);
  pPropertyStore->SetValue(PKEY_PropList_StatusIcons, tmp);

  hr = InitPropVariantFromUInt32(type, &tmp);
  pPropertyStore->SetValue(PKEY_PropList_StatusIconsDisplayFlag, tmp);

  if (propKeys.size() <= 1)
  {
    InitPropVariantFromUInt32(iconIndex, &tmp);
    pPropertyStore->SetValue(propKeys[0], tmp);
  }
  else
  {
    for (size_t i = 0; i < propKeys.size(); i++)
    {
      InitPropVariantFromUInt32(i + 1 <= iconIndex ? 1 : 2, &tmp);
      pPropertyStore->SetValue(propKeys[i], tmp);
    }
  }

  pPropertyStore->QueryInterface(&pPSP);

  hr = pPSP->GetPropertyStorage(&pSP, &cbSize);

  v.vt = VT_BLOB;
  v.blob.pBlobData = (BYTE*)pSP;
  v.blob.cbSize = cbSize;

  PropVariantClear(&tmp);

  return hr;
}

HINSTANCE CMigRavenFileItem::ExecuteFSSCommand(HWND hwndOwner, PCWCHAR pwszOperation, PCWCHAR pwszFolderPath) const
{
  WCHAR wszFssAppPath[MAX_PATH]{ '\0' };
  GetModuleFileName(_pModule->GetModuleInstance(), wszFssAppPath, MAX_PATH);
  PathCchRemoveFileSpec(wszFssAppPath, MAX_PATH);
  PathCchAppend(wszFssAppPath, MAX_PATH, L"migRavenFssApp.exe");

  WCHAR wszParameters[MAX_PATH]{ '\0' };
  swprintf(wszParameters, MAX_PATH, L"%s \"%s\"", pwszOperation, pwszFolderPath);

  return ShellExecute(hwndOwner, L"open", wszFssAppPath, wszParameters, nullptr, SW_NORMAL);
}

HRESULT CMigRavenFileItem::GetPathQuick(PCIDLIST_RELATIVE pidlPath, PCITEMID_CHILD pidlChild, LPWSTR pszPath) const
{
  wstring parentPath{};
  wstring childPath{};
  ATLASSERT(ILIsAligned64(pidlPath));
  if (::ILIsEmpty(pidlPath)) {
    pidlPath = pidlChild;
    pidlChild = nullptr;
  }
  while (!::ILIsEmpty(pidlPath)) {
    const MIG_FSS_DATA* pMfd = &(reinterpret_cast<const NSEFILEPIDLDATA*>(pidlPath))->mfd;
    parentPath = pMfd->wszUncPath;
    pidlPath = static_cast<PCIDLIST_RELATIVE>(::ILNext(pidlPath));
  }
  if (!::ILIsEmpty(pidlChild)) {
    ATLASSERT(::ILIsChild(pidlChild));
    const WIN32_FIND_DATA* pWfd = &(reinterpret_cast<const NSEFILEPIDLDATA*>(pidlChild))->wfd;
    childPath = pWfd->cFileName;
  }

  wstring path = parentPath + L"\\"s + childPath;
  wcscpy_s(pszPath, MAX_PATH, path.c_str());
  return S_OK;
}