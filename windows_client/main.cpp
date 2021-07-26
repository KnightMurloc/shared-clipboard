#define WINVER 0x0600
#define NTDDI_VERSION 0x06010000

#include <iostream>
#include <windows.h>
#include <tchar.h>
#include <winuser.h>
#include "server.h"
#include "client.h"
#include <signal.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <vector>
#include <shellapi.h>
#include <commctrl.h>
#include <strsafe.h>
#include <unistd.h>
#include <exception>
#include <cstdlib>

UINT const WMAPP_NOTIFYCALLBACK = WM_APP + 1;
UINT const WMAPP_HIDEFLYOUT     = WM_APP + 2;

UINT_PTR const HIDEFLYOUT_TIMER_ID = 1;

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

void read_config();

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

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

    read_config();

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

    HWND hWnd = CreateWindowEx(0, szWindowClass, _T("s_clipboard"),
                               0, 0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, NULL);

    if (!hWnd)
    {
        MessageBox(NULL,
                   _T("Call to CreateWindow failed!"),
                   _T(""),
                   0);

        return 1;
    }

    SetClipboardViewer(hWnd);

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

void read_config(){
    try {
        ifstream stream("config.json");

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
        wcout << hData << endl;

        void* tmp = GlobalLock(hData);

        char* pszText = static_cast<char*>(tmp);

        text = new std::wstring((wchar_t*) pszText);

        GlobalUnlock(hData);

    }
    CloseClipboard();
    return text;
}

void setClipboard(wstring* msg){
    isSet = true;
    OpenClipboard(nullptr);
    EmptyClipboard();
    HGLOBAL clipbuffer = GlobalAlloc(GMEM_DDESHARE, (msg->size()*2)+1);
    auto* buffer = (wchar_t *)GlobalLock(clipbuffer);

    memcpy(buffer,msg->data(),msg->size()*2);
    GlobalUnlock(clipbuffer);

    SetClipboardData(CF_UNICODETEXT,clipbuffer);

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

        wcout << msg->data() << endl;

        sendData(msg);
    }else{
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}
