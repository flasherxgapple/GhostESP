#include "core/calculator.h"
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    const char *cursor;
    const char *expr;
    char *errbuf;
    size_t errbuf_len;
} calc_parser_t;

static void calc_set_error(calc_parser_t *p, const char *fmt, const char *detail) {
    if (!p || !p->errbuf || p->errbuf_len == 0) return;
    if (detail) {
        snprintf(p->errbuf, p->errbuf_len, fmt, detail);
    } else {
        snprintf(p->errbuf, p->errbuf_len, "%s", fmt);
    }
}

static void calc_skip_ws(calc_parser_t *p) {
    while (*p->cursor == ' ' || *p->cursor == '\t' || *p->cursor == '\n' || *p->cursor == '\r') {
        p->cursor++;
    }
}

static bool calc_parse_number(calc_parser_t *p, double *out) {
    char *end = NULL;
    errno = 0;
    double value = strtod(p->cursor, &end);
    if (end == p->cursor) {
        calc_set_error(p, "Expected a number", NULL);
        return false;
    }

    if (errno == ERANGE) {
        calc_set_error(p, "Number out of range", NULL);
        return false;
    }

    p->cursor = end;
    *out = value;
    return true;
}

static bool calc_parse_factor(calc_parser_t *p, double *out);

static bool calc_parse_primary(calc_parser_t *p, double *out) {
    calc_skip_ws(p);
    if (*p->cursor == '+') {
        p->cursor++;
        return calc_parse_factor(p, out);
    }
    if (*p->cursor == '-') {
        p->cursor++;
        if (!calc_parse_factor(p, out)) {
            return false;
        }
        *out = -*out;
        return true;
    }
    if (*p->cursor == '(') {
        p->cursor++;
        if (!calc_parse_factor(p, out)) {
            return false;
        }
        calc_skip_ws(p);
        if (*p->cursor != ')') {
            calc_set_error(p, "Missing closing parenthesis", NULL);
            return false;
        }
        p->cursor++;
        return true;
    }
    if (isdigit((unsigned char)*p->cursor) || *p->cursor == '.') {
        return calc_parse_number(p, out);
    }
    calc_set_error(p, "Invalid expression", NULL);
    return false;
}

static bool calc_parse_factor(calc_parser_t *p, double *out) {
    double value = 0.0;
    if (!calc_parse_primary(p, &value)) {
        return false;
    }

    calc_skip_ws(p);
    while (*p->cursor == '^') {
        p->cursor++;
        double exponent = 0.0;
        if (!calc_parse_primary(p, &exponent)) {
            calc_set_error(p, "Missing exponent", NULL);
            return false;
        }
        value = pow(value, exponent);
        calc_skip_ws(p);
    }

    *out = value;
    return true;
}

static bool calc_parse_term(calc_parser_t *p, double *out) {
    double value = 0.0;
    if (!calc_parse_factor(p, &value)) {
        return false;
    }

    while (true) {
        calc_skip_ws(p);
        if (*p->cursor == '*') {
            p->cursor++;
            double rhs = 0.0;
            if (!calc_parse_factor(p, &rhs)) {
                calc_set_error(p, "Missing right-hand operand", NULL);
                return false;
            }
            value *= rhs;
        } else if (*p->cursor == '/') {
            p->cursor++;
            double rhs = 0.0;
            if (!calc_parse_factor(p, &rhs)) {
                calc_set_error(p, "Missing right-hand operand", NULL);
                return false;
            }
            if (fabs(rhs) < 1e-12) {
                calc_set_error(p, "Division by zero", NULL);
                return false;
            }
            value /= rhs;
        } else if (*p->cursor == '%') {
            p->cursor++;
            double rhs = 0.0;
            if (!calc_parse_factor(p, &rhs)) {
                calc_set_error(p, "Missing right-hand operand", NULL);
                return false;
            }
            if (fabs(rhs) < 1e-12) {
                calc_set_error(p, "Modulo by zero", NULL);
                return false;
            }
            value = fmod(value, rhs);
        } else {
            break;
        }
    }

    *out = value;
    return true;
}

static bool calc_parse_expression(calc_parser_t *p, double *out) {
    double value = 0.0;
    if (!calc_parse_term(p, &value)) {
        return false;
    }

    while (true) {
        calc_skip_ws(p);
        if (*p->cursor == '+') {
            p->cursor++;
            double rhs = 0.0;
            if (!calc_parse_term(p, &rhs)) {
                calc_set_error(p, "Missing right-hand operand", NULL);
                return false;
            }
            value += rhs;
        } else if (*p->cursor == '-') {
            p->cursor++;
            double rhs = 0.0;
            if (!calc_parse_term(p, &rhs)) {
                calc_set_error(p, "Missing right-hand operand", NULL);
                return false;
            }
            value -= rhs;
        } else {
            break;
        }
    }

    *out = value;
    return true;
}

bool ghostesp_calculator_eval(const char *expr, double *out_value, char *errbuf, size_t errbuf_len) {
    if (!expr || !out_value || !errbuf || errbuf_len == 0) {
        return false;
    }

    calc_parser_t parser = { .cursor = expr, .expr = expr, .errbuf = errbuf, .errbuf_len = errbuf_len };
    errbuf[0] = '\0';

    calc_skip_ws(&parser);
    if (*parser.cursor == '\0') {
        calc_set_error(&parser, "Empty expression", NULL);
        return false;
    }

    double result = 0.0;
    if (!calc_parse_expression(&parser, &result)) {
        return false;
    }

    calc_skip_ws(&parser);
    if (*parser.cursor != '\0') {
        char extra[32];
        snprintf(extra, sizeof(extra), "Unexpected token '%c'", *parser.cursor);
        calc_set_error(&parser, extra, NULL);
        return false;
    }

    *out_value = result;
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
