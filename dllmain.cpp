#include <windows.h>
#include <Guiddef.h>
#include <strsafe.h>
#include "ShellExt.h"
#define DLLEXPORT __declspec(dllexport)

HINSTANCE g_hInst = nullptr;
long g_cDllRef = 0;

// {5FD4F719-6833-4AB5-8F93-22F557B9EDAD}
static const GUID CLSID_ZstdShellExt =
{ 0x5fd4f719, 0x6833, 0x4ab5, { 0x8f, 0x93, 0x22, 0xf5, 0x57, 0xb9, 0xed, 0xad } };

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID reserved) {
    switch (reason) {
    case DLL_PROCESS_ATTACH:
        g_hInst = hModule;
        DisableThreadLibraryCalls(hModule);
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void** ppv) {
    if (IsEqualCLSID(CLSID_ZstdShellExt, rclsid)) {
        ZstdShellExtClassFactory* pFactory = new (std::nothrow) ZstdShellExtClassFactory();
        if (pFactory == nullptr) {
            return E_OUTOFMEMORY;
        }
        HRESULT hr = pFactory->QueryInterface(riid, ppv);
        pFactory->Release();
        return hr;
    }
    return CLASS_E_CLASSNOTAVAILABLE;
}

STDAPI DllCanUnloadNow(void) {
    return g_cDllRef > 0 ? S_FALSE : S_OK;
}

STDAPI DllRegisterServer(void) {
    WCHAR szModule[MAX_PATH];
    if (GetModuleFileNameW(g_hInst, szModule, ARRAYSIZE(szModule)) == 0) {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    // Register CLSID
    WCHAR szCLSID[MAX_PATH];
    StringFromGUID2(CLSID_ZstdShellExt, szCLSID, ARRAYSIZE(szCLSID));

    WCHAR szKeyPath[MAX_PATH];
    StringCchPrintfW(szKeyPath, ARRAYSIZE(szKeyPath), L"CLSID\\%s", szCLSID);

    HKEY hKey;
    if (RegCreateKeyExW(HKEY_CLASSES_ROOT, szKeyPath, 0, nullptr, REG_OPTION_NON_VOLATILE,
                       KEY_WRITE, nullptr, &hKey, nullptr) != ERROR_SUCCESS) {
        return E_FAIL;
    }

    // Set default value
    RegSetValueEx(hKey, nullptr, 0, REG_SZ, (LPBYTE)L"ZSTD Shell Extension",
                     (wcslen(L"ZSTD Shell Extension") + 1) * sizeof(WCHAR));

    // Add InprocServer32 key
    WCHAR szInprocKey[MAX_PATH];
    StringCchPrintfW(szInprocKey, ARRAYSIZE(szInprocKey), L"CLSID\\%s\\InprocServer32", szCLSID);
    HKEY hKeyInproc;
    if (RegCreateKeyExW(HKEY_CLASSES_ROOT, szInprocKey, 0, nullptr, REG_OPTION_NON_VOLATILE,
                       KEY_WRITE, nullptr, &hKeyInproc, nullptr) != ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return E_FAIL;
                       }

    // Set server path
    RegSetValueExW(hKeyInproc, nullptr, 0, REG_SZ, (LPBYTE)szModule,
                   (wcslen(szModule) + 1) * sizeof(WCHAR));

    // Set threading model
    RegSetValueExW(hKeyInproc, L"ThreadingModel", 0, REG_SZ, (LPBYTE)L"Apartment",
                   (wcslen(L"Apartment") + 1) * sizeof(WCHAR));

    RegCloseKey(hKeyInproc);
    RegCloseKey(hKey);

    // Register shell extension
    StringCchPrintfW(szKeyPath, ARRAYSIZE(szKeyPath),
                   L"*\\shellex\\ContextMenuHandlers\\ZSTDExt");

    if (RegCreateKeyExW(HKEY_CLASSES_ROOT, szKeyPath, 0, nullptr, REG_OPTION_NON_VOLATILE,
                       KEY_WRITE, nullptr, &hKey, nullptr) != ERROR_SUCCESS) {
        return E_FAIL;
    }

    RegSetValueEx(hKey, nullptr, 0, REG_SZ, (LPBYTE)szCLSID,
                 (wcslen(szCLSID) + 1) * sizeof(WCHAR));
    RegCloseKey(hKey);

    return S_OK;
}

STDAPI DllUnregisterServer(void) {
    WCHAR szCLSID[MAX_PATH];
    StringFromGUID2(CLSID_ZstdShellExt, szCLSID, ARRAYSIZE(szCLSID));

    WCHAR szKeyPath[MAX_PATH];
    StringCchPrintfW(szKeyPath, ARRAYSIZE(szKeyPath), L"CLSID\\%s", szCLSID);

    // Remove CLSID registration
    RegDeleteTreeW(HKEY_CLASSES_ROOT, szKeyPath);

    // Remove context menu handler
    StringCchPrintfW(szKeyPath, ARRAYSIZE(szKeyPath),
                   L"*\\shellex\\ContextMenuHandlers\\ZSTDExt");
    RegDeleteTreeW(HKEY_CLASSES_ROOT, szKeyPath);

    return S_OK;
}