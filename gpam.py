import click
import sys

import config

from pathlib import Path
from os.path import join
from pwinput import pwinput
from lib.ConfigurationFile import ConfigurationFile
from lib.VaultFile import VaultFile

def gpam_create_new_vault(name: str = "", master_key: str = "", path: str = "") -> None:
	config_file = ConfigurationFile(config.CONFIG_FILE_PATH)
	if not config_file.data:
		click.echo(f"GPAM: {click.style('Configuration file has been damaged.', fg='red')}", file=click.get_text_stream("stderr"))
		sys.exit(config.EXIT_FAILURE)

	name = name if name else click.prompt("Vault name", type=str)
	if not master_key:
		master_key = pwinput("Master key: ")
		if master_key != pwinput("Repeat master key: "):
			click.echo(f"GPAM: {click.style('Master key confirmation failed', fg='red')}", file=click.get_text_stream("stderr"))
			sys.exit(config.EXIT_FAILURE)

	if not path:
		path = click.prompt(
			"Vault location",
			type=click.Path(exists=False, file_okay=True, dir_okay=False, writable=True, readable=True, allow_dash=False, path_type=Path),
			default=join(config.GPAM_HOME_DIR, f"{name}.vault.json")
		).resolve()
	else:
		path = Path(path).resolve()


	config_file.add_vault(name, str(path))
	vault_file = VaultFile(str(path), master_key)
	if not vault_file.data:
		click.echo(f"GPAM: {click.style('Error while initializing vault file', fg='red')}", file=click.get_text_stream("stderr"))
		sys.exit(config.EXIT_FAILURE)

	vault_file.save()
	config_file.save()

@click.group()
def gpam(): ...


@gpam.command()
@click.option("--vault", "-v", type=str, help="Vault name(if not set than uses default vault)")
@click.option("--login", "-l", type=str, help="New login name")
@click.option("--site", "-s", type=str, help="New site")
@click.option("--password", "-p", type=str, help="New password")
@click.option("--field", "-f", type=(str, str), default=[], multiple=True, help="Custome field")
@click.option("--master-key", "-m", type=str, help="Master key")
@click.option("--interactive", "-i", is_flag=True, help="Interactive input of fields")
def new(vault, login, site, password, field, master_key, interactive):
	config_file = ConfigurationFile(config.CONFIG_FILE_PATH)
	if not config_file.data:
		click.echo(f"GPAM: {click.style('Configuration file has been damaged.', fg='red')}", file=click.get_text_stream("stderr"))
		sys.exit(config.EXIT_FAILURE)

	vault_file = None

	if not config_file.vaults:
		click.echo(f"GPAM: {click.style('No one vault initialized yet', fg='yellow')}")
		gpam_create_new_vault(vault, master_key)
		click.echo(f"GPAM: {click.style('New vault has been initialized', fg='green')}")
		if not site:
			sys.exit(config.EXIT_SUCCESS)
		vault_file = VaultFile(config_file.get_vault_path(vault), master_key)

	if not vault:
		vault = config_file.default_vault
		click.echo(f"GPAM: Used default vault: \"{vault}\"")
	elif vault not in config_file.get_all_vault_names():
		is_create = click.confirm(f"GPAM: Vault with name \"{vault}\" not exists. Create?", default=True)
		if not is_create:
			click.echo(f"GPAM: {click.style('Aborted by user', fg='red')}", file=click.get_text_stream("stderr"))
			sys.exit(config.EXIT_SUCCESS)

		gpam_create_new_vault(vault, master_key)
		click.echo(f"GPAM: {click.style('New vault has been initialized', fg='green')}")

		if not site:
			sys.exit(config.EXIT_SUCCESS)
		vault_file = VaultFile(config_file.get_vault_path(vault), master_key)
	
	fields = { k: v for k, v in field }
	if "site" not in fields:
		fields["site"] = site if site else click.prompt("Site")
	if "login" not in fields:
		fields["login"] = login if login else click.prompt("Login")
	if "password" not in fields:
		fields["password"] = password if password else pwinput("Password: ")

	vault_path = config_file.get_vault_path(vault)
	if not vault_path:
		error_message = click.style("Path for the vault is not set")
		click.echo(f"GPAM: {error_message}: {vault}", file=click.get_text_stream("stderr"))
		sys.exit(EXIT_FAILURE)

	if not vault_file:
		if not master_key:
			master_key = pwinput("Master key: ")
			if master_key != pwinput("Repeat master key: "):
				click.echo(f"GPAM: {click.style('Master key confirmation failed', fg='red')}", file=click.get_text_stream("stderr"))
				sys.exit(config.EXIT_FAILURE)
		vault_file = VaultFile(vault_path, master_key)

	if not vault_file.data:
		click.echo(f"GPAM: {click.style('Vault file has been damaged or master key incorrect', fg='red')}", file=click.get_text_stream("stderr"))
		sys.exit(config.EXIT_FAILURE)


if __name__ == "__main__":
	Path(config.GPAM_HOME_DIR).mkdir(mode=0o777, parents=True, exist_ok=True)
	gpam()
