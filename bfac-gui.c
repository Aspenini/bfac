// bfac-gui.c — GUI wrapper for BFAC: Brainfuck Advanced Compiler
// Build: tcc -mwindows -o bfac-gui.exe bfac-gui.c

#include <windows.h>
#include <stdio.h>

// Define missing constants for file dialogs
#ifndef OFN_PATHMUSTEXIST
#define OFN_PATHMUSTEXIST     0x00000800
#define OFN_FILEMUSTEXIST     0x00001000
#define OFN_OVERWRITEPROMPT   0x00000002
#endif

// Define OPENFILENAME structure if not available
#ifndef OPENFILENAME_SIZE_VERSION_400A
typedef struct tagOFNA {
    DWORD        lStructSize;
    HWND         hwndOwner;
    HINSTANCE    hInstance;
    LPCSTR       lpstrFilter;
    LPSTR        lpstrCustomFilter;
    DWORD        nMaxCustFilter;
    DWORD        nFilterIndex;
    LPSTR        lpstrFile;
    DWORD        nMaxFile;
    LPSTR        lpstrFileTitle;
    DWORD        nMaxFileTitle;
    LPCSTR       lpstrInitialDir;
    LPCSTR       lpstrTitle;
    DWORD        Flags;
    WORD         nFileOffset;
    WORD         nFileExtension;
    LPCSTR       lpstrDefExt;
    LPARAM       lCustData;
    LPVOID       lpfnHook;
    LPCSTR       lpTemplateName;
} OPENFILENAMEA;
#endif

// External function declarations
extern BOOL WINAPI GetOpenFileNameA(OPENFILENAMEA*);
extern BOOL WINAPI GetSaveFileNameA(OPENFILENAMEA*);

#define ID_BUTTON_BROWSE    1001
#define ID_BUTTON_COMPILE   1002
#define ID_BUTTON_RUN       1003
#define ID_EDIT_INPUT       1004
#define ID_EDIT_OUTPUT      1005
#define ID_EDIT_LOG         1006

HWND hEdit_Input, hEdit_Output, hEdit_Log, hButton_Run;
HINSTANCE hInst;

// Function to append text to log
void LogMessage(const char* message) {
    int len = GetWindowTextLength(hEdit_Log);
    SendMessage(hEdit_Log, EM_SETSEL, len, len);
    SendMessage(hEdit_Log, EM_REPLACESEL, FALSE, (LPARAM)message);
    SendMessage(hEdit_Log, EM_REPLACESEL, FALSE, (LPARAM)"\r\n");
}

// Function to run compiler
void CompileFile() {
    char input_file[MAX_PATH], output_file[MAX_PATH];
    char compiler_path[MAX_PATH], cmd[MAX_PATH * 3];
    
    // Get input and output file paths
    GetWindowText(hEdit_Input, input_file, MAX_PATH);
    GetWindowText(hEdit_Output, output_file, MAX_PATH);
    
    if (strlen(input_file) == 0) {
        LogMessage("Error: Please select an input .bf file");
        return;
    }
    
    if (strlen(output_file) == 0) {
        LogMessage("Error: Please specify output .exe file");
        return;
    }
    
    // Get directory where GUI exe is located
    char exe_dir[MAX_PATH];
    GetModuleFileName(NULL, exe_dir, MAX_PATH);
    char *last_slash = strrchr(exe_dir, '\\');
    if (last_slash) *last_slash = 0;
    
    // Build path to bfac.exe (should be in same directory)
    snprintf(compiler_path, sizeof(compiler_path), "%s\\bfac.exe", exe_dir);
    
    // Check if compiler exists
    if (GetFileAttributes(compiler_path) == INVALID_FILE_ATTRIBUTES) {
        LogMessage("Error: bfac.exe not found in same directory as GUI");
        return;
    }
    
    LogMessage("Starting compilation...");
    LogMessage("Input: ");
    LogMessage(input_file);
    LogMessage("Output: ");
    LogMessage(output_file);
    
    // Build command line
    snprintf(cmd, sizeof(cmd), "\"%s\" \"%s\" \"%s\"", compiler_path, input_file, output_file);
    
    // Create process to run compiler
    STARTUPINFO si = {sizeof(si)};
    PROCESS_INFORMATION pi;
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    
    if (CreateProcess(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        LogMessage("Compiler started...");
        
        // Wait for process to complete
        WaitForSingleObject(pi.hProcess, INFINITE);
        
        DWORD exit_code;
        GetExitCodeProcess(pi.hProcess, &exit_code);
        
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        
        if (exit_code == 0) {
            LogMessage("SUCCESS! Compilation completed.");
            LogMessage("Native Windows PE executable created!");
            EnableWindow(hButton_Run, TRUE);
        } else {
            LogMessage("ERROR: Compilation failed.");
            EnableWindow(hButton_Run, FALSE);
        }
    } else {
        LogMessage("ERROR: Failed to start compiler process.");
        EnableWindow(hButton_Run, FALSE);
    }
}

// Function to run compiled executable
void RunExecutable() {
    char output_file[MAX_PATH];
    GetWindowText(hEdit_Output, output_file, MAX_PATH);
    
    if (strlen(output_file) == 0) {
        LogMessage("Error: No executable to run");
        return;
    }
    
    if (GetFileAttributes(output_file) == INVALID_FILE_ATTRIBUTES) {
        LogMessage("Error: Executable file not found");
        return;
    }
    
    LogMessage("Running executable...");
    
    // Run the executable in a new console window
    char cmd[MAX_PATH + 50];
    snprintf(cmd, sizeof(cmd), "cmd /c \"\"%s\" & pause\"", output_file);
    
    STARTUPINFO si = {sizeof(si)};
    PROCESS_INFORMATION pi;
    
    if (CreateProcess(NULL, cmd, NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi)) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        LogMessage("Executable started in new console window.");
    } else {
        LogMessage("ERROR: Failed to run executable.");
    }
}

