#pragma once
#include <windows.h>
#include <shlobj.h>     // For IShellExtInit and IContextMenu
#include <memory>
#include <string>

class ZstdShellExt : public IShellExtInit, public IContextMenu {
private:
    long m_lRef;
    std::wstring m_selectedFile;

public:
    ZstdShellExt();
    ~ZstdShellExt();

    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IShellExtInit methods
    STDMETHODIMP Initialize(PCIDLIST_ABSOLUTE pidlFolder, IDataObject* pdtobj, HKEY hkeyProgID);

    // IContextMenu methods
    STDMETHODIMP QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
    STDMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO pici);
    STDMETHODIMP GetCommandString(UINT_PTR idCmd, UINT uFlags, UINT* pwReserved, LPSTR pszName, UINT cchMax);
};

// {Your-GUID-Here} - We'll generate this later
// Example: {5FD4F719-6833-4AB5-8F93-22F557B9EDAD}
class __declspec(uuid("{5FD4F719-6833-4AB5-8F93-22F557B9EDAD}")) ZstdShellExtClassFactory : public IClassFactory {
    private:
    long m_lRef;

    public:
    ZstdShellExtClassFactory();
    ~ZstdShellExtClassFactory();

    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IClassFactory methods
    STDMETHODIMP CreateInstance(IUnknown* pUnkOuter, REFIID riid, void** ppv);
    STDMETHODIMP LockServer(BOOL fLock);
};