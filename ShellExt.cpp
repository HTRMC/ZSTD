#include "ShellExt.h"
#include <strsafe.h>
#include <shlwapi.h>
#include <memory>
#include <filesystem>

extern HINSTANCE g_hInst;
extern long g_cDllRef;

ZstdShellExt::ZstdShellExt() : m_lRef(1) {
    InterlockedIncrement(&g_cDllRef);
}

ZstdShellExt::~ZstdShellExt() {
    InterlockedDecrement(&g_cDllRef);
}

// IUnknown methods
IFACEMETHODIMP ZstdShellExt::QueryInterface(REFIID riid, void** ppv) {
    static const QITAB qit[] = {
        QITABENT(ZstdShellExt, IShellExtInit),
        QITABENT(ZstdShellExt, IContextMenu),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

IFACEMETHODIMP_(ULONG) ZstdShellExt::AddRef() {
    return InterlockedIncrement(&m_lRef);
}

IFACEMETHODIMP_(ULONG) ZstdShellExt::Release() {
    ULONG cRef = InterlockedDecrement(&m_lRef);
    if (0 == cRef) {
        delete this;
    }
    return cRef;
}

// IShellExtInit methods
IFACEMETHODIMP ZstdShellExt::Initialize(PCIDLIST_ABSOLUTE pidlFolder, IDataObject* pdtobj, HKEY hkeyProgID) {
    if (!pdtobj) {
        return E_INVALIDARG;
    }

    FORMATETC fe = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    STGMEDIUM stm;

    if (SUCCEEDED(pdtobj->GetData(&fe, &stm))) {
        HDROP hDrop = static_cast<HDROP>(stm.hGlobal);
        UINT fileCount = DragQueryFileW(hDrop, 0xFFFFFFFF, NULL, 0);

        if (fileCount > 0) {
            WCHAR szFile[MAX_PATH];
            if (DragQueryFileW(hDrop, 0, szFile, ARRAYSIZE(szFile))) {
                m_selectedFile = szFile;
            }
        }

        ReleaseStgMedium(&stm);
    }

    return S_OK;
}

// IContextMenu methods
IFACEMETHODIMP ZstdShellExt::QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags) {
    // If the flags include CMF_DEFAULTONLY then we shouldn't do anything.
    if (uFlags & CMF_DEFAULTONLY) {
        return MAKE_HRESULT(SEVERITY_SUCCESS, 0, 0);
    }

    std::wstring ext = std::filesystem::path(m_selectedFile).extension();

    MENUITEMINFO mii = { sizeof(mii) };
    mii.fMask = MIIM_STRING | MIIM_ID | MIIM_STATE;
    mii.wID = idCmdFirst;
    mii.fState = MFS_ENABLED;

    if (_wcsicmp(ext.c_str(), L".zst") == 0) {
        // File is .zst - show decompress option
        mii.dwTypeData = const_cast<LPWSTR>(L"Decompress with ZSTD");
    } else {
        // Other file - show compress option
        mii.dwTypeData = const_cast<LPWSTR>(L"Compress with ZSTD");
    }

    if (!InsertMenuItem(hmenu, indexMenu, TRUE, &mii)) {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    // Return number of menu items added
    return MAKE_HRESULT(SEVERITY_SUCCESS, 0, 1);
}

IFACEMETHODIMP ZstdShellExt::InvokeCommand(LPCMINVOKECOMMANDINFO pici) {
    // TODO: Implement compression/decompression logic
    MessageBoxW(NULL, L"ZSTD operation will be implemented here", L"ZSTD Shell Extension", MB_OK);
    return S_OK;
}

IFACEMETHODIMP ZstdShellExt::GetCommandString(UINT_PTR idCmd, UINT uFlags, UINT* pwReserved, LPSTR pszName, UINT cchMax) {
    return E_NOTIMPL;
}

// Class factory implementation
ZstdShellExtClassFactory::ZstdShellExtClassFactory() : m_lRef(1) {
    InterlockedIncrement(&g_cDllRef);
}

ZstdShellExtClassFactory::~ZstdShellExtClassFactory() {
    InterlockedDecrement(&g_cDllRef);
}

IFACEMETHODIMP ZstdShellExtClassFactory::QueryInterface(REFIID riid, void** ppv) {
    static const QITAB qit[] = {
        QITABENT(ZstdShellExtClassFactory, IClassFactory),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

IFACEMETHODIMP_(ULONG) ZstdShellExtClassFactory::AddRef() {
    return InterlockedIncrement(&m_lRef);
}

IFACEMETHODIMP_(ULONG) ZstdShellExtClassFactory::Release() {
    ULONG cRef = InterlockedDecrement(&m_lRef);
    if (0 == cRef) {
        delete this;
    }
    return cRef;
}

IFACEMETHODIMP ZstdShellExtClassFactory::CreateInstance(IUnknown* pUnkOuter, REFIID riid, void** ppv) {
    if (pUnkOuter != NULL) {
        return CLASS_E_NOAGGREGATION;
    }

    ZstdShellExt* pExt = new (std::nothrow) ZstdShellExt();
    if (pExt == NULL) {
        return E_OUTOFMEMORY;
    }

    HRESULT hr = pExt->QueryInterface(riid, ppv);
    pExt->Release();
    return hr;
}

IFACEMETHODIMP ZstdShellExtClassFactory::LockServer(BOOL fLock) {
    if (fLock) {
        InterlockedIncrement(&g_cDllRef);
    } else {
        InterlockedDecrement(&g_cDllRef);
    }
    return S_OK;
}