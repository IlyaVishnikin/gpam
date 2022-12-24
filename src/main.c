#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <argp.h>

#include "gpam/consts/consts.h"
#include "gpam/types/types.h"
#include "gpam/utils/utils.h"

/* ===== Used only for errors information ===== */
char* field_name_buffer  = NULL;
char* field_value_buffer = NULL;
char* field_declaration  = NULL;
/* ============================================ */

struct GpamArguments* gpam_arguments = NULL;


static void
on_action_add_new_vault(void)
{
	char* vault_name = strdup(gpam_arguments->new_vault_name);
	struct GpamVaultFile* vault_file = gpam_vault_file_new(GPAM_VAULT_FILE_LOCATION);

	void destroy(int exit_code)
	{
		free(vault_name);
		gpam_vault_file_destroy(vault_file);
		gpam_arguments_destroy(gpam_arguments);
		exit(exit_code);
	}

	if (!vault_file)
	{
		fprintf(stderr, "GPAM: Can't access to vault file: \"%s\".\n", GPAM_VAULT_FILE_LOCATION);
		destroy(GPAM_EXIT_CODE_ACCESS_ERROR);
	}

	if (!vault_file->initialize(vault_file))
	{
		fprintf(stderr, "GPAM: Can't initialize vault file: \"%s\".\n", GPAM_VAULT_FILE_LOCATION);
		destroy(GPAM_EXIT_CODE_INITIALIZE_ERROR);
	}

	if (!vault_file->add_vault(vault_file, vault_name))
	{
		fprintf(stderr, "GPAM: Can't add new vault.\n");
		destroy(GPAM_EXIT_CODE_PARSE_ERROR);
	}

	if (!vault_file->set_default_vault(vault_file, vault_name))
	{
		fprintf(stderr, "GPAM: Can't set new vault as default.\n");
		destroy(GPAM_EXIT_CODE_PARSE_ERROR);
	}

	destroy(GPAM_EXIT_CODE_SUCCESS);
}

static void
on_action_set_default_vault(void)
{
	struct GpamVaultFile* vault_file = gpam_vault_file_new(GPAM_VAULT_FILE_LOCATION);

	void destroy(int exit_code)
	{
		gpam_vault_file_destroy(vault_file);
		gpam_arguments_destroy(gpam_arguments);
		exit(exit_code);
	}

	if (!vault_file)
	{
		fprintf(stderr, "GPAM: Can't access to vault file: \"%s\".\n", GPAM_VAULT_FILE_LOCATION);
		destroy(GPAM_EXIT_CODE_ACCESS_ERROR);
	}

	if (!vault_file->set_default_vault(vault_file, gpam_arguments->new_default_vault_name))
	{
		fprintf(stderr, "GPAM: Can't set a new default vault: \"%s\"\n", gpam_arguments->new_default_vault_name);
		destroy(GPAM_EXIT_CODE_PARSE_ERROR);
	}

	destroy(GPAM_EXIT_CODE_SUCCESS);
}

static void
on_action_show_default_vault(void)
{
	struct GpamVaultFile* vault_file = gpam_vault_file_new(GPAM_VAULT_FILE_LOCATION);
	char* default_vault_name = NULL;

	void destroy(int exit_code)
	{
		xmlFree(default_vault_name);
		gpam_arguments_destroy(gpam_arguments);
		gpam_vault_file_destroy(vault_file);
		exit(exit_code);
	}

	if (!vault_file)
	{
		fprintf(stderr, "GPAM: Can't access to vault file: \"%s\".\n", GPAM_VAULT_FILE_LOCATION);
		destroy(GPAM_EXIT_CODE_ACCESS_ERROR);
	}

	default_vault_name = vault_file->get_default_vault(vault_file);
	default_vault_name ? printf("GPAM: default vault: \"%s\".\n", default_vault_name) :
						 fprintf(stderr, "GPAM: default vault is not installed.\n");
	destroy(GPAM_EXIT_CODE_SUCCESS);
}

