#pragma once

#include "NseFileSystem.h"
#include <json.hpp>

using JSON = nlohmann::json;
using namespace std;

///////////////////////////////////////////////////////////////////////////////
// Definitions

#define MIGRAVEN_MAGIC_ID  0x55

#define MR_ATTRIBUTE_ROOT        0x00000001  
#define MR_ATTRIBUTE_CATEGORY    0x00000002
#define MR_ATTRIBUTE_MIGFOLDER   0x00000003
#define MR_ATTRIBUTE_SYSTEM      0x00000004

#define LANGUAGE_ENGLISH    L"en-US"
#define LANGUAGE_GERMAN     L"de-DE"
#define LANGUAGE_PERSIAN    L"fa-IR"

#define ID_CONTEXT_FSS_NEWFOLDER  3
#define ID_CONTEXT_FSS_EDITFOLDER  4
#define ID_CONTEXT_FSS_DELETEFOLDER  5
#define ID_CONTEXT_MIGFOLDERPROPERTIES  6
#define ID_CONTEXT_FSS_ARCHIVEFOLDER  7
#define ID_CONTEXT_FSS_MIGRATEFOLDER  8
#define ID_CONTEXT_FSS_WEBCLIENT  9

struct MENU_ITEM {
  UINT uFlags;
  UINT_PTR uIDNewItem;
  LPCWSTR lpNewItem;
  INT bitmapIconId;
};

///////////////////////////////////////////////////////////////////////////////
// CMigRavenModule

class CMigRavenModule : public CNseModule
{
public:
  // CShellModule

  virtual LONG GetConfigInt(VFS_CONFIG Item) override;

  virtual BOOL GetConfigBool(VFS_CONFIG Item) override;

  virtual LPCWSTR GetConfigStr(VFS_CONFIG Item) override;

  virtual HRESULT CreateFileSystem(PCIDLIST_ABSOLUTE pidlRoot, CNseFileSystem** ppFS) override;

  virtual HRESULT DllInstall() override;

  virtual HRESULT DllUninstall() override;

  virtual HRESULT ShellAction(LPCWSTR pstrType, LPCWSTR pstrCmdLine) override;

  virtual BOOL DllMain(DWORD dwReason, LPVOID lpReserved) override;

private:
  HRESULT RegisterPropdesc(BOOL regPropdesc);
};


///////////////////////////////////////////////////////////////////////////////
// CMigRavenFileSystem

class CMigRavenFileSystem : public CNseFileSystem
{
public:
  // Reference count

  volatile LONG m_cRef;

  // Constructor

  CMigRavenFileSystem();
  virtual ~CMigRavenFileSystem() {};

  // CShellFileSystem

  virtual VOID AddRef() override;

  virtual VOID Release() override;

  virtual CNseItem* GenerateRoot(CShellFolder* pShellFolder) override;
};


///////////////////////////////////////////////////////////////////////////////
// CMigRavenFileItem

class CMigRavenFileItem : public CNseFileItem
{
public:

  CMigRavenFileItem(CShellFolder* pFolder, PCIDLIST_RELATIVE pidlFolder, PCITEMID_CHILD pidlItem, BOOL bReleaseItem);

  // CNseFileItem

  virtual SFGAOF GetSFGAOF(SFGAOF dwMask) override;

  virtual HRESULT GetProperty(REFPROPERTYKEY pkey, CComPropVariant& v) override;

  virtual HRESULT SetProperty(REFPROPERTYKEY pkey, const CComPropVariant& v) override;

  virtual CNseItem* GenerateChild(CShellFolder* pFolder, PCIDLIST_RELATIVE pidlFolder, PCITEMID_CHILD pidlItem, BOOL bReleaseItem) override;

  virtual CNseItem* GenerateChild(CShellFolder* pFolder, PCIDLIST_RELATIVE pidlFolder, const WIN32_FIND_DATA wfd) override;

  virtual HRESULT GetChild(LPCWSTR pstrName, SHGNO ParseType, CNseItem** pItem);

  virtual HRESULT EnumChildren(HWND hwndOwner, SHCONTF grfFlags, CSimpleValArray<CNseItem*>& aList) override;

  virtual HRESULT GetStream(const VFS_STREAM_REASON& Reason, CNseFileStream** ppFile) override;

  virtual HRESULT CreateFolder();

  virtual HRESULT Rename(LPCWSTR pstrNewName, LPWSTR pstrOutputName) override;

  virtual HRESULT Delete();

  virtual HMENU GetMenu();

  virtual HRESULT ExecuteMenuCommand(VFS_MENUCOMMAND& Cmd) override;

private:
  // Reference to migRaven data inside PIDL
  MIG_FSS_DATA* m_pMfd;

  CNseItem* GenerateChild(CShellFolder* pFolder, PCIDLIST_RELATIVE pidlFolder, const WIN32_FIND_DATA wfd, const MIG_FSS_DATA mfd);

  HRESULT EnumRootCategories(HWND hwndOwner, SHCONTF grfFlags, const JSON& categories, CSimpleValArray<CNseItem*>& aList);

  HRESULT EnumMigRavenFSS(HWND hwndOwner, SHCONTF grfFlags, const JSON& FSS, CSimpleValArray<CNseItem*>& aList);

  HRESULT EnumSystemItems(HWND hwndOwner, SHCONTF grfFlags, CSimpleValArray<CNseItem*>& aList);

  HMENU GetFSSMenu(DWORD itemType, BOOL isFolder) const;

  HMENU GenerateMenu(const vector<MENU_ITEM>& vMenuItems) const;
};


///////////////////////////////////////////////////////////////////////////////
// CMigRavenFileStream

class CMigRavenFileStream : public CNseFileStream
{
public:
  // Reference to source archive
  CRefPtr<CMigRavenFileSystem> m_spFS;

  // Local path to file
  WCHAR m_wszFilename[MAX_PATH];

  // Memory buffer
  LPBYTE m_pData;

  // Allocated memory so far
  DWORD m_dwAllocSize;

  // Current position in memory buffer
  DWORD m_dwCurPos;

  // Known size of file in memory
  DWORD m_dwFileSize;

  // Stream opened for read or write access?
  UINT m_uAccess;

  // Constructor

  CMigRavenFileStream(CMigRavenFileSystem* pFS, LPCWSTR pstrFilename, UINT uAccess);
  virtual ~CMigRavenFileStream();

  // CNseFileStream

  virtual HRESULT Init() override;

  virtual HRESULT Read(LPVOID pData, ULONG dwSize, ULONG& dwRead) override;

  virtual HRESULT Write(LPCVOID pData, ULONG dwSize, ULONG& dwWritten) override;

  virtual HRESULT Seek(DWORD dwPos) override;

  virtual HRESULT GetCurPos(DWORD* pdwPos) override;

  virtual HRESULT GetFileSize(DWORD* pdwFileSize) override;

  virtual HRESULT SetFileSize(DWORD dwSize) override;

  virtual HRESULT Commit() override;

  virtual HRESULT Close() override;

};
