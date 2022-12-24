#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libxml/parser.h>
#include <libxml/xpath.h>

#include "types.h"


const char* GPAM_RECORD_DEFAULT_DELIMITER = "=";
const size_t GPAM_RECORD_LIST_INITIAL_CAPACITY = 4;
const xmlChar* GPAM_XML_BOOLEAN_TRUE = "yes";


/* ===== GpamRecord ===== */
static bool
gpam_record_set_name(struct GpamRecord* self, const char* name)
{
	if (!self || !name) return false;

	free(self->name);
	self->name = strdup(name);
	return true;
}

static bool
gpam_record_set_value(struct GpamRecord* self, const char* value)
{
	if (!self || !value) return false;

	free(self->value);
	self->value = strdup(value);
	return true;
}

struct GpamRecord*
gpam_record_new(const char* name, const char* value)
{
	struct GpamRecord* self = malloc(sizeof(struct GpamRecord));

	self->set_name 	= gpam_record_set_name;
	self->set_value = gpam_record_set_value;

	self->name 	= strdup(name);
	self->value = strdup(value);
	
	return self;
}

struct GpamRecord*
gpam_record_new_from_string(const char* string, const char* delimiter)
{
	char* string_copy = strdup(string);
	char* name  = strtok(string_copy, delimiter ? delimiter : GPAM_RECORD_DEFAULT_DELIMITER);
	char* value = strtok(NULL, "");

	struct GpamRecord* new_record = (strcmp(name, "") == 0 || strcmp(value, "") == 0) ? NULL : gpam_record_new(name, value);
	free(string_copy);
	return new_record;
}

bool
gpam_record_destroy(struct GpamRecord* self)
{
	if (!self) return false;

	free(self->name);
	free(self->value);
	free(self);
}
/* ====================== */

/* ===== GpamRecordList ===== */
static bool
gpam_record_list_set_capacity(struct GpamRecordList* self,
							  size_t new_capacity)
{
	if (!self) return false;

	new_capacity = new_capacity <= self->size ? self->size : new_capacity;
	struct GpamRecord** new_data = malloc(sizeof(struct GpamRecord*) * new_capacity);
	if (!new_data) return false;

	for (int i = 0; i < self->size; i++)
		new_data[i] = self->data[i];

	free(self->data);
	self->data = new_data;
	self->capacity = new_capacity;
	return true;
}

static bool
gpam_record_list_add(struct GpamRecordList* self,
					 struct GpamRecord* 	record)
{
	if (!self || !record) return false;

	/* If @record is outside of the range of @self->records 
	 * and we can't double capacity of @self->records.*/
	if (self->size >= self->capacity && !self->set_capacity(self, self->capacity * 2))
		return false;

	self->data[self->size++] = record;
	return true;
}

static bool
gpam_record_list_unique(struct GpamRecordList* self)
{
	if (!self) return false;

	size_t nullable_records = 0;
	for (int i = self->size-1; i >= 0; i--)
		for (int j = 0; j < i; j++)
			if (self->data[i] &&
				self->data[j] &&
				strcmp(self->data[i]->name, self->data[j]->name) == 0)
			{
				gpam_record_destroy(self->data[j]);
				self->data[j] = NULL;
				nullable_records++;
			}

	size_t new_data_size = self->size - nullable_records;
	if (new_data_size == self->size) return true;

	struct GpamRecord** new_data = malloc(sizeof(struct GpamRecord*) * new_data_size);
	size_t current_record = 0;
	for (int i = 0; i < self->size; i++)
		if (self->data[i])
			new_data[current_record++] = self->data[i];

	free(self->data);
	self->data = new_data;
	self->size = new_data_size;
	return true;
}

static struct GpamRecord*
gpam_record_list_get(struct GpamRecordList* self,
					 const char* record_name)
{
	if (!self || !record_name) return NULL;

	for (int i = 0; i < self->size; i++)
		if (strncmp(self->data[i]->name, record_name, strlen(record_name)) == 0)
			return self->data[i];

	return NULL;
}

