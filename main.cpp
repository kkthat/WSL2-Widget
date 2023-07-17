#include <windows.h>
#include <string>

// Resource ID for the custom icon
#define IDI_CUSTOM_ICON 101

// Custom message IDs for handling notifications
#define WM_TRAYICON (WM_USER + 1)
#define WM_BUTTON1 (WM_USER + 2)
#define WM_BUTTON2 (WM_USER + 3)
#define WM_BUTTON_CLOSE (WM_USER + 4)

// Global variable for the notification icon data
NOTIFYICONDATA nid = {0};

// Function prototypes
LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
std::string ExecuteCommand(const char* command);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // Create hidden window
    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "HiddenWindowClass";

    RegisterClassEx(&wc);

    HWND hWnd = CreateWindowEx(0, "HiddenWindowClass", "", 0, 0, 0, 0, 0, NULL, NULL, hInstance, NULL);
    if (hWnd == NULL)
    {
        MessageBox(NULL, "Failed to create hidden window.", "Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    // Create notification icon
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hWnd;
    nid.uID = 1; // Unique ID for the icon
    nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
    nid.uCallbackMessage = WM_TRAYICON; // Custom message ID for handling notifications

    // Retrieve the application's icon
    TCHAR szExePath[MAX_PATH];
    GetModuleFileName(NULL, szExePath, MAX_PATH);
    HICON hAppIcon = ExtractIcon(hInstance, szExePath, 0);

    if (hAppIcon != NULL) {
        nid.hIcon = hAppIcon;
    } else {
        nid.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CUSTOM_ICON)); // Load your custom icon resource here
    }

    strcpy_s(nid.szTip, sizeof(nid.szTip), "WSL Status");


    if (!Shell_NotifyIcon(NIM_ADD, &nid))
    {
        MessageBox(NULL, "Failed to add notification icon.", "Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    // Run message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Cleanup and exit
    Shell_NotifyIcon(NIM_DELETE, &nid);
    return 0;
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_TRAYICON:
        {
            if (LOWORD(lParam) == WM_RBUTTONDOWN)
            {
                // Create the popup menu when right-clicked on the notification icon
                HMENU hPopupMenu = CreatePopupMenu();

                std::string output = ExecuteCommand("wsl --list --running");
                    if (output.find("T") != std::string::npos)
                        AppendMenu(hPopupMenu, MF_STRING, WM_BUTTON1, "WSL Stopped");
                    else
                        AppendMenu(hPopupMenu, MF_STRING, WM_BUTTON1, "WSL Running");

                AppendMenu(hPopupMenu, MF_STRING, WM_BUTTON2, "Shutdown WSL");
                AppendMenu(hPopupMenu, MF_STRING, WM_BUTTON_CLOSE, "Close");

                POINT pt;
                GetCursorPos(&pt);
                SetForegroundWindow(hWnd);
                TrackPopupMenu(hPopupMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hWnd, NULL);

                // Cleanup the menu
                DestroyMenu(hPopupMenu);
            }
            break;
        }

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                
                case WM_BUTTON1:
                {   
                    // this code runs "wsl --list --running" and gets the output but for whatever reason when running that command it only gets the first letter
                    // of the output so hence we are only looking for the first letter. super strange tho if i used ping as the command and left everything else
                    // the same it worked fine and for the life of me i couldnt figure out why :/

                    std::string output = ExecuteCommand("wsl --list --running");
                    if (output.find("T") != std::string::npos)
                        MessageBox(hWnd, "WSL2 Is Currently Stopped", "WSL Status", MB_OK | MB_ICONINFORMATION);
                    else if (output.find("w") != std::string::npos)
                        MessageBox(hWnd, "WSL2 Is Currently Running", "WSL Status", MB_OK | MB_ICONINFORMATION);
                    else
                        MessageBox(hWnd, "WSL2 State Unknown", "WSL Status", MB_OK | MB_ICONINFORMATION);
                    break;
                }

                case WM_BUTTON2:
                {
                    int result = MessageBox(hWnd, "Are you sure you want to shutdown WSL?", "Confirmation", MB_YESNO | MB_ICONQUESTION);
                    if (result == IDYES)
                    {
                        STARTUPINFO si = {0};
                        si.cb = sizeof(STARTUPINFO);
                        PROCESS_INFORMATION pi;

                        // Create the command "wsl --shutdown"
                        std::string command = "wsl --shutdown";

                        // Create the process silently without showing the command prompt window
                        if (CreateProcess(NULL, const_cast<char*>(command.c_str()), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
                        {
                            // Wait for the process to exit
                            WaitForSingleObject(pi.hProcess, INFINITE);

                            // Close process and thread handles
                            CloseHandle(pi.hProcess);
                            CloseHandle(pi.hThread);
                        }
                        else
                        {
                            MessageBox(hWnd, "Failed to execute command.", "Error", MB_OK | MB_ICONERROR);
                        }
                    }
                    break;
                }

                case WM_BUTTON_CLOSE:
                {
                    int result = MessageBox(hWnd, "Are you sure you want to close the program?", "Confirmation", MB_YESNO | MB_ICONQUESTION);
                    if (result == IDYES)
                    {
                        PostMessage(hWnd, WM_CLOSE, 0, 0);
                    }
                    break;
                }
            }
            break;
        }

        case WM_DESTROY:
            // Cleanup and exit the program
            Shell_NotifyIcon(NIM_DELETE, &nid);
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    return 0;
}

std::string ExecuteCommand(const char* command)
{
    std::string output;

    // Create pipes for the command output
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = nullptr;

    HANDLE hReadPipe, hWritePipe;
    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0))
    {
        return output;
    }

    // Create the process with a hidden console window
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(STARTUPINFOA));
    ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
    si.cb = sizeof(STARTUPINFOA);
    si.dwFlags |= STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.hStdOutput = hWritePipe;
    si.hStdError = hWritePipe;
    si.wShowWindow = SW_HIDE;

    BOOL createProcessResult = CreateProcessA(nullptr, const_cast<LPSTR>(command), nullptr, nullptr, TRUE, 0, nullptr, nullptr, &si, &pi);
    if (createProcessResult)
    {
        // Close the write end of the pipe since we don't need it
        CloseHandle(hWritePipe);

        // Read the command output from the read end of the pipe
        const int bufferSize = 4096;
        char buffer[bufferSize];
        DWORD bytesRead;
        while (ReadFile(hReadPipe, buffer, bufferSize, &bytesRead, nullptr) && bytesRead != 0)
        {
            output.append(buffer, bytesRead);
        }

        // Close the read end of the pipe
        CloseHandle(hReadPipe);

        // Wait for the process to exit and get its exit code
        WaitForSingleObject(pi.hProcess, INFINITE);
        DWORD exitCode;
        GetExitCodeProcess(pi.hProcess, &exitCode);

        // Close the process and thread handles
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }

    return output;
}
