//
// Created by Victor on 16.07.2021.
//

#include <iostream>
#include "client.h"
#include <TCPClient.h>
#include <thread>
#include <codecvt>
#include <locale>

static const auto LogPrinter = [](const std::string& strLogMsg) { std::wcout << strLogMsg.data() << std::endl; };

static void (*callback)(std::wstring*);

static CTCPClient* client = nullptr;

static bool isConnected;

bool client_is_connected(){
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

void listenServer(){
    while(true){
        int size;
        int ret = client->Receive((char*) &size, sizeof(int));

        if(ret == 0){
            break;
        }

        if(size == -1){
            break;
        }

        auto* msg_utf8 = new std::string(size + 1, ' ');
        client->Receive(msg_utf8->data(),size);

        std::wstring* w_msg = utf8_to_wstring(msg_utf8);

        std::wcout << w_msg << std::endl;

        delete msg_utf8;

        callback(w_msg);
    }

    client->Disconnect();
}

void sendData_client(std::wstring* msg){
    if(isConnected){

        std::string* msg_utf8 = wstring_to_utf8(msg);

        int size = msg_utf8->size();

        client->Send((char*) &size, sizeof(int));
        client->Send((char*) msg_utf8->data());
    }

}

void disconnect_client(){
    if(isConnected){
        int msg = -1;

        client->Send((char*) &msg, sizeof(int));

        client->Disconnect();
    }
}

bool init_client(std::string ip, std::string port, void (*_callback)(std::wstring*)){
    callback = _callback;

    client = new CTCPClient(LogPrinter);

    isConnected = client->Connect(ip,port);

    if(!isConnected){

        delete client;

        return false;
    }

    std::thread(listenServer).detach();

    return true;
}