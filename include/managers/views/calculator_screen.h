#include "core/calculator.h"
#include "core/commands.h"
#include "core/glog.h"
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void calc_set_error(char *errbuf, size_t errbuf_len, const char *msg) {
    if (!errbuf || errbuf_len == 0) return;
    snprintf(errbuf, errbuf_len, "%s", msg ? msg : "Invalid expression");
}

static void calc_skip_ws(const char **cursor) {
    while (**cursor == ' ' || **cursor == '\t' || **cursor == '\n' || **cursor == '\r') {
        (*cursor)++;
    }
}

static bool calc_parse_number(const char **cursor, double *out_value, char *errbuf, size_t errbuf_len) {
    errno = 0;
    char *end = NULL;
    double value = strtod(*cursor, &end);
    if (end == *cursor) {
        calc_set_error(errbuf, errbuf_len, "Expected a number");
        return false;
    }
    if (errno == ERANGE) {
        calc_set_error(errbuf, errbuf_len, "Number out of range");
        return false;
    }
    *cursor = end;
    *out_value = value;
    return true;
}

static bool calc_parse_expression(const char **cursor, double *out_value, char *errbuf, size_t errbuf_len);

static bool calc_parse_function(const char **cursor, double *out_value, char *errbuf, size_t errbuf_len) {
    const char *start = *cursor;
    while (isalnum((unsigned char)**cursor) || **cursor == '_') {
        (*cursor)++;
    }
    size_t len = (size_t)(*cursor - start);
    if (len == 0) {
        calc_set_error(errbuf, errbuf_len, "Expected function name");
        return false;
    }

    char name[16];
    if (len >= sizeof(name)) {
        len = sizeof(name) - 1;
    }
    memcpy(name, start, len);
    name[len] = '\0';

    calc_skip_ws(cursor);
    if (**cursor != '(') {
        calc_set_error(errbuf, errbuf_len, "Expected '(' after function name");
        return false;
    }
    (*cursor)++;

    if (strcmp(name, "sqrt") == 0 || strcmp(name, "sqr") == 0 || strcmp(name, "square") == 0) {
        double arg = 0.0;
        if (!calc_parse_expression(cursor, &arg, errbuf, errbuf_len)) {
            return false;
        }
        calc_skip_ws(cursor);
        if (**cursor != ')') {
            calc_set_error(errbuf, errbuf_len, "Missing closing parenthesis");
            return false;
        }
        (*cursor)++;
        *out_value = (strcmp(name, "sqrt") == 0 || strcmp(name, "sqr") == 0 || strcmp(name, "square") == 0) ? (name[0] == 's' && name[1] == 'q' ? arg * arg : (strcmp(name, "sqrt") == 0 ? sqrt(arg) : arg * arg)) : 0.0;
        return true;
    }

    if (strcmp(name, "root") == 0) {
        double value = 0.0;
        double degree = 0.0;
        if (!calc_parse_expression(cursor, &value, errbuf, errbuf_len)) return false;
        calc_skip_ws(cursor);
        if (**cursor != ',') {
            calc_set_error(errbuf, errbuf_len, "Expected ',' in root()");
            return false;
        }
        (*cursor)++;
        if (!calc_parse_expression(cursor, &degree, errbuf, errbuf_len)) return false;
        calc_skip_ws(cursor);
        if (**cursor != ')') {
            calc_set_error(errbuf, errbuf_len, "Missing closing parenthesis");
            return false;
        }
        (*cursor)++;
        if (fabs(degree) < 1e-12) {
            calc_set_error(errbuf, errbuf_len, "Root degree cannot be zero");
            return false;
        }
        *out_value = pow(value, 1.0 / degree);
        return true;
    }

    if (strcmp(name, "abs") == 0) {
        double arg = 0.0;
        if (!calc_parse_expression(cursor, &arg, errbuf, errbuf_len)) return false;
        calc_skip_ws(cursor);
        if (**cursor != ')') {
            calc_set_error(errbuf, errbuf_len, "Missing closing parenthesis");
            return false;
        }
        (*cursor)++;
        *out_value = fabs(arg);
        return true;
    }

    if (strcmp(name, "ln") == 0 || strcmp(name, "log") == 0 || strcmp(name, "sin") == 0 || strcmp(name, "cos") == 0 || strcmp(name, "tan") == 0 || strcmp(name, "asin") == 0 || strcmp(name, "acos") == 0 || strcmp(name, "atan") == 0) {
        double arg = 0.0;
        if (!calc_parse_expression(cursor, &arg, errbuf, errbuf_len)) return false;
        calc_skip_ws(cursor);
        if (**cursor != ')') {
            calc_set_error(errbuf, errbuf_len, "Missing closing parenthesis");
            return false;
        }
        (*cursor)++;
        if (strcmp(name, "ln") == 0) *out_value = log(arg);
        else if (strcmp(name, "log") == 0) *out_value = log10(arg);
        else if (strcmp(name, "sin") == 0) *out_value = sin(arg);
        else if (strcmp(name, "cos") == 0) *out_value = cos(arg);
        else if (strcmp(name, "tan") == 0) *out_value = tan(arg);
        else if (strcmp(name, "asin") == 0) *out_value = asin(arg);
        else if (strcmp(name, "acos") == 0) *out_value = acos(arg);
        else *out_value = atan(arg);
        return true;
    }

    calc_set_error(errbuf, errbuf_len, "Unknown function");
    return false;
}

