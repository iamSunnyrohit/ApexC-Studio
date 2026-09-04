#include <jni.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "apex/lexer.h"
#include "apex/parser.h"
#include "apex/eval.h"
#include "apex/codegen.h"
#include "apex/builtins.h"

JNIEXPORT jstring JNICALL
Java_com_example_apexc_MainActivity_evalSource(
        JNIEnv* env,
        jobject thiz,
        jstring c_source) {

    if (!c_source) {
        return (*env)->NewStringUTF(env, "Error: Null source passed");
    }

    const char *source = (*env)->GetStringUTFChars(env, c_source, 0);
    if (!source || strlen(source) == 0) {
        if (source) (*env)->ReleaseStringUTFChars(env, c_source, source);
        return (*env)->NewStringUTF(env, "Error: Empty input source");
    }

    // Reset captured standard output buffer before starting execution
    builtins_reset_stdout();

    // 1. Initialize Lexer & Parser
    Parser parser = parser_init(source);
    ASTNode *program = parse_program(&parser);

    if (!program) {
        (*env)->ReleaseStringUTFChars(env, c_source, source);
        return (*env)->NewStringUTF(env, "Syntax Error: Failed to parse C program");
    }

    // 2. Evaluate AST
    int return_code = eval_program(program);

    // 3. Clean up AST & source string
    ast_free(program);
    (*env)->ReleaseStringUTFChars(env, c_source, source);

    // 4. Combine captured stdout with the return code
    char output_buf[MAX_TERM_OUTPUT + 256];
    if (strlen(g_runtime_stdout) > 0) {
        snprintf(output_buf, sizeof(output_buf),
                 "%s\n[Program exited with code: %d]",
                 g_runtime_stdout, return_code);
    } else {
        snprintf(output_buf, sizeof(output_buf),
                 "[Program exited with code: %d]",
                 return_code);
    }

    return (*env)->NewStringUTF(env, output_buf);
}

JNIEXPORT jstring JNICALL
Java_com_example_apexc_MainActivity_compileToAsm(
        JNIEnv* env,
        jobject thiz,
        jstring c_source) {

    if (!c_source) {
        return (*env)->NewStringUTF(env, "// Error: Null source passed");
    }

    const char *source = (*env)->GetStringUTFChars(env, c_source, 0);
    if (!source || strlen(source) == 0) {
        if (source) (*env)->ReleaseStringUTFChars(env, c_source, source);
        return (*env)->NewStringUTF(env, "// Error: Empty input source");
    }

    char *asm_buffer = NULL;
    size_t asm_size = 0;
    FILE *mem_out = open_memstream(&asm_buffer, &asm_size);

    if (!mem_out) {
        (*env)->ReleaseStringUTFChars(env, c_source, source);
        return (*env)->NewStringUTF(env, "// Internal Error: Could not allocate memory stream for codegen");
    }

    // 1. Initialize Lexer & Parser
    Parser parser = parser_init(source);
    ASTNode *program = parse_program(&parser);

    if (!program) {
        fprintf(mem_out, "// ApexC Error: Syntax parsing failed\n");
    } else {
        // 2. Emit AArch64 GNU Assembly
        codegen_generate(program, mem_out);
        ast_free(program);
    }

    fflush(mem_out);
    fclose(mem_out);
    (*env)->ReleaseStringUTFChars(env, c_source, source);

    jstring result = (*env)->NewStringUTF(
            env,
            (asm_buffer && asm_size > 0) ? asm_buffer : "// No assembly generated"
    );

    if (asm_buffer) {
        free(asm_buffer);
    }

    return result;
}