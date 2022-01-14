# mqttasker
A very simple and light daemon to run commands when MQTT messages are received

When it starts, this daemon will scan the directory specified by "actions_path" (se "how to use" bellow) for files and use the dicovered files full path (relative to "actions_path") as topic names for MQTT subscriptions.

Once a message is received on one of the subscribed topics, it will run the respective program, passing the MQTT topic and the message's payload via standard input (first the topic, them the message, on lines ended by new-lines).

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

2. Create a directory for the "actions programs" (default: /usr/local/etc/mqttasker/actions).  Those will be executed when a MQTT message is received.

The "action program" full path (relative to the "action_path" configuration parameter) is the topic it will be executed upon.

For example: supposing you have:

```
action_path = /etc/mqttasker/actions
```

on your configuration file, the a program at **/etc/mqttasker/actions/tasmota/rfbridge/tele/RESULT** will run when a message is received on the topic **tasmota/rfbridge/tele/RESULT**

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

mqtt_pub () {
  /usr/bin/mosquitto_pub \
    -h mqtt_broker_address \
    -u mqtt_broker_username \
    -P mqtt_broker_password \
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
* No support for wildcards on MQTT topics
* No log at all (use "mqttask -d" to debug on foreground)
* No Linux support files to enable the daemon to start automatically on boot (check the "extras" folder for the FreeBSD version)