static bool calc_parse_primary(const char **cursor, double *out_value, char *errbuf, size_t errbuf_len) {
    calc_skip_ws(cursor);
    if (**cursor == '+') {
        (*cursor)++;
        return calc_parse_expression(cursor, out_value, errbuf, errbuf_len);
    }
    if (**cursor == '-') {
        (*cursor)++;
        double value = 0.0;
        if (!calc_parse_expression(cursor, &value, errbuf, errbuf_len)) {
            return false;
        }
        *out_value = -value;
        return true;
    }
    if (**cursor == '(') {
        (*cursor)++;
        if (!calc_parse_expression(cursor, out_value, errbuf, errbuf_len)) {
            return false;
        }
        calc_skip_ws(cursor);
        if (**cursor != ')') {
            calc_set_error(errbuf, errbuf_len, "Missing closing parenthesis");
            return false;
        }
        (*cursor)++;
        return true;
    }
    if (isalpha((unsigned char)**cursor)) {
        return calc_parse_function(cursor, out_value, errbuf, errbuf_len);
    }
    if (isdigit((unsigned char)**cursor) || **cursor == '.') {
        return calc_parse_number(cursor, out_value, errbuf, errbuf_len);
    }
    calc_set_error(errbuf, errbuf_len, "Invalid expression");
    return false;
}

static bool calc_parse_power(const char **cursor, double *out_value, char *errbuf, size_t errbuf_len) {
    double value = 0.0;
    if (!calc_parse_primary(cursor, &value, errbuf, errbuf_len)) {
        return false;
    }
    calc_skip_ws(cursor);
    if (**cursor == '^') {
        (*cursor)++;
        double exponent = 0.0;
        if (!calc_parse_power(cursor, &exponent, errbuf, errbuf_len)) {
            calc_set_error(errbuf, errbuf_len, "Missing exponent");
            return false;
        }
        value = pow(value, exponent);
    }
    *out_value = value;
    return true;
}

static bool calc_parse_term(const char **cursor, double *out_value, char *errbuf, size_t errbuf_len) {
    double value = 0.0;
    if (!calc_parse_power(cursor, &value, errbuf, errbuf_len)) {
        return false;
    }

    while (true) {
        calc_skip_ws(cursor);
        if (**cursor == '*') {
            (*cursor)++;
            double rhs = 0.0;
            if (!calc_parse_power(cursor, &rhs, errbuf, errbuf_len)) {
                calc_set_error(errbuf, errbuf_len, "Missing right-hand operand");
                return false;
            }
            value *= rhs;
        } else if (**cursor == '/') {
            (*cursor)++;
            double rhs = 0.0;
            if (!calc_parse_power(cursor, &rhs, errbuf, errbuf_len)) {
                calc_set_error(errbuf, errbuf_len, "Missing right-hand operand");
                return false;
            }
            if (fabs(rhs) < 1e-12) {
                calc_set_error(errbuf, errbuf_len, "Division by zero");
                return false;
            }
            value /= rhs;
        } else if (**cursor == '%') {
            (*cursor)++;
            double rhs = 0.0;
            if (!calc_parse_power(cursor, &rhs, errbuf, errbuf_len)) {
                calc_set_error(errbuf, errbuf_len, "Missing right-hand operand");
                return false;
            }
            if (fabs(rhs) < 1e-12) {
                calc_set_error(errbuf, errbuf_len, "Modulo by zero");
                return false;
            }
            value = fmod(value, rhs);
        } else {
            break;
        }
    }

    *out_value = value;
    return true;
}

static bool calc_parse_expression(const char **cursor, double *out_value, char *errbuf, size_t errbuf_len) {
    double value = 0.0;
    if (!calc_parse_term(cursor, &value, errbuf, errbuf_len)) {
        return false;
    }

    while (true) {
        calc_skip_ws(cursor);
        if (**cursor == '+') {
            (*cursor)++;
            double rhs = 0.0;
            if (!calc_parse_term(cursor, &rhs, errbuf, errbuf_len)) {
                calc_set_error(errbuf, errbuf_len, "Missing right-hand operand");
                return false;
            }
            value += rhs;
        } else if (**cursor == '-') {
            (*cursor)++;
            double rhs = 0.0;
            if (!calc_parse_term(cursor, &rhs, errbuf, errbuf_len)) {
                calc_set_error(errbuf, errbuf_len, "Missing right-hand operand");
                return false;
            }
            value -= rhs;
        } else if (**cursor == '%') {
            (*cursor)++;
            value *= 0.01;
        } else {
            break;
        }
    }

    *out_value = value;
    return true;
}