static void
on_action_new_site(void)
{
	struct GpamVaultFile* vault_file = gpam_vault_file_new(GPAM_VAULT_FILE_LOCATION);
	char* default_vault_name = vault_file->get_default_vault(vault_file);
	char* site_name = gpam_arguments->records->get(gpam_arguments->records, "site")->value;

	void destroy(int exit_code)
	{
		xmlFree(default_vault_name);
		gpam_vault_file_destroy(vault_file);
		gpam_arguments_destroy(gpam_arguments);
		exit(exit_code);
	}

	if (!vault_file)
	{
		fprintf(stderr, "GPAM: Can't access to vault file: \"%s\".\n", GPAM_VAULT_FILE_LOCATION);
		destroy(GPAM_EXIT_CODE_ACCESS_ERROR);
	}

	if (!default_vault_name)
	{
		fprintf(stderr, "GPAM: Default vault is not installed.\n");
		destroy(GPAM_EXIT_CODE_VAULT_NOT_SET_ERROR);
	}

	if (!vault_file->add_site(vault_file, default_vault_name, site_name))
	{
		fprintf(stderr, "GPAM: Can't add a new site: \"%s\".\n", site_name);
		destroy(GPAM_EXIT_CODE_PARSE_ERROR);
	}

	destroy(GPAM_EXIT_CODE_SUCCESS);
}

static void
on_action_new_site_interactive(void)
{
	if (gpam_arguments->site)
		on_action_new_site();

	printf("Enter site name: ");
	char* site_name = gpam_utils_stdin_read_line();

	if (gpam_utils_is_string_empty(site_name))
	{
		free(site_name);
		fprintf(stderr, "GPAM: Empty site name. Aborted.\n");
		gpam_arguments_destroy(gpam_arguments);
		exit(GPAM_EXIT_CODE_USER_ABORT_ERROR);
	}
	gpam_arguments->records->add(gpam_arguments->records, gpam_record_new("site", site_name));
	free(site_name);
	on_action_new_site();
}

static void
on_action_new_record_specified_fields(void)
{
	struct GpamVaultFile* vault_file = gpam_vault_file_new(GPAM_VAULT_FILE_LOCATION);
	gpam_arguments->records->unique(gpam_arguments->records);

	char* default_vault_name = vault_file->get_default_vault(vault_file);
	char* site_name 		 = gpam_arguments->records->get(gpam_arguments->records, "site")->value;
	char* login 			 = gpam_arguments->records->get(gpam_arguments->records, "login")->value;
	int record_position 	 = vault_file->add_record(vault_file, default_vault_name, site_name, login);

	void destroy(int exit_code)
	{
		xmlFree(default_vault_name);
		gpam_vault_file_destroy(vault_file);
		gpam_arguments_destroy(gpam_arguments);
		exit(exit_code);
	}

	if (!vault_file)
	{
		fprintf(stderr, "GPAM: Can't access to vault file: \"%s\".\n", GPAM_VAULT_FILE_LOCATION);
		destroy(GPAM_EXIT_CODE_ACCESS_ERROR);
	}

	if (!default_vault_name)
	{
		fprintf(stderr, "GPAM: Default vault is not installed.\n");
		destroy(GPAM_EXIT_CODE_VAULT_NOT_SET_ERROR);
	}

	if (!vault_file->is_site_exists(vault_file, default_vault_name, site_name))
	{
		fprintf(stderr, "GPAM: Site \"%s\" is not exists.\n", site_name);
		destroy(GPAM_EXIT_CODE_SITE_NOT_EXISTS_ERROR);
	}
	
	if (record_position == -1)
	{
		fprintf(stderr, "GPAM: Can't create a new record.\n");
		destroy(GPAM_EXIT_CODE_PARSE_ERROR);
	}

	for (int i = 0; i < gpam_arguments->records->size; i++)
	{
		struct GpamRecord* record = gpam_arguments->records->data[i];
		if (strcmp(record->name, "site") == 0) continue;

		if (!vault_file->add_record_field(vault_file, default_vault_name, site_name, record_position, record->name, record->value))
			fprintf(stderr, "GPAM: Can't add a new record-field: \"%s\":\"%s\".\n", record->name, record->value);
	}

	destroy(GPAM_EXIT_CODE_SUCCESS);
}

