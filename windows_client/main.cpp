#define WINVER 0x0600
#define NTDDI_VERSION 0x06010000

#include <iostream>
#include <windows.h>
#include <tchar.h>
#include <winuser.h>
#include "server.h"
#include "client.h"
#include <csignal>
#include <nlohmann/json.hpp>
#include <fstream>
#include <vector>
#include <shellapi.h>
#include <commctrl.h>
#include <strsafe.h>
//#include <unistd.h>
#include <exception>
#include <cstdlib>
#include <future>
#include <shlobj.h>


using namespace std;
using namespace nlohmann;

typedef struct {
    string name;
    string ip;
    string port;
}Client;

static TCHAR szWindowClass[] = _T("DesktopApp");
static string port;
static vector<Client> clients;

void read_config(wstring& path);
void show_notify(std::wstring* msg);
wstring get_default_config_path();
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
HWND hwnd;

bool isSet = false;

void setClipboard(wstring* msg);

void signalHandler(int  signum ) {
    disconnect_client();
    disconnect_server();

    exit(0);
}

int CALLBACK WinMain(
        _In_ HINSTANCE hInstance,
        _In_opt_ HINSTANCE hPrevInstance,
        _In_ LPSTR     lpCmdLine,
        _In_ int       nCmdShow
){

    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);



    setlocale(LC_ALL, "");

    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, IDI_APPLICATION);
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = NULL;
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, IDI_APPLICATION);

    try {


        if (!RegisterClassEx(&wcex)) {
            MessageBox(NULL,
                       _T("Call to RegisterClassEx failed!"),
                       _T(""),
                       0);

            return 1;
        }
    }catch(exception& e){
        wcout << e.what() << endl;
    }

    hwnd = CreateWindowEx(0, szWindowClass, _T("s_clipboard"),
                               0, 0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, NULL);

    if (!hwnd)
    {
        MessageBox(NULL,
                   _T("Call to CreateWindow failed!"),
                   _T(""),
                   0);

        return 1;
    }

    wstring config = get_default_config_path();

    read_config(config);

    wcout << config << endl;

    SetClipboardViewer(hwnd);

    init_client(clients[0].ip,clients[0].port,setClipboard);
    init_server(port, setClipboard);
    MSG msg;
    // Main message loop:
    try {
        while (GetMessage(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }catch(exception& e){
        wcout << e.what() << endl;
    }
    return (int) msg.wParam;
}

void show_notify(std::wstring* msg){
    std::async([](HWND hwnd, std::wstring* msg){

        NOTIFYICONDATAW icon = { };
        memset(&icon,0, sizeof(icon));
        std::wstring clientTmp(clients[0].name.begin(), clients[0].name.end());

        icon.cbSize = sizeof(icon);
        icon.hWnd = hwnd;
        icon.uVersion = NOTIFYICON_VERSION_4;
        icon.uCallbackMessage = WM_USER;
        icon.uFlags = NIF_MESSAGE | NIF_INFO;
        Shell_NotifyIconW(NIM_ADD, &icon);
        
        if(msg->length() > sizeof(icon.szInfo) / sizeof(WCHAR)){
            memcpy(icon.szInfo,msg->data(), sizeof(icon.szInfo));
            size_t last = sizeof(icon.szInfo) / sizeof(WCHAR) - 1;
            icon.szInfo[last] = L'.';
            icon.szInfo[last - 1] = L'.';
            icon.szInfo[last - 2] = L'.';
        }else{
            memcpy(icon.szInfo,msg->data(),msg->size() * 2 + 1);
        }

        if(clientTmp.length() > sizeof(icon.szInfoTitle) / sizeof(WCHAR)){
            memcpy(icon.szInfoTitle,clientTmp.data(), sizeof(icon.szInfoTitle));
            size_t last = sizeof(icon.szInfoTitle) / sizeof(WCHAR) - 1;
            icon.szInfoTitle[last] = L'.';
            icon.szInfoTitle[last - 1] = L'.';
            icon.szInfoTitle[last - 2] = L'.';
        }else{
            memcpy(icon.szInfoTitle,clientTmp.data(),clientTmp.size() * 2 + 1);
        }

        Shell_NotifyIconW( NIM_MODIFY, &icon );

        Sleep(3000);

        Shell_NotifyIconW(NIM_DELETE, &icon);
    },hwnd, msg);
}

wstring get_default_config_path(){
    wstring appDataPath(MAX_PATH, '\0');

    SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, appDataPath.data());

    wcout << MAX_PATH << endl;

    wstring tmp = L"\\s_clipboard\\config.json";

    memcpy(appDataPath.data() + wcslen(appDataPath.data()),tmp.data(), (size_t) tmp.size() * 2);

    return appDataPath;
}

void read_config(wstring& path){
    try {

        ifstream stream(path);

        json config;

        stream >> config;

        port = config["port"];

        wcout << port.data() << endl;

        json clients_json = config["clients"];

        for (json &client_json : clients_json) {

            Client client;

            client.name = client_json["name"];
            wcout << client.name.data() << endl;

            client.ip = client_json["ip"];

            wcout << client.ip.data() << endl;

            client.port = client_json["port"];

            wcout << client.port.data() << endl;

            clients.push_back(client);
        }
    }catch(exception& e){
        wcout << e.what() << endl;
    }
}

std::wstring* GetClipboardText()
{
    wstring* text = nullptr;
    OpenClipboard(nullptr);
    if(OpenClipboard(nullptr)) { ;
        HANDLE hData = GetClipboardData(CF_UNICODETEXT);
        if(hData == nullptr){
            CloseClipboard();
            return nullptr;
        }

        void* tmp = GlobalLock(hData);
        if(tmp == nullptr){
            goto end;
        }
        char* pszText = static_cast<char*>(tmp);

        text = new std::wstring((wchar_t*) pszText);

        GlobalUnlock(hData);

    }
    end:
    CloseClipboard();
    return text;
}

void setClipboard(wstring* msg){
    show_notify(msg);

    if(OpenClipboard(nullptr)) {
        isSet = true;

        EmptyClipboard();
        HGLOBAL clipbuffer = GlobalAlloc(GMEM_DDESHARE, (msg->size() * 2) + 1);
        auto* buffer = (wchar_t*) GlobalLock(clipbuffer);
        if(buffer == nullptr){
            goto end;
        }
        memcpy(buffer, msg->data(), msg->size() * 2);
        GlobalUnlock(clipbuffer);

        SetClipboardData(CF_UNICODETEXT, clipbuffer);
    }
    end:
    CloseClipboard();

    delete msg;
}

void sendData(std::wstring* msg){

    if(msg == nullptr){
        return;
    }

    if(client_is_connected()){
        sendData_client(msg);
    }

    if(server_is_connected()){
        sendData_server(msg);
    }

    delete msg;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{

    if(message == WM_DRAWCLIPBOARD){
        wcout << L"test" << endl;
        if(isSet){
            isSet = false;
            return 0;
        }

        wstring* msg = GetClipboardText();

        sendData(msg);
    }else{
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}
