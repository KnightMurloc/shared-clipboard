#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <thread>
#include <iostream>
#include "server.h"
#include "client.h"
#include <csignal>
#include <nlohmann/json.hpp>
#include <vector>
#include <pwd.h>
#include <fstream>
#include <libnotify/notify.h>

#define NOTIFY_MSG_LEN 256

using namespace nlohmann;

typedef struct {
    std::string name;
    std::string ip;
    std::string port;
}Client;

GtkClipboard *clipboard;
bool isSet = false;

std::string port;
std::vector<Client> clients;

void sendData(std::string* msg);

void signalHandle(int sig){

    std::cout << "disconnect" << std::endl;

    disconnect_client();
    disconnect_server();

    exit(sig);
}

void wait_text(){

    if(isSet){
        isSet = false;
        return;
    }

    gchar* msg = gtk_clipboard_wait_for_text(clipboard);
    printf("%s\n",msg);

    auto* s_msg = new std::string(msg);

    sendData(s_msg);
}
gboolean set_clipboard(std::string* msg){

    isSet = true;
    gtk_clipboard_set_text(clipboard, msg->data(), -1);

#ifdef NOTIFY
    NotifyNotification *notify;

    if(msg->length() > NOTIFY_MSG_LEN){
        std::string short_msg(NOTIFY_MSG_LEN, '.');
        memcpy(short_msg.data(),msg->data(),NOTIFY_MSG_LEN - 3);
        notify = notify_notification_new(clients[0].name.data(), short_msg.data(), "clipit-trayicon");
    }else{
        notify = notify_notification_new(clients[0].name.data(), msg->data(),"clipit-trayicon");
    }

    GError* err = nullptr;

    notify_notification_show(notify,&err);

    g_object_unref(notify);
#endif

    delete msg;

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

void sendData(std::string* msg){
    if(client_is_connected()){
        sendData_client(msg);
    }

    if(server_is_connected()){
        sendData_server(msg);
    }

    delete msg;
}

void callback(std::string* msg){
    g_idle_add(G_SOURCE_FUNC(set_clipboard),msg);
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

    notify_init("shared clipboard");
    gtk_init(&argc, &argv);

    clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);

    init_client(clients[0].ip,clients[0].port,callback);
    init_server(port, callback);

    g_signal_connect(clipboard,"owner-change",wait_text,NULL);

    gtk_main();

    return 0;
}
