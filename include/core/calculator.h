#ifndef CORE_CALCULATOR_H
#define CORE_CALCULATOR_H

#include <stdbool.h>
#include <stddef.h>

bool ghostesp_calculator_eval(const char *expr, double *out_value, char *errbuf, size_t errbuf_len);
void ghostesp_calculator_format(double value, char *buf, size_t len);
bool ghostesp_calculator_append_history(const char *expr, double value);
bool ghostesp_calculator_load_history(char *buf, size_t len, size_t index);
size_t ghostesp_calculator_history_count(void);
const char *ghostesp_calculator_history_path(void);

#endif
