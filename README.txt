shared-clipboard
shared clipboard between PC.

warning: the data is currently being transmitted without encryption.

linux dependencies:
  gtk-3
  libnotify (optional)

default config path:
  linux: /home/${USER}/.config/shared clipboard/config.json
  windows: in same folder with executable

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
