#include "core/commands.h"
#include "core/glog.h"
#include "managers/views/terminal_screen.h"
#include "sdkconfig.h"

#include "core/network_constants.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Help command
void handle_help(int argc, char **argv) {
    const char *category = (argc > 1) ? argv[1] : "unknown";

    const char *all_categories[] = {
        "wifi", "ble", "chameleon", "comm", "sd", "led", "gps", "shell", "misc", "portal", "printer", "cast", "gadgets", "capture", "beacon", "attack"
#ifdef CONFIG_HAS_LORA
        , "lora"
#endif
#ifdef CONFIG_HAS_INFRARED
        , "ir"
#endif
#ifdef CONFIG_HAS_CAMERA
        , "camera"
#endif
#ifdef CONFIG_WITH_ETHERNET
        , "ethernet"
#endif
    };
    int num_categories = sizeof(all_categories) / sizeof(all_categories[0]);

    if (strcmp(category, "all") == 0) {
        for (int i = 0; i < num_categories; ++i) {
            char *fake_argv[] = { "help", (char *)all_categories[i] };
            handle_help(2, fake_argv);
        }
        return;
    }

    if (strcmp(category, "misc") == 0) {
        glog("\nMiscellaneous Commands:\n\n");
        glog("help\n");
        glog("    Description: Display this help message.\n");
        glog("    Usage: help [category]\n\n");
        glog("calc\n");
        glog("    Description: Evaluate a simple built-in firmware calculator expression.\n");
        glog("    Usage: calc <expression>\n");
        glog("    Examples: calc 12 + 7 | calc 5 * (9 - 2) | calc 100 / 4\n\n");
        return;
    }

    glog("\nGhost ESP Command Categories:\n\n");
    glog("  help wifi      - Wi-Fi commands\n");
    glog("  help ble       - Bluetooth/BLE commands\n");
    glog("  help comm      - ESP32 communication commands\n");
    glog("  help sd        - SD card commands\n");
    glog("  help led       - LED/RGB commands\n");
    glog("  help gps       - GPS commands\n");
    glog("  help shell     - Headless shell commands\n");
    glog("  help misc      - Miscellaneous commands\n");
    glog("  help all       - All commands\n\n");
    glog("Type 'help <category>' for details on that category.\n\n");
}
