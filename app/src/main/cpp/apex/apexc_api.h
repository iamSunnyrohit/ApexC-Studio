#ifndef APEXC_API_H
#define APEXC_API_H

#ifdef __cplusplus
extern "C" {
#endif

// Evaluates C source and returns formatted terminal output (stdout + exit code)
char *apexc_eval(const char *source);

// Compiles C source into AAPCS64 ARM64 assembly string
char *apexc_compile_asm(const char *source);

// Free strings allocated by apexc_eval and apexc_compile_asm
void apexc_free_string(char *str);

#ifdef __cplusplus
}
#endif

#endif // APEXC_API_H