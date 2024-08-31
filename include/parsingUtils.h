#pragma once

bool validNumberOfArgs(int argc, int expected);
bool isPositiveIntegerNumber(const char* str);
size_t concatenated_args_size(char** argv, int n_args);
void concatenate_args(char* args_str_dest, char** argv_src, int n_args);
char** tokenize_str(const char* str, size_t n_tokens, char splitter) ;
void append_null(char*** arr, size_t size);
char* itoa(size_t num);