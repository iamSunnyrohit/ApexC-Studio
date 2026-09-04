#include <jni.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "apex/lexer.h"
#include "apex/parser.h"
#include "apex/codegen.h"
#include "apex/eval.h"

JNIEXPORT jstring JNICALL
Java_com_example_apexc_MainActivity_evalSource(
        JNIEnv* env,
        jobject thiz,
        jstring c_source) {

    const char *source = (*env)->GetStringUTFChars(env, c_source, 0);
    if (!source) return (*env)->NewStringUTF(env, "Error: Empty input");

    Parser parser = parser_init(source);
    ASTNode *program = parse_program(&parser);

    if (!program) {
        (*env)->ReleaseStringUTFChars(env, c_source, source);
        return (*env)->NewStringUTF(env, "Syntax Error: Failed to parse C program");
    }

    int result_value = eval_program(program);
    (*env)->ReleaseStringUTFChars(env, c_source, source);

    char output_buf[64];
    snprintf(output_buf, sizeof(output_buf), "%d", result_value);
    return (*env)->NewStringUTF(env, output_buf);
}

JNIEXPORT jstring JNICALL
Java_com_example_apexc_MainActivity_compileToAsm(
        JNIEnv* env,
        jobject thiz,
        jstring c_source) {

    const char *source = (*env)->GetStringUTFChars(env, c_source, 0);
    if (!source) return (*env)->NewStringUTF(env, "// Error: Empty input");

    char *asm_buffer = NULL;
    size_t asm_size = 0;
    FILE *mem_out = open_memstream(&asm_buffer, &asm_size);

    if (!mem_out) {
        (*env)->ReleaseStringUTFChars(env, c_source, source);
        return (*env)->NewStringUTF(env, "// Error: Could not allocate memory stream");
    }

    Parser parser = parser_init(source);
    ASTNode *program = parse_program(&parser);

    if (!program) {
        fprintf(mem_out, "// ApexC Error: Syntax parsing failed\n");
    } else {
        codegen_generate(program, mem_out);
    }

    fflush(mem_out);
    fclose(mem_out);
    (*env)->ReleaseStringUTFChars(env, c_source, source);

    jstring result = (*env)->NewStringUTF(env, (asm_buffer && asm_size > 0) ? asm_buffer : "// No assembly generated");
    if (asm_buffer) free(asm_buffer);
    return result;
}