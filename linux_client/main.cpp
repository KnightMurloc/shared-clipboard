#include <gdk/gdk.h>
#include <gtk/gtk.h>
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
#include <libnotify/notify.h>

#define NOTIFY_MSG_LEN 256

using namespace nlohmann;

typedef struct {
    std::string name;
    std::string ip;
    std::string port;
}Client;

GdkClipboard *clipboard;
bool isSet = false;
bool wait_lock = false;

std::string port;
std::vector<Client> clients;

void sendData(std::string* msg);

void signalHandle(int sig){

    std::cout << "disconnect" << std::endl;

    disconnect_client();
    disconnect_server();

    exit(sig);
}

void wait_text(GdkClipboard* clipboard, gpointer userdata){
    GdkContentFormats* format = gdk_clipboard_get_formats(clipboard);

    const GType* type = gdk_content_formats_get_gtypes(format, nullptr); 

    if(type == nullptr || type[0] != G_TYPE_STRING){
        return;
    }

    if(isSet){
        isSet = false;
        return;
    }

    if(wait_lock){
        wait_lock = false;
        return;
    }
    wait_lock = true;

    gdk_clipboard_read_text_async(clipboard, nullptr, [](GObject* source_object, GAsyncResult* res, gpointer user_data){
            GError* err = nullptr;
            char* msg = gdk_clipboard_read_text_finish((GdkClipboard*) source_object, res,&err);

            if(msg == nullptr){
                return;
            }

            std::cout << msg << std::endl;
            auto* s_msg = new std::string(msg);
            sendData(s_msg);
            }, nullptr);

}

gboolean set_clipboard(std::string* msg){
    isSet = true;

    std::cout << g_utf8_validate(msg->c_str(),-1, nullptr) << msg->c_str() << std::endl;
    gdk_clipboard_set_text(clipboard, msg->c_str());

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
    gtk_init();
    GMainLoop* loop = g_main_loop_new (NULL, FALSE);

    GdkDisplay* display = gdk_display_get_default();

    clipboard = gdk_display_get_clipboard(display);
//    gdk_display_get_primary_clipboard()


    init_client(clients[0].ip,clients[0].port,callback);
    init_server(port, callback);

    g_signal_connect(clipboard,"changed",G_CALLBACK(wait_text),NULL);

//    set_clipboard(new std::string("test"));
//    g_idle_add(G_SOURCE_FUNC(set_clipboard),new std::string("test"));

    g_main_loop_run(loop);

    return 0;
}
