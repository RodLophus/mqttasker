#ifndef _GLOBALS_H
#define _GLOBALS_H

#include "version.h"

#define PROGNAME "mqttasker"

#define DEFAULT_CONFIG_PATH "/usr/local/etc/" PROGNAME

#define DEFAULT_CONFIG_FILE DEFAULT_CONFIG_PATH "/" PROGNAME ".conf"

#define LOG(...) if(debug) fprintf(stderr, __VA_ARGS__)

#endif
