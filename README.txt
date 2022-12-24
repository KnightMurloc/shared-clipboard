shared-clipboard
shared clipboard between PC.

warning: the data is currently being transmitted without encryption.

linux dependencies:
  Xlib
  Xfixes
  gio-2

default config path:
  linux: /home/${USER}/.config/shared clipboard/config.json
  windows: C:\Users\${USER}\AppData\Roaming\s_clipboard\config.json

config file example (current support only one client):
{
  "port" : "8888",
  "clients" : [
    {
      "name" : "laptop",
      "ip" : "192.168.0.105",
      "port" : "8888"
    }
  ]
}