struct GpamRecordList*
gpam_record_list_new(struct GpamRecord** data, size_t size)
{
	struct GpamRecordList* self = malloc(sizeof(struct GpamRecordList));

	self->set_capacity = gpam_record_list_set_capacity;
	self->add 		   = gpam_record_list_add;
	self->get 		   = gpam_record_list_get;
	self->unique 	   = gpam_record_list_unique;

	self->size 	   = data ? size : 0;
	self->capacity = data ? size : GPAM_RECORD_LIST_INITIAL_CAPACITY;
	self->data     = malloc(sizeof(struct GpamRecord*) * self->capacity);

	if (data)
		for (int i = 0; i < size; i++)
			self->data[i] = data[i];

	return self;
}

bool
gpam_record_list_destroy(struct GpamRecordList* self)
{
	if (!self) return false;

	for (int i = 0; i < self->size; i++) gpam_record_destroy(self->data[i]);
	free(self->data);
	free(self);

	return true;
}
/* ========================== */

/* ===== GpamArguments ===== */
struct GpamArguments*
gpam_arguments_new(void)
{
	struct GpamArguments* self = malloc(sizeof(struct GpamArguments));

	self->records = gpam_record_list_new(NULL, 0);

	self->site 	= NULL;
	self->login = NULL;
	self->is_interactive_mode = false;
	self->is_command_new	  = false;
	self->is_command_update	  = false;
	self->is_command_delete	  = false;
	self->is_command_list	  = false;

	self->is_show_default_vault  = false;
	self->new_vault_name 		 = NULL;
	self->new_default_vault_name = NULL;

	self->arg_num = 0;

	return self;
}

void
gpam_arguments_destroy(struct GpamArguments* self)
{
	if (!self) return;

	gpam_record_list_destroy(self->records);

	free(self->new_vault_name);
	free(self->new_default_vault_name);
	free(self);
}
/* ========================= */

