#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <libgen.h>
#include <libxml/parser.h>
#include <libxml/xmlmemory.h>

#include "utils.h"

bool
gpam_utils_is_string_uppercase(const char* string)
{
	if (!string) return true;

	for (int i = 0; i < strlen(string); i++)
		if (!isupper(string[i]) && isalpha(string[i]))
			return false;

	return true;
}

bool
gpam_utils_is_string_empty(const char* string)
{
	if (!string) return true;

	return strcmp(string, "") == 0;
}

char*
gpam_utils_stdin_read_line(void)
{
	char* output_line = malloc(sizeof(char));
	output_line[0] = '\0';
	char current_char;
	size_t output_line_size = 1;
	while ((current_char = getchar()) != '\n')
	{
		void* new_location = realloc(output_line, ++output_line_size);
		if (!new_location) return output_line;
		output_line = new_location;
		strncat(output_line, &current_char, 1);
	}
	return output_line;
}