static void
on_action_new_record_interactive(void)
{
	while (1)
	{
		printf("Enter record: ");
		char* record_string = gpam_utils_stdin_read_line();
		if (gpam_utils_is_string_empty(record_string))
		{
			free(record_string);
			break;
		}

		if (!strchr(record_string, '='))
		{
			fprintf(stderr, "Invalid record format.\n");
			free(record_string);
			continue;
		}

		struct GpamRecord* record = gpam_record_new_from_string(record_string, "=");
		free(record_string);
		if (!record)
		{
			fprintf(stderr, "Invalid record format.\n");
			continue;
		}
		gpam_arguments->records->add(gpam_arguments->records, record);
	}
	on_action_new_record_specified_fields();
}

static void
on_action_list_sites(void)
{
	struct GpamVaultFile* vault_file = gpam_vault_file_new(GPAM_VAULT_FILE_LOCATION);

	char*  default_vault_name = vault_file->get_default_vault(vault_file);
	size_t sites_count 		  = vault_file->get_sites_count(vault_file, default_vault_name);

	void destroy(int exit_code)
	{
		xmlFree(default_vault_name);
		gpam_vault_file_destroy(vault_file);
		gpam_arguments_destroy(gpam_arguments);
		exit(exit_code);
	}

	if (!vault_file)
	{
		fprintf(stderr, "GPAM: Can't access to vault file: \"%s\".\n", GPAM_VAULT_FILE_LOCATION);
		destroy(GPAM_EXIT_CODE_ACCESS_ERROR);
	}

	if (!default_vault_name)
	{
		fprintf(stderr, "GPAM: Default vault is not installed.\n");
		destroy(GPAM_EXIT_CODE_VAULT_NOT_SET_ERROR);
	}

	if (sites_count < 1) destroy(GPAM_EXIT_CODE_SUCCESS);

	for (int i = 0; i < sites_count; i++)
	{
		char* site_name = vault_file->get_site_name(vault_file, default_vault_name, i);
		site_name ? printf("%s\n", site_name) : fprintf(stderr, "GPAM: Can't extract name for vault[%s]/site[%d].\n", default_vault_name, i);
		xmlFree(site_name);
	}
	destroy(GPAM_EXIT_CODE_SUCCESS);
}

static void
on_action_list_record(void)
{
	struct GpamVaultFile* vault_file = gpam_vault_file_new(GPAM_VAULT_FILE_LOCATION);

	char*  default_vault_name  = vault_file->get_default_vault(vault_file);
	char*  site_name 		   = gpam_arguments->records->get(gpam_arguments->records, "site")->value;
	char*  login			   = gpam_arguments->records->get(gpam_arguments->records, "login")->value;
	int    record_position 	   = vault_file->get_record_position(vault_file, default_vault_name, site_name, login);
	size_t record_fields_count = vault_file->get_record_fields_count(vault_file, default_vault_name, site_name, login);

	void destroy(int exit_code)
	{
		xmlFree(default_vault_name);
		gpam_vault_file_destroy(vault_file);
		gpam_arguments_destroy(gpam_arguments);
		exit(exit_code);
	}

	if (!vault_file)
	{
		fprintf(stderr, "GPAM: Can't access to vault file: \"%s\".\n", GPAM_VAULT_FILE_LOCATION);
		destroy(GPAM_EXIT_CODE_ACCESS_ERROR);
	}

	if (!default_vault_name)
	{
		fprintf(stderr, "GPAM: Default vault is not installed.\n");
		destroy(GPAM_EXIT_CODE_VAULT_NOT_SET_ERROR);
	}

	if (record_fields_count < 1) destroy(GPAM_EXIT_CODE_SUCCESS);

	for (int i = 0; i < record_fields_count; i++)
	{
		char* record_field_name  = vault_file->get_record_field_name(vault_file, default_vault_name, site_name, record_position, i);
		char* record_field_value = vault_file->get_record_field_value(vault_file, default_vault_name, site_name, record_position, i);
		if (record_field_name) printf("%s: %s\n", record_field_name, record_field_value);

		xmlFree(record_field_name);
		xmlFree(record_field_value);
	}

	destroy(GPAM_EXIT_CODE_SUCCESS);
}