/* ===== GpamAction ===== */
enum GpamAction
gpam_action_get_action(const struct GpamArguments* gpam_arguments)
{
	/* toplevel actions */
	if (gpam_arguments->new_vault_name) 		return GPAM_ACTION_ADD_VAULT;
	if (gpam_arguments->new_default_vault_name) return GPAM_ACTION_SET_DEFAULT_VAULT;
	if (gpam_arguments->is_show_default_vault) 	return GPAM_ACTION_SHOW_DEFAULT_VAULT;
	if (gpam_arguments->new_master_key_value)	return GPAM_ACTION_SET_MASTER_KEY;

	bool site_specified   		 = gpam_arguments->records->get(gpam_arguments->records, "site")  != NULL,
		 login_specified  		 = gpam_arguments->records->get(gpam_arguments->records, "login") != NULL,
		 others_specified 		 = gpam_arguments->records->size > 2;

	bool i = gpam_arguments->is_interactive_mode,
		 n = gpam_arguments->is_command_new,
		 u = gpam_arguments->is_command_update,
		 l = gpam_arguments->is_command_list,
		 d = gpam_arguments->is_command_delete;

	if (!site_specified && !login_specified && !others_specified)
	{
		if 		(d && i) return GPAM_ACTION_DELETE_SITE_INTERACTIVE;
		else if (u && i) return GPAM_ACTION_UPDATE_SITE_INTERACTIVE;
		else if (n && i) return GPAM_ACTION_NEW_SITE_INTERACTIVE;
		else if (l && i) return GPAM_ACTION_LIST_SITES;
		else if (d)		 return GPAM_ACTION_INVALID_DELETE_SITE_SITE_NOT_SPECIFIED;
		else if (u)		 return GPAM_ACTION_INVALID_UPDATE_SITE_SITE_NOT_SPECIFIED;
		else if (n)		 return GPAM_ACTION_INVALID_NEW_SITE_SITE_NOT_SPECIFIED;
		else if (l)		 return GPAM_ACTION_LIST_SITES;
		else 			 return GPAM_ACTION_UNKNOWN;
	}
	else if (site_specified && !login_specified && !others_specified)
	{
		if 		(d && i) return GPAM_ACTION_DELETE_SITE;
		else if (u && i) return GPAM_ACTION_UPDATE_SITE_INTERACTIVE;
		else if (n && i) return GPAM_ACTION_NEW_SITE;
		else if (l && i) return GPAM_ACTION_LIST_SITES;
		else if (d)		 return GPAM_ACTION_DELETE_SITE;
		else if (u)		 return GPAM_ACTION_INVALID_UPDATE_SITE_NEW_SITE_NOT_SPECIFIED;
		else if (n)		 return GPAM_ACTION_NEW_SITE;
		else if (l)		 return GPAM_ACTION_LIST_SITES;
		else 			 return GPAM_ACTION_UNKNOWN;
	}
	else if (site_specified && login_specified && !others_specified)
	{
		if 		(d && i) return GPAM_ACTION_DELETE_RECORD_FULL;
		else if (u && i) return GPAM_ACTION_UPDATE_RECORD_INTERCATIVE;
		else if (n && i) return GPAM_ACTION_NEW_RECORD_INTERACTIVE;
		else if (l && i) return GPAM_ACTION_LIST_RECORD;
		else if (d)		 return GPAM_ACTION_DELETE_RECORD_FULL;
		else if (u)		 return GPAM_ACTION_INVALID_UPDATE_RECORD_FIELDS_NOT_SPECIFIED;
		else if (n)		 return GPAM_ACTION_INVALID_NEW_RECORD_FIELDS_NOT_SPECIFIED;
		else if (l)		 return GPAM_ACTION_LIST_RECORD;
		else 			 return GPAM_ACTION_UNKNOWN;
	}
	else if (site_specified && login_specified && others_specified)
	{
		if 		(d && i) return GPAM_ACTION_DELETE_RECORD_FULL;
		else if (u && i) return GPAM_ACTION_UPDATE_RECORD_INTERCATIVE;
		else if (n && i) return GPAM_ACTION_NEW_RECORD_INTERACTIVE;
		else if (l && i) return GPAM_ACTION_LIST_RECORD;
		else if (d)		 return GPAM_ACTION_DELETE_RECORD_FULL;
		else if (u)		 return GPAM_ACTION_UPDATE_RECORD_SPECIFIED_FIELDS;
		else if (n)		 return GPAM_ACTION_NEW_RECORD_SPECIFIED_FIELDS;
		else if (l)		 return GPAM_ACTION_LIST_RECORD;
		else 			 return GPAM_ACTION_UNKNOWN;
	}
	else
	{
		return GPAM_ACTION_UNKNOWN;
	}
}

bool
gpam_action_is_action_invalid(enum GpamAction action)
{
	return action >= 0 && action <= 6;
}
/* ====================== */

/* ===== GpamXMLFile ===== */
static bool
gpam_xml_file_is_empty(struct GpamXMLFile* self)
{
	if (!self) return false;

	FILE* xml_file = fopen(self->path, "r");
	fseek(xml_file, 0, SEEK_END);
	bool is_empty = ftell(xml_file) == 0;
	fclose(xml_file);
	return is_empty;
}

static bool
gpam_xml_file_clear(struct GpamXMLFile* self)
{
	if (!self) return false;

	FILE* xml_file = fopen(self->path, "w");
	if (!xml_file) return false;
	fclose(xml_file);
	return true;
}

static bool
gpam_xml_file_puts(struct GpamXMLFile* self,
				   const char* string)
{
	if (!self || !string) return false;

	FILE* file = fopen(self->path, "a");
	if (!file) return false;

	fputs(string, file);
	fclose(file);
	return true;
}

static bool
gpam_xml_file_parse(struct GpamXMLFile* self)
{
	if (!self) return false;

	xmlDocPtr xml_document = xmlParseFile(self->path);
	if (!xml_document)
	{
		self->xml_document = NULL;
		return false;
	}

	self->xml_document = xml_document;
	return true;
}

static bool
gpam_xml_file_close(struct GpamXMLFile* self)
{
	if (!self) return false;

	xmlFreeDoc(self->xml_document);
	self->xml_document = NULL;
	return true;
}

