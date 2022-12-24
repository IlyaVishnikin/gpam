#ifndef __GPAM_UTILS_H__
#define __GPAM_UTILS_H__

#include <stdbool.h>

bool gpam_utils_is_string_uppercase(const char* string);
bool gpam_utils_is_string_empty(const char* string);
char* gpam_utils_stdin_read_line();

#endif