static void
on_action_update_site_interactive(void)
{
	struct GpamVaultFile* vault_file = gpam_vault_file_new(GPAM_VAULT_FILE_LOCATION);

	char* default_vault_name = vault_file->get_default_vault(vault_file);
	char* site_name 		 = gpam_arguments->records->get(gpam_arguments->records, "site")->value;
	char* new_site_name		 = NULL;

	void destroy(int exit_code)
	{
		free(new_site_name);
		xmlFree(default_vault_name);
		gpam_vault_file_destroy(vault_file);
		gpam_arguments_destroy(gpam_arguments);
		exit(exit_code);
	}

	if (!vault_file)
	{
		fprintf(stderr, "GPAM: Can't access to vault file: \"%s\".\n", GPAM_VAULT_FILE_LOCATION);
		destroy(GPAM_EXIT_CODE_ACCESS_ERROR);
	}

	if (!default_vault_name)
	{
		fprintf(stderr, "GPAM: Default vault is not installed.\n");
		destroy(GPAM_EXIT_CODE_VAULT_NOT_SET_ERROR);
	}

	printf("Enter site name: ");
	new_site_name = gpam_utils_stdin_read_line();
	if (gpam_utils_is_string_empty(new_site_name))
	{
		fprintf(stderr, "GPAM: Empty site name. Aborted.\n");
		destroy(GPAM_EXIT_CODE_USER_ABORT_ERROR);
	}

	if (!vault_file->set_site_name(vault_file, default_vault_name, site_name, new_site_name))
	{
		fprintf(stderr, "GPAM: Can't update site name.\n");
		destroy(GPAM_EXIT_CODE_PARSE_ERROR);
	}

	destroy(GPAM_EXIT_CODE_SUCCESS);
}

static void
on_action_update_record_specified_fields(void)
{
	struct GpamVaultFile* vault_file = gpam_vault_file_new(GPAM_VAULT_FILE_LOCATION);

	char* default_vault_name = vault_file->get_default_vault(vault_file);
	char* site_name 		 = strdup(gpam_arguments->records->get(gpam_arguments->records, "site")->value);
	char* login				 = strdup(gpam_arguments->records->get(gpam_arguments->records, "login")->value);
	int   record_position 	 = vault_file->get_record_position(vault_file, default_vault_name, site_name, login);
	gpam_arguments->records->unique(gpam_arguments->records);
	char* new_login = gpam_arguments->records->get(gpam_arguments->records, "login")->value;

	void destroy(int exit_code)
	{
		xmlFree(default_vault_name);
		free(site_name);
		free(login);
		gpam_vault_file_destroy(vault_file);
		gpam_arguments_destroy(gpam_arguments);
		exit(exit_code);
	}

	if (!vault_file)
	{
		fprintf(stderr, "GPAM: Can't access to vault file: \"%s\".\n", GPAM_VAULT_FILE_LOCATION);
		destroy(GPAM_EXIT_CODE_ACCESS_ERROR);
	}

	if (!default_vault_name)
	{
		fprintf(stderr, "GPAM: Default vault is not installed.\n");
		destroy(GPAM_EXIT_CODE_VAULT_NOT_SET_ERROR);
	}

	if (record_position == -1)
	{
		fprintf(stderr, "GPAM: Can't create a new record.\n");
		destroy(GPAM_EXIT_CODE_PARSE_ERROR);
	}

	for (int i = 0; i < gpam_arguments->records->size; i++)
	{
		struct GpamRecord* record = gpam_arguments->records->data[i];
		if (strcmp(record->name, "site") == 0 )
			vault_file->set_site_name(vault_file, default_vault_name, site_name, record->value);
		else if (strcmp(record->name, "login") != 0)
			vault_file->add_record_field(vault_file, default_vault_name, site_name, record_position, record->name, record->value);
	}
	
	if (new_login)
		vault_file->add_record_field(vault_file, default_vault_name, site_name, record_position, "login", new_login);

	destroy(GPAM_EXIT_CODE_SUCCESS);
}

static void
on_action_update_record_interactive(void)
{
	while (1)
	{
		printf("Enter record: ");
		char* record_string = gpam_utils_stdin_read_line();
		if (gpam_utils_is_string_empty(record_string))
		{
			free(record_string);
			break;
		}

		if (!strchr(record_string, '='))
		{
			fprintf(stderr, "Invalid record format.\n");
			free(record_string);
			continue;
		}

		struct GpamRecord* record = gpam_record_new_from_string(record_string, "=");
		free(record_string);
		if (!record)
		{
			fprintf(stderr, "Invalid record format.\n");
			continue;
		}
		gpam_arguments->records->add(gpam_arguments->records, record);
	}
	on_action_update_record_specified_fields();
}