static bool
gpam_xml_file_save(struct GpamXMLFile* self)
{
	if (!self || !self->xml_document) return false;

	xmlSaveFormatFile(self->path, self->xml_document, true);
	return self->close(self);
}

static xmlXPathObjectPtr
gpam_xml_file_get_nodes(struct GpamXMLFile* self,
						  const char* xpath,
						  ...)
{
	if (!self || !xpath) return NULL;

	xmlXPathContextPtr xpath_context = xmlXPathNewContext(self->xml_document);

	/* formatting xpath expression */
	va_list xpath_arguments;
	va_start(xpath_arguments, xpath);
	size_t xpath_formatted_size = vsnprintf(NULL, 0, xpath, xpath_arguments) + 1;
	va_end(xpath_arguments);

	va_start(xpath_arguments, xpath);
	char* xpath_formatted = malloc(xpath_formatted_size);
	vsprintf(xpath_formatted, xpath, xpath_arguments);
	va_end(xpath_arguments);


	xmlXPathObjectPtr xpath_result = xmlXPathEvalExpression((const xmlChar*)xpath_formatted, xpath_context);
	xmlXPathFreeContext(xpath_context);
	free(xpath_formatted);
	if (!xpath_result) return NULL;
	if (xmlXPathNodeSetIsEmpty(xpath_result->nodesetval))
	{
		xmlXPathFreeObject(xpath_result);
		return NULL;
	}

	return xpath_result;
}

struct GpamXMLFile*
gpam_xml_file_new(const char* xml_file_path)
{
	if (!xml_file_path) return NULL;

	/* checking that @xml_file_path exists */
	FILE* xml_file = fopen(xml_file_path, "a");
	if (!xml_file) return NULL;
	fclose(xml_file);

	struct GpamXMLFile* self = malloc(sizeof(struct GpamXMLFile));
	self->is_empty 	  = gpam_xml_file_is_empty;
	self->clear		  = gpam_xml_file_clear;
	self->puts 		  = gpam_xml_file_puts;
	self->parse 	  = gpam_xml_file_parse;
	self->close 	  = gpam_xml_file_close;
	self->save 		  = gpam_xml_file_save;
	self->get_nodes   = gpam_xml_file_get_nodes;

	self->path 		   = strdup(xml_file_path);
	self->xml_document = NULL;

	return self;
}

void
gpam_xml_file_destroy(struct GpamXMLFile* self)
{
	if (!self) return;

	if (self->xml_document) xmlFreeDoc(self->xml_document);
	free(self->path);
	free(self);
}
/* ======================= */

/* ===== GpamVaultFile ===== */
static bool
gpam_vault_file_initialize(struct GpamVaultFile* self)
{
	if (!self) return false;
	if (!self->file->is_empty(self->file)) return true;

	return self->file->puts(self->file, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
								 		"<vaults master-key=\"\">\n"
								 		"</vaults>\n");
}

static bool
gpam_vault_file_set_default_vault(struct GpamVaultFile* self,
					   		 const char* vault_name)
{
	if (!self || (!self->file->xml_document && !self->file->parse(self->file)))
		return false;

	xmlXPathObjectPtr xpath_default_vaults = self->file->get_nodes(self->file,
																   "//vault[@default='%s' and not(@name='%s')]",
																   GPAM_XML_BOOLEAN_TRUE,
																   vault_name);
	if (xpath_default_vaults)
	{
		for (int i = 0; i < xpath_default_vaults->nodesetval->nodeNr; i++)
			xmlUnsetProp(xpath_default_vaults->nodesetval->nodeTab[i], "default");
		xmlXPathFreeObject(xpath_default_vaults);
	}

	xmlXPathObjectPtr xpath_vault = self->file->get_nodes(self->file, "//vault[@name=\"%s\"]", vault_name);
	if (!xpath_vault)
	{
		self->file->close(self->file);
		return false;
	}

	xmlNodePtr vault_node = xpath_vault->nodesetval->nodeTab[0];
	if (!xmlSetProp(vault_node, "default", GPAM_XML_BOOLEAN_TRUE))
	{
		xmlXPathFreeObject(xpath_vault);
		self->file->close(self->file);
		return false;
	}

	xmlXPathFreeObject(xpath_vault);
	self->file->save(self->file);
	return true;
}