bool ghostesp_calculator_eval(const char *expr, double *out_value, char *errbuf, size_t errbuf_len) {
    if (!expr || !out_value || !errbuf || errbuf_len == 0) {
        return false;
    }

    errbuf[0] = '\0';
    const char *cursor = expr;
    calc_skip_ws(&cursor);
    if (*cursor == '\0') {
        calc_set_error(errbuf, errbuf_len, "Empty expression");
        return false;
    }

    double value = 0.0;
    if (!calc_parse_expression(&cursor, &value, errbuf, errbuf_len)) {
        return false;
    }

    calc_skip_ws(&cursor);
    if (*cursor != '\0') {
        char extra[64];
        snprintf(extra, sizeof(extra), "Unexpected token '%c'", *cursor);
        calc_set_error(errbuf, errbuf_len, extra);
        return false;
    }

    *out_value = value;
    return true;
}

void ghostesp_calculator_format(double value, char *buf, size_t len) {
    if (!buf || len == 0) {
        return;
    }
    if (!isfinite(value)) {
        snprintf(buf, len, "Error");
        return;
    }

    if (fabs(value) < 1e-9) {
        value = 0.0;
    }
    double rounded = round(value * 1000000.0) / 1000000.0;
    if (fabs(rounded - round(rounded)) < 1e-9) {
        snprintf(buf, len, "%.0f", rounded);
        return;
    }

    char tmp[64];
    snprintf(tmp, sizeof(tmp), "%.6f", rounded);
    size_t end = strlen(tmp);
    while (end > 0 && tmp[end - 1] == '0') {
        tmp[--end] = '\0';
    }
    if (end > 0 && tmp[end - 1] == '.') {
        tmp[--end] = '\0';
    }
    snprintf(buf, len, "%s", tmp);
}

const char *ghostesp_calculator_history_path(void) {
    return "/sdcard/GhostESP_calculator_history.txt";
}

static bool ghostesp_calculator_history_open(FILE **out_file, const char *mode) {
    const char *path = ghostesp_calculator_history_path();
    if (!path || !out_file) return false;
    *out_file = fopen(path, mode);
    return *out_file != NULL;
}

bool ghostesp_calculator_append_history(const char *expr, double value) {
    if (!expr || !*expr) {
        return false;
    }
    FILE *f = NULL;
    if (!ghostesp_calculator_history_open(&f, "a")) {
        return false;
    }

    char formatted[64];
    ghostesp_calculator_format(value, formatted, sizeof(formatted));
    fprintf(f, "%s = %s\n", expr, formatted);
    fclose(f);
    return true;
}

size_t ghostesp_calculator_history_count(void) {
    FILE *f = NULL;
    if (!ghostesp_calculator_history_open(&f, "r")) {
        return 0;
    }

    size_t count = 0;
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (line[0] != '\0') {
            count++;
        }
    }
    fclose(f);
    return count;
}

bool ghostesp_calculator_load_history(char *buf, size_t len, size_t index) {
    if (!buf || len == 0) return false;
    buf[0] = '\0';

    FILE *f = NULL;
    if (!ghostesp_calculator_history_open(&f, "r")) {
        return false;
    }

    size_t current = 0;
    char line[256];
    bool found = false;
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '\0' || line[0] == '\n') {
            continue;
        }
        if (current == index) {
            size_t keep = strlen(line);
            while (keep > 0 && (line[keep - 1] == '\n' || line[keep - 1] == '\r')) {
                line[--keep] = '\0';
            }
            snprintf(buf, len, "%s", line);
            found = true;
            break;
        }
        current++;
    }
    fclose(f);
    return found;
}

void handle_calc_cmd(int argc, char **argv) {
    if (argc <= 1 || strcmp(argv[1], "help") == 0 || strcmp(argv[1], "--help") == 0) {
        glog("Calculator\n");
        glog("Usage: calc <expression>\n");
        glog("Examples:\n");
        glog("  calc 12 + 7\n");
        glog("  calc 5 * (9 - 2)\n");
        glog("  calc sqrt(81)\n");
        glog("  calc square(5)\n");
        glog("  calc root(64, 3)\n");
        glog("  calc 25%\n");
        return;
    }

    if (strcmp(argv[1], "history") == 0) {
        size_t count = ghostesp_calculator_history_count();
        glog("History entries: %u\n", (unsigned)count);
        for (size_t i = 0; i < count; ++i) {
            char line[256];
            if (ghostesp_calculator_load_history(line, sizeof(line), i)) {
                glog("%u: %s\n", (unsigned)i, line);
            }
        }
        return;
    }

    if (strcmp(argv[1], "clear") == 0) {
        remove(ghostesp_calculator_history_path());
        glog("Calculator history cleared.\n");
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
    ghostesp_calculator_append_history(expr, value);
}
