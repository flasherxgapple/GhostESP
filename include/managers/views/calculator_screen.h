#include "core/calculator.h"
#include "core/commands.h"
#include "core/glog.h"
#include <stdio.h>
#include <string.h>

static void calc_print_help(void) {
    glog("Calculator\n");
    glog("Usage: calc <expression>\n");
    glog("Examples:\n");
    glog("  calc 12 + 7\n");
    glog("  calc 5 * (9 - 2)\n");
    glog("  calc 100 / 4\n");
    glog("  calc help\n");
}

void handle_calc_cmd(int argc, char **argv) {
    if (argc <= 1 || strcmp(argv[1], "help") == 0) {
        calc_print_help();
        return;
    }

    char expr[256] = {0};
    size_t pos = 0;
    for (int i = 1; i < argc && pos < sizeof(expr) - 1; ++i) {
        if (i > 1 && pos < sizeof(expr) - 1) {
            expr[pos++] = ' ';
        }
        size_t len = strlen(argv[i]);
        if (pos + len >= sizeof(expr)) {
            len = sizeof(expr) - pos - 1;
        }
        memcpy(expr + pos, argv[i], len);
        pos += len;
    }
    expr[pos] = '\0';

    double value = 0.0;
    char errbuf[128];
    if (!ghostesp_calculator_eval(expr, &value, errbuf, sizeof(errbuf))) {
        glog("calc: %s\n", errbuf);
        return;
    }

    char output[64];
    ghostesp_calculator_format(value, output, sizeof(output));
    glog("= %s\n", output);
}
