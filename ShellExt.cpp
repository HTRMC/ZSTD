#include "ShellExt.h"
#include <strsafe.h>
#include <shlwapi.h>
#include <memory>
#include <filesystem>
#include <fstream>
#include <vector>
#include <string>
#include <zstd.h>

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
// Check if we're invoked directly or through command prompt
    if (HIWORD(pici->lpVerb) != 0)
        return E_INVALIDARG;

    std::wstring filePath = m_selectedFile;
    std::wstring ext = std::filesystem::path(filePath).extension();
    bool isDecompressing = _wcsicmp(ext.c_str(), L".zst") == 0;

    try {
        // Open input file
        std::ifstream inFile(filePath, std::ios::binary);
        if (!inFile.is_open()) {
            MessageBoxW(NULL, L"Failed to open input file", L"Error", MB_ICONERROR);
            return E_FAIL;
        }

        // Get file size
        inFile.seekg(0, std::ios::end);
        size_t inSize = inFile.tellg();
        inFile.seekg(0, std::ios::beg);

        // Read input file
        std::vector<char> inBuffer(inSize);
        inFile.read(inBuffer.data(), inSize);
        inFile.close();

        if (isDecompressing) {
            // Determine output path by removing .zst extension
            std::wstring outPath = filePath.substr(0, filePath.length() - 4);

            // Get original size from frame header
            unsigned long long const rSize = ZSTD_getFrameContentSize(inBuffer.data(), inSize);
            if (rSize == ZSTD_CONTENTSIZE_ERROR) {
                MessageBoxW(NULL, L"Not a valid ZSTD compressed file", L"Error", MB_ICONERROR);
                return E_FAIL;
            }
            if (rSize == ZSTD_CONTENTSIZE_UNKNOWN) {
                MessageBoxW(NULL, L"Original size unknown - cannot decompress", L"Error", MB_ICONERROR);
                return E_FAIL;
            }

            // Allocate output buffer
            std::vector<char> outBuffer(rSize);

            // Decompress
            size_t const dSize = ZSTD_decompress(outBuffer.data(), rSize, inBuffer.data(), inSize);
            if (ZSTD_isError(dSize)) {
                MessageBoxW(NULL, L"Decompression failed", L"Error", MB_ICONERROR);
                return E_FAIL;
            }

            // Write output file
            std::ofstream outFile(outPath, std::ios::binary);
            if (!outFile.is_open()) {
                MessageBoxW(NULL, L"Failed to create output file", L"Error", MB_ICONERROR);
                return E_FAIL;
            }
            outFile.write(outBuffer.data(), dSize);
            outFile.close();
        } else {
            // Compressing - add .zst extension
            std::wstring outPath = filePath + L".zst";

            // Get a safe buffer size
            size_t const cBuffSize = ZSTD_compressBound(inSize);
            std::vector<char> outBuffer(cBuffSize);

            // Compress with default level
            size_t const cSize = ZSTD_compress(outBuffer.data(), cBuffSize, inBuffer.data(), inSize, ZSTD_CLEVEL_DEFAULT);
            if (ZSTD_isError(cSize)) {
                MessageBoxW(NULL, L"Compression failed", L"Error", MB_ICONERROR);
                return E_FAIL;
            }

            // Write compressed file
            std::ofstream outFile(outPath, std::ios::binary);
            if (!outFile.is_open()) {
                MessageBoxW(NULL, L"Failed to create output file", L"Error", MB_ICONERROR);
                return E_FAIL;
            }
            outFile.write(outBuffer.data(), cSize);
            outFile.close();
        }

        return S_OK;
    }
    catch (const std::exception& e) {
        std::string errMsg = "Operation failed: " + std::string(e.what());
        MessageBoxA(NULL, errMsg.c_str(), "Error", MB_ICONERROR);
        return E_FAIL;
    }
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