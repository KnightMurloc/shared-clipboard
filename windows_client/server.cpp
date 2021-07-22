//
// Created by Victor on 15.07.2021.
//

#include "server.h"
#include <TCPServer.h>
#include <Socket.h>
#include <thread>
#include <iostream>
#include <codecvt>
#include <locale>

static const auto LogPrinter = [](const std::string& strLogMsg) { std::wcout << strLogMsg.data() << std::endl; };

static void (*callback)(std::wstring*);

static CTCPServer* server = nullptr;
static ASocket::Socket client;
static bool isConnected = false;
static bool isShutdown = false;

bool server_is_connected(){
    return isConnected;
}

static std::wstring* utf8_to_wstring (const std::string* str)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
    return new std::wstring(myconv.from_bytes(str->data()));
}

static std::string* wstring_to_utf8 (const std::wstring* str)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
    return new std::string(myconv.to_bytes(str->data()));
}

void disconnect_server(){
    if(isConnected){
        int msg = -1;

        server->Send(client, (char*) &msg, sizeof(int));

        server->Disconnect(client);
        isConnected = false;
        isShutdown = true;
    }
}

void listenClients(){
    server->Listen(client);

    isConnected = true;

    printf("\nconnected\n");

    while(true){
        int size;

        int ret = server->Receive(client,(char*) &size, sizeof(int));

		if(ret == 0){
			break;
		}

        if(size == -1){
            break;
        }

//        std::string msg(size + 1,' ');
        auto* msg = new std::string(size + 1, ' ');
        server->Receive(client,(char*) msg->data(),size);

        std::wstring* w_msg = utf8_to_wstring(msg);

        std::wcout << w_msg << std::endl;

        callback(w_msg);

        delete msg;
    }
    if(!isShutdown){
        listenClients();
    }

}

void sendData_server(std::wstring* msg){
    if(isConnected){

        std::string* msg_utf8 = wstring_to_utf8(msg);

        int size = msg_utf8->size();

        server->Send(client,(char*) &size, sizeof(int));
        server->Send(client,(char*) msg_utf8->data());
    }

}

bool init_server(std::string port, void (*_callback)(std::wstring*)){

    server = new CTCPServer(LogPrinter,port);

    std::thread(listenClients).detach();

    callback = _callback;

    return true;
}