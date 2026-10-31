// commands.h
// Shared declarations for CLI command handlers that live outside commandline.c.

#ifndef COMMANDS_H
#define COMMANDS_H

#include "sdkconfig.h"
#include <stdbool.h>
#include "core/commandline.h"
#include "core/shell.h"

#ifdef __cplusplus
extern "C" {
#endif

void handle_calc_cmd(int argc, char **argv);

#ifdef __cplusplus
}
#endif

#endif // COMMANDS_H