static bool
gpam_vault_file_add_vault(struct GpamVaultFile* self,
					 const char* vault_name)
{
	if (!self || (!self->file->xml_document && !self->file->parse(self->file)))
		return false;

	xmlXPathObjectPtr xpath_vault = self->file->get_nodes(self->file, "//vault[@name='%s']", vault_name);
	
	/* when vault already exists */
	if (xpath_vault)
	{
		xmlXPathFreeObject(xpath_vault);
		self->file->close(self->file);
		return true;
	}

	xmlNodePtr new_vault = xmlNewTextChild(xmlDocGetRootElement(self->file->xml_document), NULL, "vault", "");
	if (!new_vault)
	{
		self->file->close(self->file);
		return false;
	}

	if (!xmlSetProp(new_vault, "name", vault_name))
	{
		self->file->close(self->file);
		return false;
	}

	self->file->save(self->file);
	return true;
}

static bool
gpam_vault_file_add_site(struct GpamVaultFile* self,
					const char* vault_name,
					const char* site_name)
{
	if (!self ||
		!vault_name ||
		!site_name ||
		(!self->file->xml_document && !self->file->parse(self->file)))
		return false;

	/* checks if site already exists */
	xmlXPathObjectPtr xpath_site = self->file->get_nodes(self->file,
														 "//vault[@name=\"%s\"]/site[@name=\"%s\"]",
														 vault_name,
														 site_name);
	if (xpath_site)
	{
		xmlXPathFreeObject(xpath_site);
		self->file->close(self->file);
		return true;
	}

	xmlXPathObjectPtr xpath_vault = self->file->get_nodes(self->file, "//vault[@name=\"%s\"]", vault_name);
	if (!xpath_vault)
	{
		self->file->close(self->file);
		return false;
	}

	xmlNodePtr vault_node = xmlNewTextChild(xpath_vault->nodesetval->nodeTab[0], NULL, "site", "");
	if (!vault_node)
	{
		xmlXPathFreeObject(xpath_vault);
		self->file->close(self->file);
		return false;
	}

	if (!xmlSetProp(vault_node, "name", site_name))
	{
		xmlXPathFreeObject(xpath_vault);
		self->file->close(self->file);
		return false;
	}

	xmlXPathFreeObject(xpath_vault);
	self->file->save(self->file);
	return true;
}

static int
gpam_vault_file_add_record(struct GpamVaultFile* self,
						   const char* vault_name,
						   const char* site_name,
						   const char* login)
{
	if (!self 		||
		!vault_name ||
		!site_name  ||
		!login 		||
		(!self->file->xml_document && !self->file->parse(self->file)))
		return -1;

	xmlXPathObjectPtr xpath_record = self->file->get_nodes(self->file,
														   "//vault[@name='%s']/site[@name='%s']/record/record-field[@name='login' and text()='%s']",
														   vault_name,
														   site_name,
														   login);
	if (xpath_record)
	{
		int record_position = xmlChildElementCount(xpath_record->nodesetval->nodeTab[0]->parent->parent);
		xmlXPathFreeObject(xpath_record);
		self->file->close(self->file);
		return record_position;
	}

	xpath_record = self->file->get_nodes(self->file,
										 "//vault[@name='%s']/site[@name='%s']",
										 vault_name,
										 site_name);
	if (!xpath_record)
	{
		self->file->close(self->file);
		return -1;
	}

	xmlNodePtr record_node = xmlNewTextChild(xpath_record->nodesetval->nodeTab[0], NULL, "record", "");
	if (!record_node)
	{
		xmlXPathFreeObject(xpath_record);
		self->file->close(self->file);
		return -1;
	}

	int record_position = xmlChildElementCount(record_node->parent);
	xmlXPathFreeObject(xpath_record);
	self->file->save(self->file);
	return record_position;
}