static void
on_action_delete_site(void)
{
	struct GpamVaultFile* vault_file = gpam_vault_file_new(GPAM_VAULT_FILE_LOCATION);

	char* default_vault_name = vault_file->get_default_vault(vault_file);
	char* site_name = gpam_arguments->records->get(gpam_arguments->records, "site")->value;

	void destroy(int exit_code)
	{
		xmlFree(default_vault_name);
		gpam_vault_file_destroy(vault_file);
		gpam_arguments_destroy(gpam_arguments);
		exit(exit_code);
	}

	if (!vault_file)
	{
		fprintf(stderr, "GPAM: Can't access to vault file: \"%s\".\n", GPAM_VAULT_FILE_LOCATION);
		destroy(GPAM_EXIT_CODE_ACCESS_ERROR);
	}

	if (!default_vault_name)
	{
		fprintf(stderr, "GPAM: Default vault is not installed.\n");
		destroy(GPAM_EXIT_CODE_VAULT_NOT_SET_ERROR);
	}

	if (!vault_file->delete_site(vault_file, default_vault_name, site_name))
	{
		fprintf(stderr, "GPAM: Can't delete specified site: \"%s\".\n", site_name);
		destroy(GPAM_EXIT_CODE_SUCCESS);
	}

	destroy(GPAM_EXIT_CODE_SUCCESS);
}

static void
on_action_delete_site_interactive(void)
{
	if (gpam_arguments->site)
		on_action_delete_site();

	printf("Enter site name: ");
	char* site_name = gpam_utils_stdin_read_line();

	if (gpam_utils_is_string_empty(site_name))
	{
		free(site_name);
		fprintf(stderr, "GPAM: Empty site name. Aborted.\n");
		gpam_arguments_destroy(gpam_arguments);
		exit(GPAM_EXIT_CODE_USER_ABORT_ERROR);
	}
	gpam_arguments->records->add(gpam_arguments->records, gpam_record_new("site", site_name));
	free(site_name);
	on_action_delete_site();
}

static void
on_action_delete_record_full(void)
{
	struct GpamVaultFile* vault_file = gpam_vault_file_new(GPAM_VAULT_FILE_LOCATION);

	char* default_vault_name = vault_file->get_default_vault(vault_file);
	char* site_name 		 = gpam_arguments->records->get(gpam_arguments->records, "site")->value;
	char* login 			 = gpam_arguments->records->get(gpam_arguments->records, "login")->value;
	int record_position 	 = vault_file->get_record_position(vault_file, default_vault_name, site_name, login);

	void destroy(int exit_code)
	{
		xmlFree(default_vault_name);
		gpam_vault_file_destroy(vault_file);
		gpam_arguments_destroy(gpam_arguments);
		exit(exit_code);
	}

	if (!vault_file)
	{
		fprintf(stderr, "GPAM: Can't access to vault file: \"%s\".\n", GPAM_VAULT_FILE_LOCATION);
		destroy(GPAM_EXIT_CODE_ACCESS_ERROR);
	}

	if (!default_vault_name)
	{
		fprintf(stderr, "GPAM: Default vault is not installed.\n");
		destroy(GPAM_EXIT_CODE_VAULT_NOT_SET_ERROR);
	}

	if (record_position == -1)
	{
		fprintf(stderr, "GPAM: Can't create a new record.\n");
		destroy(GPAM_EXIT_CODE_PARSE_ERROR);
	}

	if (!vault_file->delete_record(vault_file, default_vault_name, site_name, record_position))
	{
		fprintf(stderr, "GPAM: Can't remove record for login: \"%s\".\n", login);
		destroy(GPAM_EXIT_CODE_PARSE_ERROR);
	}

	destroy(GPAM_EXIT_CODE_SUCCESS);
}

