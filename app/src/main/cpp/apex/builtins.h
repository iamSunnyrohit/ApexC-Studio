#ifndef APEXC_BUILTINS_H
#define APEXC_BUILTINS_H

#include <stddef.h>

#define MAX_TERM_OUTPUT 4096

#ifdef __cplusplus
extern "C" {
#endif

// Global buffer capturing stdout calls (printf, puts, putchar)
extern char g_runtime_stdout[MAX_TERM_OUTPUT];

// Reset terminal capture buffer before every execution
void builtins_reset_stdout(void);

// Check if a symbol belongs to stdio.h, math.h, or compiler intrinsics
int is_builtin_func(const char *name);

// Execute intercepted intrinsic functions and return an integer exit/eval value
int execute_builtin(const char *name, int *int_args, double *float_args, int arg_count, const char *fmt_str);

#ifdef __cplusplus
}
#endif

#endif // APEXC_BUILTINS_H