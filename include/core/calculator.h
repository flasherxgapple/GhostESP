#ifndef CORE_CALCULATOR_H
#define CORE_CALCULATOR_H

#include <stdbool.h>
#include <stddef.h>

bool ghostesp_calculator_eval(const char *expr, double *out_value, char *errbuf, size_t errbuf_len);
void ghostesp_calculator_format(double value, char *buf, size_t len);

#endif
