//
// Created by Victor on 16.07.2021.
//

#include <iostream>
#include "client.h"
#include <TCPClient.h>
#include <thread>
#include <codecvt>
#include <locale>

static const auto LogPrinter = [](const std::string& strLogMsg) { std::cout << strLogMsg << std::endl; };

static void (*callback)(std::string*);

static CTCPClient* client = nullptr;

static bool isConnected;
static bool isShutdown = false;

bool client_is_connected(){
    return isConnected;
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

        std::string *msg = nullptr;
        try {
            msg = new std::string(size + 1, ' ');
        }catch(std::exception& e){
            break;
        }

        client->Receive(msg->data(),size);

        callback(msg);
    }

    client->Disconnect();
}

void disconnect_client(){
    if(isConnected){
        int msg = -1;

        client->Send((char*) &msg, sizeof(int));

        client->Disconnect();
    }
}

void sendData_client(std::string* msg){
    if(isConnected){

        int size = msg->size();

        client->Send((char*) &size, sizeof(int));
        client->Send((char*) msg->data());
    }
}


bool init_client(std::string ip, std::string port, void (*_callback)(std::string*)){
    callback = _callback;

    client = new CTCPClient(LogPrinter);

    isConnected = client->Connect(ip,port);

    if(!isConnected){

        delete client;

        return false;
    }
    printf("connected\n");
    std::thread(listenServer).detach();

    return true;
}