// File browser dialog
void BrowseForFile(HWND hEdit, BOOL bOpen) {
    OPENFILENAMEA ofn;
    char file[MAX_PATH] = "";
    
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = GetParent(hEdit);
    ofn.lpstrFile = file;
    ofn.nMaxFile = sizeof(file);
    
    if (bOpen) {
        ofn.lpstrFilter = "Brainfuck Files (*.bf)\0*.bf\0All Files (*.*)\0*.*\0";
        ofn.lpstrTitle = "Select Brainfuck Source File";
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
        
        if (GetOpenFileNameA(&ofn)) {
            SetWindowText(hEdit, file);
            
            // Auto-generate output filename
            char output[MAX_PATH];
            strcpy(output, file);
            char *dot = strrchr(output, '.');
            if (dot) {
                strcpy(dot, ".exe");
                SetWindowText(hEdit_Output, output);
            }
        }
    } else {
        ofn.lpstrFilter = "Executable Files (*.exe)\0*.exe\0All Files (*.*)\0*.*\0";
        ofn.lpstrTitle = "Save Executable As";
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
        ofn.lpstrDefExt = "exe";
        
        if (GetSaveFileNameA(&ofn)) {
            SetWindowText(hEdit, file);
        }
    }
}

// Window procedure
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE:
            // Create controls
            CreateWindow("STATIC", "Input .bf file:", WS_VISIBLE | WS_CHILD,
                10, 10, 100, 20, hwnd, NULL, hInst, NULL);
            
            hEdit_Input = CreateWindow("EDIT", "", WS_VISIBLE | WS_CHILD | WS_BORDER,
                10, 35, 300, 25, hwnd, (HMENU)ID_EDIT_INPUT, hInst, NULL);
            
            CreateWindow("BUTTON", "Browse...", WS_VISIBLE | WS_CHILD,
                320, 35, 80, 25, hwnd, (HMENU)ID_BUTTON_BROWSE, hInst, NULL);
            
            CreateWindow("STATIC", "Output .exe file:", WS_VISIBLE | WS_CHILD,
                10, 70, 100, 20, hwnd, NULL, hInst, NULL);
            
            hEdit_Output = CreateWindow("EDIT", "", WS_VISIBLE | WS_CHILD | WS_BORDER,
                10, 95, 300, 25, hwnd, (HMENU)ID_EDIT_OUTPUT, hInst, NULL);
            
            CreateWindow("BUTTON", "Compile!", WS_VISIBLE | WS_CHILD,
                10, 130, 100, 35, hwnd, (HMENU)ID_BUTTON_COMPILE, hInst, NULL);
            
            hButton_Run = CreateWindow("BUTTON", "Run", WS_VISIBLE | WS_CHILD,
                120, 130, 80, 35, hwnd, (HMENU)ID_BUTTON_RUN, hInst, NULL);
            EnableWindow(hButton_Run, FALSE);
            
            CreateWindow("STATIC", "Log:", WS_VISIBLE | WS_CHILD,
                10, 180, 50, 20, hwnd, NULL, hInst, NULL);
            
            hEdit_Log = CreateWindow("EDIT", "", WS_VISIBLE | WS_CHILD | WS_BORDER | WS_VSCROLL | ES_MULTILINE | ES_READONLY,
                10, 205, 400, 150, hwnd, (HMENU)ID_EDIT_LOG, hInst, NULL);
            
            LogMessage("BFAC - Brainfuck Advanced Compiler GUI v1.0");
            LogMessage("Select a .bf file and click Compile to create native Windows executable!");
            break;
            
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case ID_BUTTON_BROWSE:
                    BrowseForFile(hEdit_Input, TRUE);
                    break;
                    
                case ID_BUTTON_COMPILE:
                    EnableWindow(hButton_Run, FALSE);
                    CompileFile();
                    break;
                    
                case ID_BUTTON_RUN:
                    RunExecutable();
                    break;
            }
            break;
            
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
            
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    hInst = hInstance;
    
    const char* CLASS_NAME = "BF2PE_GUI";
    
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    
    RegisterClass(&wc);
    
    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        "BFAC - Brainfuck Advanced Compiler",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 450, 420,
        NULL, NULL, hInstance, NULL
    );
    
    if (hwnd == NULL) {
        return 0;
    }
    
    ShowWindow(hwnd, nCmdShow);
    
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return 0;
}
