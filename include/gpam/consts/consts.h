#ifndef __GPAM_CONSTS_H__
#define __GPAM_CONSTS_H__

#include <argp.h>

#define GPAM_VAULT_FILE_LOCATION "/home/vilya/.gpam.vaults.xml"

#define GPAM_EXIT_CODE_SUCCESS		 		 0
#define GPAM_EXIT_CODE_USER_ABORT_ERROR		 1
#define GPAM_EXIT_CODE_ACTION_ERROR  		 2
#define GPAM_EXIT_CODE_PARSE_ERROR 			 3
#define GPAM_EXIT_CODE_ACCESS_ERROR	 		 4
#define GPAM_EXIT_CODE_INITIALIZE_ERROR 	 5
#define GPAM_EXIT_CODE_INCORRECT_PATH_ERROR  6
#define GPAM_EXIT_CODE_VAULT_NOT_SET_ERROR	 7
#define GPAM_EXIT_CODE_SITE_NOT_SET_ERROR	 8
#define GPAM_EXIT_CODE_SITE_NOT_EXISTS_ERROR 9

const char* argp_program_version = "GPAM 1.0";
const char* GPAM_DOCUMENTATION = "\nGPAM - GNU Password Manager\n";

const char* GPAM_ARGUMENTS_DOCUMENTATION = "\nSITE\n"
										   "SITE LOGIN [PASSWORD=]PASSWORD [[FIELD=VALUE]...]";

const struct argp_option GPAM_OPTIONS[] = {
	{ "interactive", 'i', 0, 0, "interactive mode" },
	{ "add-vault", 1000, "NAME", 0, "add new vault." },
	{ "set-default-vault", 1001, "NAME", 0, "set default vault" },
	{ "default-vault", 1002, 0, 0, "shows current(default) vault" },

	{ 0, 0, 0, 0, "Specifying", 1 },
	{ "site", 	  'S', "VALUE", 0, "site(alias for \"-Fsite -Vvalue\")" },
	{ "login", 	  'L', "VALUE", 0, "login(alias for \"-Flogin -Vvalue\")" },
	{ "password", 'P', "VALUE", 0, "password(alias for \"-Fpassword -Vvalue\")" },
	{ "field",	  'F', "NAME",  0, "field name" },
	{ "value",	  'V', "VALUE", 0, "field value" },

	{ 0, 0, 0, 0, "Commands", 2 },
	{ "new", 	'n', 0, 0, "create a new record. Can contact with -i option" },
	{ "list",	'l', 0, 0, "shows all sites(if SITE specified) otherwise(if SITE and LOGIN specified) shows information about password" },
	{ "update", 'u', 0, 0, "update password" },
	{ "delete", 'd', 0, 0, "delete a record" },

	{ 0 }
};

#endif