static error_t
parse_options(int key, char* argument, struct argp_state* state)
{
	switch (key)
	{
	case 1000: /* --add-vault */
		gpam_arguments->new_vault_name = strdup(argument);
		return GPAM_ARGP_EXIT_STATUS_TERMINATED_OK;

	case 1001: /* --set-default-vault */
		gpam_arguments->new_default_vault_name = strdup(argument);
		return GPAM_ARGP_EXIT_STATUS_TERMINATED_OK;

	case 1002: /* --default-vault */
		gpam_arguments->is_show_default_vault = true;
		return GPAM_ARGP_EXIT_STATUS_TERMINATED_OK;

	case 1003: /* --set-master-key */
		gpam_arguments->new_master_key_value = strdup(argument);
		return GPAM_ARGP_EXIT_STATUS_TERMINATED_OK;

	case 'i':
		gpam_arguments->is_interactive_mode = true;
		break;

	case 'S':
		parse_options('F', "site", state);
		return parse_options('V', argument, state);

	case 'L':
		parse_options('F', "login", state);
		return parse_options('V', argument, state);

	case 'P':
		parse_options('F', "password", state);
		return parse_options('V', argument, state);

	case 'F':
		if (!gpam_utils_is_string_empty(field_name_buffer))
			return GPAM_ARGP_EXIT_STATUS_FIELD_NAME_ALREADY_SPECIFIED;

		field_name_buffer = strdup(argument);
		break;

	case 'V':
		field_value_buffer = strdup(argument);
		if (gpam_utils_is_string_empty(field_name_buffer))
			return GPAM_ARGP_EXIT_STATUS_FIELD_NAME_NOT_SPECIFIED;

		gpam_arguments->records->add(gpam_arguments->records, gpam_record_new(field_name_buffer, field_value_buffer));
		free(field_name_buffer);
		free(field_value_buffer);
		field_name_buffer  = NULL;
		field_value_buffer = NULL;
		gpam_arguments->arg_num++;
		break;

	case 'n':
		gpam_arguments->is_command_new = true;
		break;

	case 'l':
		gpam_arguments->is_command_list = true;
		break;

	case 'u':
		gpam_arguments->is_command_update = true;
		break;

	case 'd':
		gpam_arguments->is_command_delete = true;
		break;

	case ARGP_KEY_ARG:
		if (argument[0] == '=' || strchr(argument, '=') == NULL)
		{
			if 		(gpam_arguments->arg_num == 0) parse_options('S', argument, state);
			else if (gpam_arguments->arg_num == 1) parse_options('L', argument, state);
			else if (gpam_arguments->arg_num == 2) parse_options('P', argument, state);
			else if (gpam_arguments->arg_num > 2)
			{
				field_declaration = strdup(argument);
				return GPAM_ARGP_EXIT_STATUS_INVALID_FIELD_DECLARATION;
			}
		}
		else
		{
			field_declaration = strdup(argument);
			char* field_name  = strtok(argument, "=");
			char* field_value = strtok(NULL, 	 "=");
			if (gpam_utils_is_string_empty(field_value)) return GPAM_ARGP_EXIT_STATUS_INVALID_FIELD_DECLARATION;

			parse_options('F', field_name,  state);
			parse_options('V', field_value, state);
			free(field_declaration);
			field_declaration = NULL;
		}
		break;

	case ARGP_KEY_END:
		if (!gpam_utils_is_string_empty(field_name_buffer) && !field_value_buffer)
			return GPAM_ARGP_EXIT_STATUS_FIELD_VALUE_NOT_SPECIFIED;
		break;
	}

	return GPAM_ARGP_EXIT_STATUS_OK;
}