static bool
gpam_vault_file_add_record_field(struct GpamVaultFile* self,
								 const char* vault_name,
								 const char* site_name,
								 int record_position,
								 const char* record_field_name,
								 const char* record_field_value)
{
	if (!self 				||
		!vault_name 		||
		!site_name  		||
		record_position < 0	||
		!record_field_name	||
		!record_field_value ||
		(!self->file->xml_document && !self->file->parse(self->file)))
		return false;

	xmlXPathObjectPtr xpath_record = self->file->get_nodes(self->file,
														   "//vault[@name='%s']/site[@name='%s']/record[position()=%d]/record-field[@name='%s']",
														   vault_name,
														   site_name,
														   record_position,
														   record_field_name,
														   record_field_value);

	if (xpath_record)
	{
		xmlNodePtr record_field_node = xpath_record->nodesetval->nodeTab[0];
		xmlNodeSetContent(record_field_node, record_field_value);
		xmlXPathFreeObject(xpath_record);
		self->file->save(self->file);
		return true;
	}

	xpath_record = self->file->get_nodes(self->file,
										 "//vault[@name='%s']/site[@name='%s']/record[%d]",
										 vault_name,
										 site_name,
										 record_position);
	if (!xpath_record)
	{
		printf("1\n");
		self->file->close(self->file);
		return false;
	}

	xmlNodePtr new_field_record_node = xmlNewTextChild(xpath_record->nodesetval->nodeTab[0], NULL, "record-field", record_field_value);
	if (!new_field_record_node)
	{
		printf("2\n");
		xmlXPathFreeObject(xpath_record);
		self->file->close(self->file);
		return false;
	}

	if (!xmlSetProp(new_field_record_node, "name", record_field_name))
	{
		printf("3\n");
		xmlXPathFreeObject(xpath_record);
		self->file->close(self->file);
		return false;
	}

	xmlXPathFreeObject(xpath_record);
	self->file->save(self->file);
	return true;
}

static bool
gpam_vault_file_is_site_exists(struct GpamVaultFile* self,
							   const char* vault_name,
							   const char* site_name)
{
	if (!self 				||
		!vault_name 		||
		!site_name  		||
		(!self->file->xml_document && !self->file->parse(self->file)))
		return false;

	xmlXPathObjectPtr xpath_site = self->file->get_nodes(self->file,
														 "//vault[@name='%s']/site[@name='%s']",
														 vault_name,
														 site_name);
	if (xpath_site)
	{
		xmlXPathFreeObject(xpath_site);
		self->file->close(self->file);
		return true;
	}

	self->file->close(self->file);
	return false;
}

static bool
gpam_vault_file_set_site_name(struct GpamVaultFile* self,
							  const char* vault_name,
							  const char* site_name,
							  const char* new_site_name)
{
	if (!self 			||
		!vault_name 	||
		!site_name  	||
		!new_site_name	||
		(!self->file->xml_document && !self->file->parse(self->file)))
		return false;

	xmlXPathObjectPtr xpath_site = self->file->get_nodes(self->file,
														 "//vault[@name='%s']/site[@name='%s']",
														 vault_name,
														 site_name);

	if (!xpath_site)
	{
		xmlXPathFreeObject(xpath_site);
		self->file->close(self->file);
		return false;
	}

	bool result = xmlSetProp(xpath_site->nodesetval->nodeTab[0], "name", new_site_name);
	xmlXPathFreeObject(xpath_site);
	self->file->save(self->file);
	return result;
}

static bool
gpam_vault_file_delete_site(struct GpamVaultFile* self,
							const char* vault_name,
							const char* site_name)
{
	if (!self 		||
		!vault_name ||
		!site_name  ||
		(!self->file->xml_document && !self->file->parse(self->file)))
		return false;

	xmlXPathObjectPtr xpath_site = self->file->get_nodes(self->file,
														 "//vault[@name='%s']/site[@name='%s']",
														 vault_name,
														 site_name);

	bool result = xpath_site ? (xmlUnlinkNode(xpath_site->nodesetval->nodeTab[0]), true) : false;
	xmlXPathFreeObject(xpath_site);
	self->file->save(self->file);
	return result;
}

