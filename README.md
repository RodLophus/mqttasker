# mqttasker
A very simple and light daemon to run commands when MQTT messages are received

Upon start, this daemon will look for a file or directory on the location pointed by
"actions_path" on the configuration file (se "how to use" bellow).

* If "actions_path" is a file, the daemon will subscribe to all MQTT topics ("#") and call this (executable) file when it receives a message.

* If "actions_path" is a directory, the daemon will scan this directory and subscribe to MQTT topics corresponding to all files it find, and will run the corresponding (executable) file when a message is received.

In any case, the daemon will pass the MQTT topic and the message's payload to the called program via standard input (first the topic, them the message, on lines ended by new-lines).

Important: I wrote this program in a hurry just to meet some needs.  It problably has some bugs, so please use with care.  Also, I added lots of comments, so it will be ease to modify - even for those not so used to C programming.

### How to install

1. Install the dependencies:

* Mosquitto development libraries:\
  Linux (Debian-like): apt install libmosquitto-dev\
  FreeBSD: pkg install mosquitto

* Inih\
  Linux (Debian-like): apt install libinih-dev\
  FreeBSD: pkg install inih

2. Run "make"

3. Copy "mqttasker" to the desired directory (usually "/usr/local/sbin")


### How to use

1. Use "mqttasker.conf.sample" to create your configuration file (default path: /usr/local/etc/mqttasker/mqttasker.conf").  See the comments on the sample file for help.

2. Create a directory (or file) on the location pointed by "actions_path" (on the configuration file).

* If this path points to a file, it will be run when a MQTT request is received

* If this path points to a directory, MQTTasker will scan it for files and subscribe to topics named after the path (relative to "actions_path") of the files it finds, and will the respective program when a message is received.

For example: supposing you have:

```
actions_path = /etc/mqttasker/actions
```

on your configuration file, the a program at **/etc/mqttasker/actions/tasmota/rfbridge/tele/RESULT** will run when a message is received on the topic **tasmota/rfbridge/tele/RESULT** (run **mqttasker -d** to see the topics MQTTasker is subscribing to and the programs it is trying to call).

3. Create your "action programs".

For example:

```sh
#!/bin/sh
read topic
read message

echo Received message on topic $topic >> /tmp/mqqtasker.txt
echo The messages content is: $message >> /tmp/mqqtasker.txt
```

You can use "jq" to deal with messages with JSON payloads.

For example, the "action program" bellow could be used to turn tasmota-compatible smart plugs ON and OFF when a button pressed on a RF remote control is received by a tasmota-enabled Sonoff RF Bridge:

```sh
#!/bin/sh

read topic
rfcode=`/usr/local/bin/jq -r .RfReceived.Data`

mqtt_broker_username='my_username'
mqtt_broker_password='my_password'

mqtt_pub () {
  /usr/bin/mosquitto_pub \
    -h localhost
    -u $mqtt_broker_username \
    -P $mqtt_broker_password \
    -r -t $1 -m $2
}

case "$rfcode" in
  081CCB)
    mqtt_pub "tasmota/plug01/cmnd/POWER" "TOGGLE"
    ;;
  081C24)
    mqtt_pub "tasmota/plug02/cmnd/POWER" "TOGGLE"
    ;;
  082CDB)
    mqtt_pub "tasmota/plug03/cmnd/POWER" "TOGGLE"
    ;;
esac
```

### Limitations

* No SSL support for MQTT broker connection
* No log at all (use "mqttask -d" to debug on foreground)
* No Linux support files to enable the daemon to start automatically on boot (check the "extras" folder for the FreeBSD version)

