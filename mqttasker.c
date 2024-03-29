/*****************************************************************************\
 * MQTTasker - Run external executables when a MQTT message is received      *
 * Copyright(c) 2022, Rodolfo Broco Manin                                    *
\*****************************************************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <mosquitto.h>
#include "globals.h"
#include "config.h"


config_t config;
struct mosquitto *mqtt;
char **topics;
int topic_count = 0;
int debug = 0;
int catch_all_mode = 0;


/*****************************************************************************\
 *                                 usage()                                   *
 * Shows command line usage help.                                            *
\*****************************************************************************/
void usage() {
    printf("\n%s v.%s\n\n"
        "Syntax: %s [-h] [-d] [-c configuration_file]\n\n"
        "  -h : Shows this help information\n"
        "  -c : Specifies the location of the configuration file\n"
        "       (default: %s)\n"
        "  -d : Debug mode (runs in foreground and shows messages on stderr)\n\n",
        PROGNAME, VERSION, PROGNAME, DEFAULT_CONFIG_FILE);
}


/*****************************************************************************\
 *                             get_file_mode()                               *
 * Gets the mode and type of a file                                          *
\*****************************************************************************/
int get_file_mode(char *path) {
    struct stat filestat;
    if(! stat(path, &filestat)) {
        return filestat.st_mode;
    }
    return -1;
}


/*****************************************************************************\
 *                               mqtt_on_message()                           *
 * Callback called when a message is received on one of the subscribed       *
 * topics.                                                                   *
 * Runs the suitable action executable, passing the topic name and the       *
 * message's payload via stdin (separated by newlines)                       *
\*****************************************************************************/
void mqtt_on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message) {
    char *action_file;
    int action_file_mode;
    FILE *cmd;
    if(message) {
        LOG("\n- Topic: %s\n  Message: %s\n", message->topic, (char *)message->payload);
        if(catch_all_mode) {
            action_file = topics[0];
        } else {
            action_file = malloc(sizeof(config.actions_path) + sizeof(message->topic) + 1);
            sprintf(action_file, "%s/%s", config.actions_path, message->topic);
        }
        if((action_file_mode = get_file_mode(action_file)) == -1) {
            LOG("  Unable to access file: %s\n", action_file);
        } else {
            if(action_file_mode & 0100) {
                LOG("  Executing: %s\n", action_file);
                if((cmd = popen(action_file, "w"))) {
                    fprintf(cmd, "%s\n%s\n", message->topic, (char *)message->payload);
                    pclose(cmd);
                }
            } else {
                LOG("  Ignoring non-executable file: %s\n", action_file);
            }
        }
        if(! catch_all_mode) {
            free(action_file);
        }
    }
}


/*****************************************************************************\
 *                             mqtt_on_connect()                             *
 * Callback called when the client connects to the MQTT broker.              *
 * Subscribes the client on MQTT topics.                                     *
\*****************************************************************************/
void mqtt_on_connect(struct mosquitto *mosq, void *obj, int rc) {
    int i;
    if(rc != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "Error #%d connecting to MQTT Broker (incorrect username / password?).\n", rc);
        exit(1);
    }
    if(catch_all_mode) {
        // A single action program will process messages from all MQTT topics
        LOG("Subscribing to all MQTT topics\n");
        mosquitto_subscribe(mqtt, NULL, "#", 1);
    } else {
        // One action program by message topic
        LOG("Subscribing to MQTT topics:\n");
        for(i = 0; i < topic_count; i++) {
            LOG("- %s\n", topics[i]);
            mosquitto_subscribe(mqtt, NULL, topics[i], 1);
        }
    }
    LOG("\nRead to process publications.\n");
}


/*****************************************************************************\
 *                                 mqtt_init()                               *
 * Connects to the broker and initializes the MQTT client.                   *
\*****************************************************************************/
int mqtt_init() {
    mosquitto_lib_init();
    mqtt = mosquitto_new(NULL, true, NULL);
    if((strlen(config.mqtt_username) > 0) && (strlen(config.mqtt_password) > 0)) {
        mosquitto_username_pw_set(mqtt, config.mqtt_username, config.mqtt_password);
    }
    mosquitto_message_callback_set(mqtt, mqtt_on_message);
    mosquitto_connect_callback_set(mqtt, mqtt_on_connect);
    if(mosquitto_connect(mqtt, config.mqtt_address, config.mqtt_port, 60) != MOSQ_ERR_SUCCESS) {
        return -1;
    }
    return 0;
}