int main(int argc, char* argv[])
{
	gpam_arguments = gpam_arguments_new();
	
	struct argp argp = { GPAM_OPTIONS, parse_options, GPAM_ARGUMENTS_DOCUMENTATION, GPAM_DOCUMENTATION };
	enum GpamArgpExitStatus exit_status = argp_parse(&argp, argc, argv, ARGP_IN_ORDER, 0, 0);

	/* errors processing */
	if (exit_status != GPAM_ARGP_EXIT_STATUS_OK && exit_status != GPAM_ARGP_EXIT_STATUS_TERMINATED_OK)
	{
		gpam_arguments_destroy(gpam_arguments);
		switch (exit_status)
		{
		case GPAM_ARGP_EXIT_STATUS_FIELD_NAME_ALREADY_SPECIFIED:
			fprintf(stderr, "GPAM: Creating new field but value for the previos field \"%s\" was never been specified.\n", field_name_buffer);
			free(field_name_buffer);
			break;

		case GPAM_ARGP_EXIT_STATUS_FIELD_NAME_NOT_SPECIFIED:
			fprintf(stderr, "GPAM: Field name for the value \"%s\" was never been specified.\n", field_value_buffer);
			free(field_value_buffer);
			break;

		case GPAM_ARGP_EXIT_STATUS_FIELD_VALUE_NOT_SPECIFIED:
			fprintf(stderr, "GPAM: Value for the field \"%s\" was never been specified.\n", field_name_buffer);
			free(field_name_buffer);
			break;

		case GPAM_ARGP_EXIT_STATUS_INVALID_FIELD_DECLARATION:
			fprintf(stderr, "GPAM: Field \"%s\" is decleared incorrectly.\n", field_declaration);
			free(field_declaration);
			break;
		}
		exit(GPAM_EXIT_CODE_PARSE_ERROR);
	}

	enum GpamAction action = gpam_action_get_action(gpam_arguments);
	if (gpam_action_is_action_invalid(action))
	{
		gpam_arguments_destroy(gpam_arguments);
		switch (action)
		{
		case GPAM_ACTION_UNKNOWN:
			fprintf(stderr, "GPAM: Unkonw usage of options or arguments.\n");
			break;

		case GPAM_ACTION_INVALID_NEW_SITE_SITE_NOT_SPECIFIED:
			fprintf(stderr, "GPAM: Invalid usage of '-n' or '--new' option: site has not been specified.\n");
			break;

		case GPAM_ACTION_INVALID_NEW_RECORD_FIELDS_NOT_SPECIFIED:
			fprintf(stderr, "GPAM: Invalid usage of '-n' or '--new' option: "
							"fields for new record has not been specified.\n");
			break;

		case GPAM_ACTION_INVALID_UPDATE_RECORD_FIELDS_NOT_SPECIFIED:
			fprintf(stderr, "GPAM: Invalid usage of '-u' or '--update' option: "
							"fields for updated record has not been specified.\n");
			break;

		case GPAM_ACTION_INVALID_UPDATE_SITE_NEW_SITE_NOT_SPECIFIED:
			break;

		case GPAM_ACTION_INVALID_UPDATE_SITE_SITE_NOT_SPECIFIED:
			fprintf(stderr, "GPAM: Invalid usage of '-u' or '--update' option: site has not been specified.\n");
			break;

		case GPAM_ACTION_INVALID_DELETE_SITE_SITE_NOT_SPECIFIED:
			fprintf(stderr, "GPAM: Invalid usage of '-d' or '--delete' options: site has not been specified.\n");
			break;
		}
		exit(GPAM_EXIT_CODE_ACTION_ERROR);		
	}

	switch (action)
	{
	case GPAM_ACTION_ADD_VAULT: 		 on_action_add_new_vault(); 	 break;
	case GPAM_ACTION_SET_DEFAULT_VAULT:  on_action_set_default_vault();  break;
	case GPAM_ACTION_SHOW_DEFAULT_VAULT: on_action_show_default_vault(); break;
	
	case GPAM_ACTION_NEW_SITE: 		 	   		  on_action_new_site(); 			 	   break;
	case GPAM_ACTION_NEW_SITE_INTERACTIVE: 		  on_action_new_site_interactive(); 	   break;
	case GPAM_ACTION_NEW_RECORD_SPECIFIED_FIELDS: on_action_new_record_specified_fields(); break;
	case GPAM_ACTION_NEW_RECORD_INTERACTIVE: 	  on_action_new_record_interactive();	   break;

	case GPAM_ACTION_LIST_SITES:  on_action_list_sites();  break;
	case GPAM_ACTION_LIST_RECORD: on_action_list_record(); break;

	case GPAM_ACTION_UPDATE_SITE_INTERACTIVE:	  	 on_action_update_site_interactive(); 		 break;
	case GPAM_ACTION_UPDATE_RECORD_SPECIFIED_FIELDS: on_action_update_record_specified_fields(); break;
	case GPAM_ACTION_UPDATE_RECORD_INTERCATIVE:		 on_action_update_record_interactive();		 break;
	
	case GPAM_ACTION_DELETE_SITE: 			  on_action_delete_site(); 			   break;
	case GPAM_ACTION_DELETE_SITE_INTERACTIVE: on_action_delete_site_interactive(); break;
	case GPAM_ACTION_DELETE_RECORD_FULL:	  on_action_delete_record_full();	   break;
	}
}
