//
// Created by Victor on 15.07.2021.
//

#include "server.h"
#include <TCPServer.h>
#include <Socket.h>
#include <thread>
#include <iostream>
#include <map>

static auto LogPrinter = [](const std::string& strLogMsg) { std::cout << strLogMsg << std::endl; };

static void (*callback)(const std::map<std::string, std::vector<char>>& data);

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
        int data_size;
        int ret = server->Receive(client, (char*) &data_size, sizeof(int));

        if(ret == 0 || ret == -1){
            break;
        }

        std::map<std::string, std::vector<char>> data;
        for(int i = 0; i < data_size; i++){
            int size;
            server->Receive(client, (char*) &size,sizeof(int));
            std::string name(size,'\0');
            server->Receive(client, name.data(),size);

            server->Receive(client, (char*) &size,sizeof(int));

            std::vector<char> vec;
            vec.resize(size);
            server->Receive(client, vec.data(),size);

            data.insert(std::make_pair(std::move(name),std::move(vec)));
        }

        callback(data);
    }

    if(!isShutDown){
        listenClients();
    }
    int msg = -1;

    server->Send(client, (char*) &msg, sizeof(int));

    server->Disconnect(client);
}

void sendData_server(const std::map<std::string, std::vector<char>>& data){
    if(isConnected){

        int data_size = data.size();

        server->Send(client, (char*) &data_size, sizeof(data_size));

        for(const auto& entry : data){
            int size = entry.first.size();
            std::cout << entry.first << std::endl;
            server->Send(client, (char*) &size, sizeof(size));
            server->Send(client, (char*) entry.first.data(), size);

            size = entry.second.size();

            server->Send(client, (char*) &size, sizeof(size));
            server->Send(client, entry.second.data(), size);
        }
    }
}

bool init_server(std::string port, void (*_callback)(const std::map<std::string, std::vector<char>>& data)){

    server = new CTCPServer(LogPrinter,port);

    std::thread(listenClients).detach();

    callback = _callback;

    return true;
}