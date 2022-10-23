#include <string>
#include <thread>
#include <iostream>
#include "server.h"
#include "client.h"
#include <csignal>
#include <nlohmann/json.hpp>
#include <vector>
#include <pwd.h>
#include <fstream>
#ifdef NOTIFY
#include <gio/gio.h>
#endif
#include "clipboard.h"

#define NOTIFY_MSG_LEN 256

using namespace nlohmann;

typedef struct {
    std::string name;
    std::string ip;
    std::string port;
}Client;

std::string port;
std::vector<Client> clients;
#ifdef NOTIFY
GApplication* app;
#endif
void sendData(std::string& msg);

void signalHandle(int sig){

    std::cout << "disconnect" << std::endl;

    disconnect_client();
    disconnect_server();

    stop_loop();

}

void wait_text(char* text){
    if(text == nullptr){
        return;
    }

    std::cout << text << std::endl;
    std::string s_msg(text);
    sendData(s_msg);
    free(text);
}

bool set_clipboard(std::string& msg){
    put_to_clipboard(msg.c_str());

#ifdef NOTIFY
    GNotification* notify = g_notification_new (clients[0].name.c_str());
    if(msg.length() > NOTIFY_MSG_LEN){
        std::string short_msg(NOTIFY_MSG_LEN, '.');
        memcpy(short_msg.data(),msg.data(),NOTIFY_MSG_LEN - 3);
        g_notification_set_body(notify,short_msg.c_str());
    }else{
        g_notification_set_body(notify,msg.c_str());
    }

    g_application_send_notification(app,"firefox",notify);
#endif
    return false;
}

void load_config(std::string file){
    try {
        std::ifstream stream(file);
        json config;

        stream >> config;

        port = config["port"];

        json clients_json = config["clients"];

        for(json& client_json : clients_json){
            Client client;

            client.name = client_json["name"];
            client.ip = client_json["ip"];
            client.port = client_json["port"];

            clients.push_back(client);
        }
    }catch(std::exception& e){
        std::cout << "incorrect configurate file" << std::endl;
        exit(-1);
    }

}

void sendData(std::string& msg){
    if(client_is_connected()){
        sendData_client(msg);
    }

    if(server_is_connected()){
        sendData_server(msg);
    }
}

void callback(std::string msg){
    set_clipboard(msg);
}

std::string get_default_path(){
    const char* home = getenv("HOME");
    if(home == nullptr){
        home = getpwuid(getuid())->pw_dir;
    }

    std::string result = home + std::string("/.config/shared clipboard/config.json");

    std::cout << result << std::endl;

    return result;
}

int main(int argc, char** argv)
{
    std::string config_file = get_default_path();
    if(argc >= 3){
        if(std::string("--config") == argv[1]){
            config_file = std::string(argv[2]);
        }
    }

    load_config(config_file);

    signal(SIGTERM,signalHandle);
	signal(SIGINT,signalHandle);

#ifdef NOTIFY
    app = g_application_new("org.txt.test", G_APPLICATION_IS_SERVICE);
    g_application_register(app,NULL,NULL);
#endif

    init_client(clients[0].ip,clients[0].port,callback);
    init_server(port, callback);

    clip_board_init();

    clipboard_notify_loop(wait_text);

    std::cout << "exit" << std::endl;

#ifdef NOTIFY
    g_object_unref(app);
#endif
    return 0;
}
