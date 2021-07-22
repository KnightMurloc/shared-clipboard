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

static auto LogPrinter = [](const std::string& strLogMsg) { std::cout << strLogMsg << std::endl; };

static void (*callback)(std::string*);

static CTCPServer* server = nullptr;
static ASocket::Socket client;
static bool isConnected = false;
static bool isShutDown = false;

bool server_is_connected(){
    return isConnected;
}

void disconnect_server(){
    if(isConnected){
        int msg = -1;

        server->Send(client, (char*) &msg, sizeof(int));
        isShutDown = true;
        isConnected = false;
    }
}

void listenClients(){
    server->Listen(client);

    isConnected = true;

    printf("\nconnected\n");

    while(true){
        int size;

        int ret = server->Receive(client,(char*) &size, sizeof(int));
//        std::cout << ret << std::endl;
        if(size == -1 || ret == 0){
            break;
        }

//        std::string msg(size + 1,' ');
        auto* msg = new std::string(size + 1,' ');
        server->Receive(client,(char*) msg->data(),size);


        callback(msg);
    }

    if(!isShutDown){
        listenClients();
    }
    int msg = -1;

    server->Send(client, (char*) &msg, sizeof(int));

    server->Disconnect(client);
//    int msg = -1;
//
//    server->Send(client, (char*) &msg, sizeof(int));
//
//    server->Disconnect(client);
}

void sendData_server(std::string* msg){
    if(isConnected){

        int size = msg->size();

        server->Send(client,(char*) &size, sizeof(int));
        server->Send(client,(char*) msg->data());
    }
}

bool init_server(std::string port, void (*_callback)(std::string*)){

    server = new CTCPServer(LogPrinter,port);

    std::thread(listenClients).detach();

    callback = _callback;

    return true;
}