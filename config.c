#include <stdlib.h>
#include <string.h>
#include <ini.h>
#include "globals.h"
#include "config.h"


extern int debug;


/*****************************************************************************\
 *                               config_handler()                            *
 * Callback called to process the parameters in the configuration file.      *
\*****************************************************************************/
static int config_handler(void *user, const char *section, const char *param, const char *value) {
    config_t *cfg = (config_t *)user;

    if(! strcmp(section, "mqtt")) {
        if(! strcmp(param, "address")) strncpy(cfg->mqtt_address, value, sizeof(cfg->mqtt_address));
        if(! strcmp(param, "port")) cfg->mqtt_port = atoi(value);
        if(! strcmp(param, "username")) strncpy(cfg->mqtt_username, value, sizeof(cfg->mqtt_username));
        if(! strcmp(param, "password")) strncpy(cfg->mqtt_password, value, sizeof(cfg->mqtt_password));
    }

    if(! strcmp(section, "main")) {
        if(! strcmp(param, "actions_path")) strncpy(cfg->actions_path, value, sizeof(cfg->actions_path));
        if(! strcmp(param, "pid_file")) strncpy(cfg->pid_file, value, sizeof(cfg->pid_file));
    }

    return 0;
}


/*****************************************************************************\
 *                               config_load()                               *
 * Initializes the global variable 'config' with the parameters from the     *
 * configuration file.                                                       *
\*****************************************************************************/
int config_load(char *filename, config_t *config) {
    int retcode;
    strcpy(config->mqtt_address, DEFAULT_MQTT_ADDRESS);
    config->mqtt_port = DEFAULT_MQTT_PORT;
    strcpy(config->mqtt_username, DEFAULT_MQTT_USERNAME);
    strcpy(config->mqtt_password, DEFAULT_MQTT_PASSWORD);
    strcpy(config->actions_path, DEFAULT_ACTIONS_PATH);
    strcpy(config->pid_file, DEFAULT_PID_FILE);

    retcode = (ini_parse(filename, config_handler, config) < 0);

    LOG("\nGeneral Parameters:\n"
        "- PID file: %s\n"
        "- Actions directory (or file): %s\n"
        "\nMQTT Broker:\n"
        "- Address: %s:%d\n"
        "- Username: %s\n"
        "- Password: %s\n\n", config->pid_file, config->actions_path,
        config->mqtt_address, config->mqtt_port, config->mqtt_username, config->mqtt_password);

    return retcode;
}
