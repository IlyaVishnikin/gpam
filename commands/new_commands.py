import click

from pwinput import pwinput
from os.path import expanduser, join, abspath

from config 				import GPAM_DIRECTORY_LOCATION, GPAM_CONFIGURATION_FILE_LOCATION
from libs.ConfigurationFile import ConfigurationFile
from libs.VaultFile 		import VaultFile
from libs.crypto			import aes_encrypt

@click.group(help="Add new vaults and records")
def new(): ...


@new.command(help="Add and initialize the new vault item")
@click.option("-n", "--name", prompt="Vault name", type=str, help="New name of the vault")
@click.option("-p", "--path", type=click.Path(exists=False, dir_okay=False, writable=True, readable=True), help="Path to the vault")
@click.option("-k", "--key", type=str, default="", help="Master key")
@click.option("--verbose/--no-verbose", type=bool, default=False, help="Verbose output")
def vault(name: str, path: str, key: str, verbose: bool):
	path = abspath(expanduser(path)) if path else join(GPAM_DIRECTORY_LOCATION, f"vault.{name}.json")
	
	vault_file = VaultFile(path)
	if verbose: print(f"GPAM: Initialize a new vault file: {path}.")

	configuration_file = ConfigurationFile(GPAM_CONFIGURATION_FILE_LOCATION)
	if verbose: print(f"GPAM: Initialize configuration file: {GPAM_CONFIGURATION_FILE_LOCATION}.")

	configuration_file.add_vault(name, path, key)
	if verbose: print(f"GPAM: Adding the new vault({name}:{path}) to the configuration file.")

	vault_file.save()
	if verbose: print("GPAM: Saving the vault file.")
	
	configuration_file.save()
	if verbose: print("GPAM: Saving the configuration file")


@new.command(help="Add a new alias for the vault")
@click.option("-n", "--name", prompt="Vault name", type=str, help="Vault name")
@click.option("-a", "--alias", prompt="Vault alias", type=str, help="Vault alias")
@click.option("--verbose/--no-verbose", type=bool, default=False, help="Verbose output")
def alias(name: str, alias: str, verbose: bool):
	configuration_file = ConfigurationFile(GPAM_CONFIGURATION_FILE_LOCATION)
	if verbose: print(f"GPAM: Initialize configuration file: {GPAM_CONFIGURATION_FILE_LOCATION}.")

	configuration_file.add_vault_alias(name, alias)
	if verbose: print(f"GPAM: Adding alias \"{alias}\" for the vault: {name}.")

	configuration_file.save()
	if verbose: print("GPAM: Saving the configuration file")


@new.command(help="Add a new record")
@click.option("-v", "--vault", type=str, help="Vault name [not used if \"default-vault\" is set]")
@click.option("-s", "--site", type=str, help="Site [-f site PASSWORD]")
@click.option("-l", "--login", type=str, help="Login [-f login LOGIN]")
@click.option("-p", "--password", type=str, hide_input=True, confirmation_prompt=True, help="Password [-f password PASSWORD]")
@click.option("-f", "--field", type=click.Tuple([str, str]), multiple=True, help="General purpose field")
@click.option("-i", "--interactive", is_flag=True, type=bool, default=False, help="Interactive input for general purpose fields")
@click.option("-c", "--confirm", is_flag=True, type=bool, default=False, help="Asking for password confirmation")
@click.option("-e", "--encrypt", is_flag=True, type=bool, default=True, help="Encrypt password or not")
def record(vault: str,
		   site: str,
		   login: str,
		   password: str,
		   field: [str, str],
		   interactive: bool,
		   confirm: bool,
		   encrypt: bool) -> None:

	configuration_file = ConfigurationFile(GPAM_CONFIGURATION_FILE_LOCATION)

	if not vault:
		if configuration_file.data["configuration"]["default-vault"]:
			vault = configuration_file.data["configuration"]["default-vault"]
		else:
			all_vaults = configuration_file.get_all_vaults()
			vault = click.prompt("Vault", type=click.Choice(all_vaults), show_choices=len(all_vaults) <= 5)

	site = site if site else click.prompt("Site", type=str)
	login = login if login else click.prompt("Login", type=str)
	password = password if password else pwinput("Password: ")
	if confirm and not password == pwinput("Repeat for confirmation: "):
		click.echo(f"GPAM: {click.style('Password confirmation failed', fg='red')}.")
		return

	field = dict(zip(field)) if field else {}
	if interactive:
		while True:
			field_name = click.prompt("Field name", type=str, default="", show_default=False)
			if not field_name:
				break

			field_value = click.prompt("Field value", type=str, default="", show_default=False)
			if not field_value:
				break

			field[field_name] = field_value

	vault_path = configuration_file.get_vault_path(vault)
	if not vault_path:
		click.echo(f"GPAM: {click.style(f'Path for the vault {vault} is not set', fg='red')}.")
		return

	all_fields = field.copy()
	all_fields['login'] = login
	all_fields['site'] = site
	all_fields['password'] = password
	all_fields['encrypted'] = bool(encrypt and configuration_file.get_master_key(vault))

	if all_fields['encrypted']:
		all_fields['password'] = aes_encrypt(all_fields['password'], configuration_file.get_master_key(vault))

	vault_file = VaultFile(vault_path)
	vault_file.add_fields(**all_fields)

	vault_file.save()
	configuration_file.save()
