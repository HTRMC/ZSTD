#include <windows.h>
#include <iostream>
#include <string>
#include <filesystem>

int main(int argc, char* argv[]) {
    if (argc != 2 || (strcmp(argv[1], "/register") != 0 && strcmp(argv[1], "/unregister") != 0)) {
        std::cout << "Usage: RegisterExtension /register|/unregister\n";
        return 1;
    }

    bool shouldRegister = strcmp(argv[1], "/register") == 0;

    // Get the path to our DLL
    WCHAR exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    std::wstring dllPathStr = exePath;
    // Remove the exe name and append our DLL name
    dllPathStr = dllPathStr.substr(0, dllPathStr.find_last_of(L"\\") + 1) + L"ZSTDShellExt.dll";

    std::wcout << L"Looking for DLL at: " << dllPathStr << std::endl;

    if (!std::filesystem::exists(dllPathStr)) {
        std::cout << "DLL file does not exist at the specified path!\n";
        return 1;
    }

    // Load the DLL
    std::cout << "Attempting to load DLL...\n";
    HMODULE hDll = LoadLibraryW(dllPathStr.c_str());
    if (!hDll) {
        DWORD error = GetLastError();
        std::cout << "Failed to load DLL. Error code: " << error << "\n";

        // Try to get more detailed error message
        LPVOID lpMsgBuf;
        FormatMessageW(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            error,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPWSTR)&lpMsgBuf,
            0, NULL);

        std::wcout << L"Error message: " << (LPWSTR)lpMsgBuf << std::endl;
        LocalFree(lpMsgBuf);
        return 1;
    }

    std::cout << "DLL loaded successfully.\n";

    // Get the registration/unregistration function
    typedef HRESULT (__stdcall *DllInstallFunction)();
    DllInstallFunction installFunc = (DllInstallFunction)GetProcAddress(hDll,
        shouldRegister ? "DllRegisterServer" : "DllUnregisterServer");

    if (!installFunc) {
        DWORD error = GetLastError();
        std::cout << "Failed to get function address. Error: " << error << "\n";
        FreeLibrary(hDll);
        return 1;
    }

    // Call the function
    std::cout << "Calling " << (shouldRegister ? "DllRegisterServer" : "DllUnregisterServer") << "...\n";
    HRESULT hr = installFunc();
    if (FAILED(hr)) {
        std::cout << "Failed to " << (shouldRegister ? "register" : "unregister")
                  << " server. HRESULT: 0x" << std::hex << hr << "\n";
        FreeLibrary(hDll);
        return 1;
    }

    std::cout << "Successfully " << (shouldRegister ? "registered" : "unregistered") 
              << " the shell extension.\n";

    FreeLibrary(hDll);
    return 0;
}