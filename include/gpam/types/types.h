#ifndef __GPAM_TYPES_H__
#define __GPAM_TYPES_H__

#include <stdbool.h>
#include <libxml/xpath.h>

/* ===== GpamRecord ===== */
struct GpamRecord
{
	char* name;
	char* value;

	bool (*set_name) (struct GpamRecord* self, const char* new_name);
	bool (*set_value)(struct GpamRecord* self, const char* new_value);
};

bool 			   gpam_record_destroy 		  (struct GpamRecord* self);
struct GpamRecord* gpam_record_new 			  (const char* name,   const char* value);
struct GpamRecord* gpam_record_new_from_string(const char* string, const char* delimiter);
/* ====================== */

/* ===== GpamRecordList ===== */
struct GpamRecordList
{
	struct GpamRecord** data;
	size_t size;
	size_t capacity;

	bool (*set_capacity)(struct GpamRecordList* self, size_t new_capacity);
	bool (*add)(struct GpamRecordList* self, struct GpamRecord* record);
	bool (*unique)(struct GpamRecordList* self);
	struct GpamRecord* (*get)(struct GpamRecordList* self, const char* record_name);
};

bool 				   gpam_record_list_destroy(struct GpamRecordList* self);
struct GpamRecordList* gpam_record_list_new    (struct GpamRecord** data, size_t size);
/* ========================== */

/* ===== GpamArguments ===== */
struct GpamArguments
{
	struct GpamRecordList* records;

	char* site;
	char* login;
	bool is_interactive_mode,
		 is_command_new,
		 is_command_delete,
		 is_command_update,
		 is_command_list;

	bool is_show_default_vault;
	char* new_vault_name;
	char* new_default_vault_name;
	char* new_master_key_value;

	size_t arg_num;
};

void 				  gpam_arguments_destroy(struct GpamArguments* self);
struct GpamArguments* gpam_arguments_new	(void);
/* ========================= */

/* ===== GpamExitStatus ===== */
enum GpamArgpExitStatus
{
	GPAM_ARGP_EXIT_STATUS_OK,
	GPAM_ARGP_EXIT_STATUS_TERMINATED_OK,
	GPAM_ARGP_EXIT_STATUS_FIELD_NAME_ALREADY_SPECIFIED,
	GPAM_ARGP_EXIT_STATUS_FIELD_NAME_NOT_SPECIFIED,
	GPAM_ARGP_EXIT_STATUS_FIELD_VALUE_NOT_SPECIFIED,
	GPAM_ARGP_EXIT_STATUS_INVALID_FIELD_DECLARATION,
	GPAM_ARGP_EXIT_STATUS_UNKNOWN,
};
/* ========================== */

/* ===== GpamAction ===== */
/* On update @GpamAction, don't forget change
 * gpam_action_is_action_invalid function
 * in gpam/types/types.c file. */
enum GpamAction
{
	GPAM_ACTION_UNKNOWN,

	/* used to identify errors */
	GPAM_ACTION_INVALID_NEW_SITE_SITE_NOT_SPECIFIED,
	GPAM_ACTION_INVALID_NEW_RECORD_FIELDS_NOT_SPECIFIED,

	GPAM_ACTION_INVALID_UPDATE_SITE_SITE_NOT_SPECIFIED,
	GPAM_ACTION_INVALID_UPDATE_SITE_NEW_SITE_NOT_SPECIFIED,
	GPAM_ACTION_INVALID_UPDATE_RECORD_FIELDS_NOT_SPECIFIED,

	GPAM_ACTION_INVALID_DELETE_SITE_SITE_NOT_SPECIFIED,	

	/* toplevel actions */
	GPAM_ACTION_ADD_VAULT, // done
	GPAM_ACTION_SET_DEFAULT_VAULT, // done
	GPAM_ACTION_SHOW_DEFAULT_VAULT, // done,
	GPAM_ACTION_SET_MASTER_KEY,

	/* -n(--new) option */
	GPAM_ACTION_NEW_SITE, // done
	GPAM_ACTION_NEW_SITE_INTERACTIVE, // done
	GPAM_ACTION_NEW_RECORD_INTERACTIVE, // done
	GPAM_ACTION_NEW_RECORD_SPECIFIED_FIELDS, // done

	/* -l(--list) option */
	GPAM_ACTION_LIST_SITES, // done
	GPAM_ACTION_LIST_RECORD, // done