/*****************************************************************************\
 *                            init_topics_list()                             *
 * Initializes the global array 'topics' with a list of topics we are        *
 * subscribing  to.                                                          *
 * The list contains all files found under 'basepath' and its                *
 * subdirectories.   The file's paths are relative to 'basepath'             *
\*****************************************************************************/
void init_topics_list(char *basepath, char *relpath) {
    char workpath[PATH_MAX];
    char tmppath[PATH_MAX];
    DIR *dir;
    int action_path_mode;
    int i = 0;
    struct dirent *dirent;

    if(topic_count == 0) {
        // The list is empty: do initial allocation
        topics = malloc(sizeof(char *));
        if((action_path_mode = get_file_mode(basepath)) == -1) {
            fprintf(stderr, "Unable to access %s\n", basepath);
            exit(1);
        } else if(action_path_mode & S_IFREG) {
            // config's 'action_path' is a regular file: this file will proccess all
            // MQTT messages (this is the "catch-all mode")
            topics[0] = basepath;
            topic_count = 1;
            catch_all_mode = 1;
            return;
        }
    }

    snprintf(workpath, sizeof(workpath), "%s/%s", basepath, relpath);
    if((dir = opendir(workpath))) {
        while((dirent = readdir(dir))) {
            // Ignores '.', '..', hidden files
            // and files which have MQTT wildcards ("+" and "#") on their names
            if((dirent->d_name[0] == '.') || strchr(dirent->d_name, '+') || strchr(dirent->d_name, '#')) {
                continue;
            }
            // Executable's path, relactive to 'basepath'
            snprintf(tmppath, sizeof(tmppath), "%s/%s", relpath, dirent->d_name);
            switch(dirent->d_type) {
                case DT_DIR:
                    // Entry is a directory: descend recursively
                    init_topics_list(basepath, tmppath);
                    break;
                case DT_REG:
                    // Entry is a file
                    // Excludes "/"s from the path's begin (if any)
                    for(i = 0; tmppath[i] == '/'; i++);
                    // Inserts the new topic on the list
                    if((topics = realloc(topics, ++topic_count * sizeof(char *)))) {
                        topics[topic_count - 1] = malloc(strlen(&tmppath[i]) + 1);
                        strcpy(topics[topic_count - 1], &tmppath[i]);
                    } else {
                        perror("Memory allocation error");
                        exit(1);
                    }
            }
        }
        closedir(dir);
    }
}


/*****************************************************************************\
 *                                 main()                                    *
 * Main function.                                                            *
\*****************************************************************************/
int main(int argc, char *argv[]) {
    char *cfgfile = DEFAULT_CONFIG_FILE;
    FILE *pid_file;
    int opt, pid;

    while((opt = getopt(argc, argv, "hdc:")) != -1) {
        switch(opt) {
            case 'h':
                usage();
                return 0;
            case 'd':
                debug = 1;
                break;
            case 'c':
                cfgfile = optarg;
                break;
            default:
                printf("Use \"" PROGNAME " -h\" for help on command line options.\n");
                return 1;
        }
    }

    LOG("\nConfiguration file: %s\n", cfgfile);

    if(config_load(cfgfile, &config)) {
        fprintf(stderr, "Error loading the configuration file:\n");
        perror(cfgfile);
        return 1;
    }

    init_topics_list(config.actions_path, "");

    if(topic_count == 0) {
        fprintf(stderr, "No MQTT topics to watch.  Aborting.\n");
        return 1;
    }

    // Don't care about the return code of child processes
    // (prevents executor processes from becoming "defunct" when finishes)
    signal(SIGCHLD, SIG_IGN);

    if(mqtt_init()) {
        perror("Unable to connect to the MQTT broker");
        return 1;
    }

    if(! debug) {
        pid = fork();
        if(pid > 0) {
            if((pid_file = fopen(config.pid_file, "w"))) {
                fprintf(pid_file, "%d\n", pid);
                fclose(pid_file);
                return 0;
            }
        }
    }

    mosquitto_loop_forever(mqtt, -1, 1);

    return 0;
}