static bool
gpam_vault_file_delete_record(struct GpamVaultFile* self,
							  const char* vault_name,
							  const char* site_name,
							  size_t record_position)
{
	if (!self 				  ||
		!vault_name 		  ||
		!site_name  		  ||
		record_position == -1 ||
		(!self->file->xml_document && !self->file->parse(self->file)))
		return false;

	xmlXPathObjectPtr xpath_record = self->file->get_nodes(self->file,
														   "//vault[@name='%s']/site[@name='%s']/record[position()=%zu]",
														   vault_name,
														   site_name,
														   record_position);

	if (!xpath_record)
	{
		self->file->close(self->file);
		return false;
	}

	xmlUnlinkNode(xpath_record->nodesetval->nodeTab[0]);
	xmlXPathFreeObject(xpath_record);
	self->file->save(self->file);
	return true;
}

static char*
gpam_vault_file_get_default_vault(struct GpamVaultFile* self)
{
	if (!self || (!self->file->xml_document && !self->file->parse(self->file)))
		return NULL;

	xmlXPathObjectPtr xpath_default_vault = self->file->get_nodes(self->file, "//vault[@default=\"%s\"]", GPAM_XML_BOOLEAN_TRUE);
	if (!xpath_default_vault) return NULL;
	
	char* default_node = xmlGetProp(xpath_default_vault->nodesetval->nodeTab[0], "name");

	xmlXPathFreeObject(xpath_default_vault);
	self->file->close(self->file);
	return default_node;
}

static size_t
gpam_vault_file_get_sites_count(struct GpamVaultFile* self,
								const char* vault_name)
{
	if (!self 		||
		!vault_name ||
		(!self->file->xml_document && !self->file->parse(self->file)))
		return 0;

	xmlXPathObjectPtr xpath_sites = self->file->get_nodes(self->file,
														  "//vault[@name='%s']/site",
														  vault_name);

	size_t sites_count = xpath_sites ? xpath_sites->nodesetval->nodeNr : 0;
	
	xmlXPathFreeObject(xpath_sites);
	self->file->close(self->file);
	return sites_count;
}

static size_t
gpam_vault_file_get_record_fields_count(struct GpamVaultFile* self,
										const char* vault_name,
										const char* site_name,
										const char* login)
{
	if (!self 		||
		!vault_name ||
		(!self->file->xml_document && !self->file->parse(self->file)))
		return 0;

	xmlXPathObjectPtr xpath_record = self->file->get_nodes(self->file,
														   "//vault[@name='%s']/site[@name='%s']/record/record-field[@name='login' and text()='%s']/..",
														   vault_name,
														   site_name,
														   login);
	if (!xpath_record)
	{
		self->file->close(self->file);
		return 0;
	}

	size_t records_count = 0;
	xmlNodePtr record_node = xpath_record->nodesetval->nodeTab[0]->xmlChildrenNode;
	while (record_node)
	{
		if (record_node->type == XML_ELEMENT_NODE) records_count++;
		record_node = record_node->next;
	}
	xmlXPathFreeObject(xpath_record);
	self->file->close(self->file);
	return records_count;
}

static int
gpam_vault_file_get_record_position(struct GpamVaultFile* self,
									const char* vault_name,
									const char* site_name,
									const char* login)
{
	if (!self 		||
		!vault_name ||
		(!self->file->xml_document && !self->file->parse(self->file)))
		return -1;

	xmlXPathObjectPtr xpath_record = self->file->get_nodes(self->file,
														   "//vault[@name='%s']/site[@name='%s']/record/record-field[@name='login' and text()='%s']/..",
														   vault_name,
														   site_name,
														   login);
	int record_position = xpath_record ?
						  xmlChildElementCount(xpath_record->nodesetval->nodeTab[0]->parent) :
						  -1;

	xmlXPathFreeObject(xpath_record);
	self->file->close(self->file);
	return record_position;
}