	/* -u(--update) option */
	GPAM_ACTION_UPDATE_SITE_INTERACTIVE, // done
	GPAM_ACTION_UPDATE_RECORD_INTERCATIVE, // done
	GPAM_ACTION_UPDATE_RECORD_SPECIFIED_FIELDS, // done

	/* -d(--update) option */
	GPAM_ACTION_DELETE_SITE, // done
	GPAM_ACTION_DELETE_SITE_INTERACTIVE, // done
	// GPAM_ACTION_DELETE_RECORD_FIELDS_INTERACTIVE,
	GPAM_ACTION_DELETE_RECORD_FULL, // done
	// GPAM_ACTION_DELETE_RECORD_SPECIFIED_FIELDS
};

enum GpamAction gpam_action_get_action(const struct GpamArguments* gpam_arguments);
bool gpam_action_is_action_invalid(enum GpamAction action);
/* ====================== */

/* ===== GpamXMLFile ===== */
struct GpamXMLFile
{
	char* path;
	xmlDocPtr xml_document;

	bool (*is_empty)(struct GpamXMLFile* self);

	bool (*clear)(struct GpamXMLFile* self);
	bool (*puts)(struct GpamXMLFile* self, const char* string);
	bool (*parse)(struct GpamXMLFile* self);
	bool (*close)(struct GpamXMLFile* self);
	bool (*save)(struct GpamXMLFile* self);

	bool (*__format_xpath)(const char* xpath, ...);

	bool (*is_node_exists)(struct GpamXMLFile* self, const char* xpath, ...);
	xmlXPathObjectPtr (*get_nodes)(struct GpamXMLFile* self, const char* xpath, ...);
};

struct GpamXMLFile* gpam_xml_file_new(const char* path);
void gpam_xml_file_destroy(struct GpamXMLFile* self);
/* ==================== */

/* ===== GpamConfigurationFile ===== */
struct GpamVaultFile
{
	struct GpamXMLFile* file;

	bool (*initialize)(struct GpamVaultFile* self);

	bool (*set_default_vault)(struct GpamVaultFile* self, const char* vault_name);
	bool (*set_site_name)(struct GpamVaultFile* self, const char* vault_name, const char* site_name, const char* new_site_name);

	bool (*add_vault)(struct GpamVaultFile* self, const char* vault_name);
	bool (*add_site)(struct GpamVaultFile* self,
					 const char* vault_name,
					 const char* site_name);

	bool (*add_record_field)(struct GpamVaultFile* self,
							 const char* vault_name,
							 const char* site_name,
							 int record_position,
							 const char* record_field_name,
							 const char* record_field_value);

	bool (*is_site_exists)(struct GpamVaultFile* self,
						   const char* vault_name,
						   const char* site_name);

	int (*add_record)(struct GpamVaultFile* self,
					  const char* vault_name,
					  const char* site_name,
					  const char* login);

	size_t (*get_sites_count)(struct GpamVaultFile* self,
						   	  const char* vault_name);

	size_t (*get_record_fields_count)(struct GpamVaultFile* self,
									  const char* vault_name,
									  const char* site_name,
									  const char* login);

	int (*get_record_position)(struct GpamVaultFile* self,
							   const char* vault_name,
							   const char* site_name,
							   const char* login);

	char* (*get_default_vault)(struct GpamVaultFile* self);
	char* (*get_site_name)(struct GpamVaultFile* self,
						   const char* vault_name,
						   size_t site_position);
	char* (*get_record_field_name)(struct GpamVaultFile* self,
								   const char* vault_name,
								   const char* site_name,
								   size_t record_position,
								   size_t record_field_position);

	char* (*get_record_field_value)(struct GpamVaultFile* self,
								   	const char* vault_name,
								   	const char* site_name,
								   	size_t record_position,
								   	size_t record_field_position);

	char* (*get_master_key)(struct GpamVaultFile* self);

	bool (*delete_site)(struct GpamVaultFile* self,
						const char* vault_name,
						const char* site_name);

	bool (*delete_record)(struct GpamVaultFile* self,
						  const char* vault_name,
						  const char* site_name,
						  size_t record_position);
};

struct GpamVaultFile* gpam_vault_file_new(const char* path);
void gpam_vault_file_destroy(struct GpamVaultFile* self);
/* ================================= */

#endif
