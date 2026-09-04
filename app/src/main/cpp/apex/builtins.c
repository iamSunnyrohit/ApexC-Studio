#include "builtins.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

char g_runtime_stdout[MAX_TERM_OUTPUT] = {0};

void builtins_reset_stdout(void) {
    g_runtime_stdout[0] = '\0';
}

static void append_output(const char *text) {
    size_t cur_len = strlen(g_runtime_stdout);
    if (cur_len < MAX_TERM_OUTPUT - 1) {
        strncat(g_runtime_stdout, text, MAX_TERM_OUTPUT - cur_len - 1);
    }
}

int is_builtin_func(const char *name) {
    if (!name) return 0;
    // stdio.h
    if (strcmp(name, "printf") == 0 || strcmp(name, "puts") == 0 ||
        strcmp(name, "putchar") == 0) {
        return 1;
    }
    // math.h
    if (strcmp(name, "sqrt") == 0 || strcmp(name, "pow") == 0 ||
        strcmp(name, "abs") == 0  || strcmp(name, "sin") == 0 ||
        strcmp(name, "cos") == 0  || strcmp(name, "floor") == 0 ||
        strcmp(name, "ceil") == 0) {
        return 1;
    }
    return 0;
}

int execute_builtin(const char *name, int *int_args, double *float_args, int arg_count, const char *fmt_str) {
    char temp[256];

    if (strcmp(name, "puts") == 0 && fmt_str) {
        snprintf(temp, sizeof(temp), "%s\n", fmt_str);
        append_output(temp);
        return (int)strlen(temp);
    }

    if (strcmp(name, "putchar") == 0 && arg_count > 0) {
        temp[0] = (char)int_args[0];
        temp[1] = '\0';
        append_output(temp);
        return int_args[0];
    }

    if (strcmp(name, "printf") == 0) {
        if (!fmt_str) {
            if (arg_count > 0) {
                snprintf(temp, sizeof(temp), "%d", int_args[0]);
                append_output(temp);
            }
            return 0;
        }

        char formatted[512] = {0};
        int arg_idx = 0;
        int out_idx = 0;

        for (int i = 0; fmt_str[i] != '\0' && out_idx < 510; i++) {
            if (fmt_str[i] == '%' && fmt_str[i + 1] != '\0') {
                i++;
                if (fmt_str[i] == 'd' || fmt_str[i] == 'i') {
                    if (arg_idx < arg_count) {
                        out_idx += snprintf(formatted + out_idx, 512 - out_idx, "%d", int_args[arg_idx++]);
                    }
                } else if (fmt_str[i] == 'c') {
                    if (arg_idx < arg_count) {
                        formatted[out_idx++] = (char)int_args[arg_idx++];
                    }
                } else if (fmt_str[i] == '%') {
                    formatted[out_idx++] = '%';
                }
            } else if (fmt_str[i] == '\\' && fmt_str[i + 1] == 'n') {
                formatted[out_idx++] = '\n';
                i++;
            } else {
                formatted[out_idx++] = fmt_str[i];
            }
        }
        append_output(formatted);
        return out_idx;
    }

    if (strcmp(name, "abs") == 0 && arg_count > 0) return abs(int_args[0]);
    if (strcmp(name, "sqrt") == 0 && arg_count > 0) return (int)sqrt((double)int_args[0]);
    if (strcmp(name, "pow") == 0 && arg_count > 1) return (int)pow((double)int_args[0], (double)int_args[1]);
    if (strcmp(name, "floor") == 0 && arg_count > 0) return (int)floor((double)int_args[0]);
    if (strcmp(name, "ceil") == 0 && arg_count > 0) return (int)ceil((double)int_args[0]);

    return 0;
}