static char*
gpam_vault_file_get_site_name(struct GpamVaultFile* self,
							  const char* vault_name,
							  size_t site_position)
{
	if (!self 		||
		!vault_name ||
		(!self->file->xml_document && !self->file->parse(self->file)))
		return NULL;

	xmlXPathObjectPtr xpath_site = self->file->get_nodes(self->file,
														 "//vault[@name='%s']/site[position()=%zu]",
														 vault_name,
														 site_position+1);

	char* site_name = xpath_site ? xmlGetProp(xpath_site->nodesetval->nodeTab[0], "name") : NULL;

	xmlXPathFreeObject(xpath_site);
	self->file->close(self->file);
	return site_name;
}

static char*
gpam_vault_file_get_record_field_name(struct GpamVaultFile* self,
									  const char* vault_name,
									  const char* site_name,
									  size_t record_position,
									  size_t record_field_position)
{
	if (!self 		||
		!vault_name ||
		(!self->file->xml_document && !self->file->parse(self->file)))
		return NULL;

	xmlXPathObjectPtr xpath_record_field = self->file->get_nodes(self->file,
																 "//vault[@name='%s']/site[@name='%s']/record[position()=%zu]/record-field[position()=%zu]",
																 vault_name,
																 site_name,
																 record_position,
																 record_field_position);

	char* record_name = xpath_record_field ?
						xmlGetProp(xpath_record_field->nodesetval->nodeTab[0], "name") :
						NULL;

	xmlXPathFreeObject(xpath_record_field);
	self->file->close(self->file);
	return record_name;
}

static char*
gpam_vault_file_get_record_field_value(struct GpamVaultFile* self,
									   const char* vault_name,
									   const char* site_name,
									   size_t record_position,
									   size_t record_field_position)
{
	if (!self 		||
		!vault_name ||
		(!self->file->xml_document && !self->file->parse(self->file)))
		return NULL;

	xmlXPathObjectPtr xpath_record_field = self->file->get_nodes(self->file,
																 "//vault[@name='%s']/site[@name='%s']/record[position()=%zu]/record-field[position()=%zu]",
																 vault_name,
																 site_name,
																 record_position,
																 record_field_position);

	char* record_value = xpath_record_field ?
						 xmlNodeGetContent(xpath_record_field->nodesetval->nodeTab[0]) :
						 NULL;

	xmlXPathFreeObject(xpath_record_field);
	self->file->close(self->file);
	return record_value;
}

struct GpamVaultFile*
gpam_vault_file_new(const char* path)
{
	if (!path) return NULL;

	struct GpamVaultFile* self = malloc(sizeof(struct GpamVaultFile));

	self->file = gpam_xml_file_new(path);
	if (!self->file)
	{
		free(self);
		return NULL;
	}

	self->initialize = gpam_vault_file_initialize;

	self->is_site_exists = gpam_vault_file_is_site_exists;

	self->set_default_vault = gpam_vault_file_set_default_vault;
	self->set_site_name 	= gpam_vault_file_set_site_name;

	self->add_vault 	   = gpam_vault_file_add_vault;
	self->add_site 		   = gpam_vault_file_add_site;
	self->add_record 	   = gpam_vault_file_add_record;
	self->add_record_field = gpam_vault_file_add_record_field;

	self->get_default_vault 	  = gpam_vault_file_get_default_vault;	
	self->get_sites_count 		  = gpam_vault_file_get_sites_count;
	self->get_site_name 		  = gpam_vault_file_get_site_name;
	self->get_record_fields_count = gpam_vault_file_get_record_fields_count;
	self->get_record_position 	  = gpam_vault_file_get_record_position;
	self->get_record_field_name   = gpam_vault_file_get_record_field_name;
	self->get_record_field_value  = gpam_vault_file_get_record_field_value;

	self->delete_site 	= gpam_vault_file_delete_site;
	self->delete_record = gpam_vault_file_delete_record;

	return self;
}

void
gpam_vault_file_destroy(struct GpamVaultFile* self)
{
	if (!self) return;

	gpam_xml_file_destroy(self->file);
	free(self);
}
/* ================================= */
