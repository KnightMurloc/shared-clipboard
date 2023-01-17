//
// Created by Victor on 16.07.2021.
//

#include <iostream>
#include "client.h"
#include <TCPClient.h>
#include <thread>
#include <map>

static const auto LogPrinter = [](const std::string& strLogMsg) { std::cout << strLogMsg << std::endl; };

static void (*callback)(const std::map<std::string, std::vector<char>>& data);

static CTCPClient* client = nullptr;

static bool isConnected;
static bool isShutdown = false;

bool client_is_connected(){
    return isConnected;
}

void listenServer(){
    while(true){
        int data_size;
        int ret = client->Receive((char*) &data_size, sizeof(int));

        if(ret == 0 || ret == -1){
            break;
        }

        std::map<std::string, std::vector<char>> data;
        for(int i = 0; i < data_size; i++){
            int size;
            client->Receive((char*) &size,sizeof(int));
            std::string name(size,'\0');
            client->Receive(name.data(),size);

            client->Receive((char*) &size,sizeof(int));

            std::vector<char> vec;
            vec.resize(size);
            client->Receive(vec.data(),size);

            data.insert(std::make_pair(std::move(name),std::move(vec)));
        }
        callback(data);
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

void sendData_client(const std::map<std::string, std::vector<char>>& data){
    if(isConnected){

        int data_size = data.size();

        client->Send((char*) &data_size, sizeof(data_size));

        for(const auto& entry : data){
            int size = entry.first.size();

            client->Send((char*) &size, sizeof(size));
            client->Send((char*) entry.first.data(),size);

            size = entry.second.size();
            client->Send((char*) &size, sizeof(size));
            client->Send(entry.second.data(), size);
        }
    }
}


bool init_client(std::string ip, std::string port, void (*_callback)(const std::map<std::string, std::vector<char>>& data)){
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