#ifndef _CONFIG_H
#define _CONFIG_H

#include <limits.h>
#include "globals.h"

#define DEFAULT_MQTT_ADDRESS "localhost"
#define DEFAULT_MQTT_PORT   1883
#define DEFAULT_MQTT_USERNAME ""
#define DEFAULT_MQTT_PASSWORD ""
#define DEFAULT_ACTIONS_PATH DEFAULT_CONFIG_PATH "/actions"
#define DEFAULT_PID_FILE "/var/run/" PROGNAME ".pid"

typedef struct {
    char mqtt_address[256];
    int mqtt_port;
    char mqtt_username[64];
    char mqtt_password[64];
    char actions_path[PATH_MAX];
    char pid_file[PATH_MAX];
} config_t;

int config_load(char *, config_t *);

#endif
