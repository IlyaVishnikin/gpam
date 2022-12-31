import sys
import click
import config
import lib.crypto as crypto

from pathlib import Path
from os.path import join
from pwinput import pwinput
from rich.console import Console
from rich.table import Table
from rich.tree import Tree
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
	return [name, master_key, path]

def ask_vault_path(vault_name: str = "") -> str:
	config_file = ConfigurationFile(config.CONFIG_FILE_PATH)
	if not config_file:
		click.echo(f"GPAM: {click.style('Configuration file has been damaged.', fg='red')}", file=click.get_text_stream("stderr"))
		sys.exit(config.EXIT_FAILURE)

	vault_name = vault_name if vault_name else click.prompt("Vault")
	vault_path = config_file.get_vault_path(vault_name)
	if not vault_path:
		click.echo(f"GPAM: {click.style('Path for the vault is not set', fg='red')}: {vault_name}", file=click.get_text_stream("stderr"))
		sys.exit(config.EXIT_FAILURE)
	return vault_path

def ask_master_key(confirm: bool = True) -> str:
	master_key = pwinput("Master key: ")
	if confirm and master_key != pwinput("Repeat master key: "):
		click.echo(f"GPAM: {click.style('Master key confirmation failed', fg='red')}", file=click.get_text_stream("stderr"))
		sys.exit(config.EXIT_FAILURE)
	return master_key


@click.group()
def gpam(): ...

@gpam.command()
@click.option("--vault", "-v", type=str, help="Vault name(if not set than uses default vault)")
@click.option("--login", "-l", type=str, help="New login name")
@click.option("--site", "-s", type=str, help="New site")
@click.option("--password", "-p", type=str, help="New password")
@click.option("--field", "-f", type=(str, str), default=[], multiple=True, help="Custome field")
@click.option("--master-key", "-m", type=str, help="Master key")
@click.option("--path", "-P", type=str, help="Path to the vault")
@click.option("--interactive", "-i", is_flag=True, help="Interactive input of fields")
@click.option("--generate-password", "-g", is_flag=True, help="Generate password")
def new(vault, login, site, password, field, master_key, path, interactive, generate_password):
	config_file = ConfigurationFile(config.CONFIG_FILE_PATH)
	if not config_file.data:
		click.echo(f"GPAM: {click.style('Configuration file has been damaged.', fg='red')}", file=click.get_text_stream("stderr"))
		sys.exit(config.EXIT_FAILURE)
	vault_file = None

	if not config_file.vaults:
		click.echo(f"GPAM: {click.style('No one vault initialized yet', fg='yellow')}")
		vault, master_key, path = gpam_create_new_vault(vault, master_key, path)
		click.echo(f"GPAM: {click.style('New vault has been initialized', fg='green')}")
		if not site:
			sys.exit(config.EXIT_SUCCESS)
		vault_file = VaultFile(path, master_key)
		config_file.read()
	elif not vault:
		vault = config_file.default_vault
		click.echo(f"GPAM: Used default vault: \"{vault}\"")
	elif vault not in config_file.get_all_vault_names():
		is_create = click.confirm(f"GPAM: Vault with name \"{vault}\" not exists. Create?", default=True)
		if not is_create:
			click.echo(f"GPAM: {click.style('Aborted by user', fg='red')}", file=click.get_text_stream("stderr"))
			sys.exit(config.EXIT_SUCCESS)
		vault, master_key, path = gpam_create_new_vault(vault, master_key, path)
		click.echo(f"GPAM: {click.style('New vault has been initialized', fg='green')}")
		if not site:
			sys.exit(config.EXIT_SUCCESS)
		vault_file = VaultFile(path, master_key)
		config_file.read()

	fields = { k: v for k, v in field }
	if "site" not in fields:
		fields["site"] = site if site else click.prompt("Site")
	if "login" not in fields:
		fields["login"] = login if login else click.prompt("Login")
	if "password" not in fields:
		fields["password"] = password or (generate_password and crypto.generate_password(15)) or pwinput("Password: ")

	path = path if path else ask_vault_path(vault)
	if not vault_file:
		master_key = master_key if master_key else ask_master_key(confirm=False)
		vault_file = VaultFile(path, master_key)

	if not vault_file.data:
		click.echo(f"GPAM: {click.style('Vault file has been damaged or master key incorrect', fg='red')}", file=click.get_text_stream("stderr"))
		sys.exit(config.EXIT_FAILURE)

	vault_file.add_record(**fields)
	vault_file.save()
	config_file.save()

@gpam.command()
@click.argument("vault", default="", type=str)
@click.option("--master-key", "-m", type=str, help="Master key")
def show(vault, master_key):
	config_file = ConfigurationFile(config.CONFIG_FILE_PATH)
	if not config_file.data:
		click.echo(f"GPAM: {click.style('Configuration file has been damaged.', fg='red')}", file=click.get_text_stream("stderr"))
		sys.exit(config.EXIT_FAILURE)

	if not vault:
		all_vaults = config_file.get_all_vault_names()
		if not all_vaults:
			click.echo("No available vaults")
			sys.exit(config.EXIT_SUCCESS)

		table = Table(title="GPAM Vaults Information")
		table.add_column("Name")
		table.add_column("Path")
		for name in all_vaults:
			table.add_row(name, config_file.get_vault_path(name) or "<Not set>")
		Console().print(table)
		sys.exit(config.EXIT_SUCCESS)

	vault_path = config_file.get_vault_path(vault)
	if not vault_path:
		error_message = click.style("Path for the vault is not set", fg="red")
		click.echo(f"GPAM: {error_message}: {vault}", file=click.get_text_stream("stderr"))
		sys.exit(config.EXIT_FAILURE)

	if not master_key:
		master_key = pwinput("Master key: ")
		if master_key != pwinput("Repeat master key: "):
			click.echo(f"GPAM: {click.style('Master key confirmation failed', fg='red')}", file=click.get_text_stream("stderr"))
			sys.exit(config.EXIT_FAILURE)

	vault_file = VaultFile(vault_path, master_key)
	if not vault_file.data:
		click.echo(f"GPAM: {click.style('Vault file has been damaged or master key incorrect', fg='red')}", file=click.get_text_stream("stderr"))
		sys.exit(config.EXIT_FAILURE)

	tree = Tree(vault)
	for record in vault_file.data["records"]:
		tree_site = tree.add(record["site"])
		for k in record:
			if k == "site":
				continue
			tree_site.add(f"{k}: {vault_file.decrypt_password(record['password']) if k == 'password' else record[k]}")

	Console().print(tree)

@gpam.command()
@click.option("--vault", "-v", type=str, multiple=True)
@click.option("--master-key", "-m", type=str, multiple=True)
@click.option("--site", "-s", type=str, multiple=True)
@click.option("--login", "-l", type=str, multiple=True)
@click.option("--password", "-p", type=str, multiple=True)
@click.option("--field", "-f", type=(str, str), multiple=True)
def update(vault, master_key, site, login, password, field):
	config_file = ConfigurationFile(config.CONFIG_FILE_PATH)
	if not config_file.data:
		click.echo(f"GPAM: {click.style('Configuration file has been damaged.', fg='red')}", file=click.get_text_stream("stderr"))
		sys.exit(config.EXIT_FAILURE)
	vault_file = None

	if login or password or field:
		fields = { f[0]: f[1] for f in field }
		
		site = site if site else (click.prompt("Site"), )
		fields["site"] = site[-1] if len(site) >= 2 else site[0]

		login = login if login else (click.prompt("Login"), )
		fields["login"] = login[-1] if len(login) >= 2 else login[0]

		if len(password) >= 2 and password[0] != password[-1]:
			fields["password"] = password[-1]

		vault_path = ask_vault_path(vault[0] if vault else "")
		master_key = master_key if master_key else (ask_master_key(confirm=False), )
		vault_file = VaultFile(vault_path, master_key[0])
		if not vault_file.data:
			click.echo(f"GPAM: {click.style('Vault file has been damaged or master key incorrect', fg='red')}", file=click.get_text_stream("stderr"))
			sys.exit(config.EXIT_FAILURE)
		print(fields)
		vault_file.update_record(site[0], login[0], fields)
		vault_file.save()

	if len(site) >= 2 and site[0] != site[-1]:
		if not vault_file:
			vault_path = ask_vault_path(vault[0] if vault else "")
			master_key = master_key if master_key else (ask_master_key(confirm=False), )
			vault_file = VaultFile(vault_path, master_key[0])
			if not vault_file.data:
				click.echo(f"GPAM: {click.style('Vault file has been damaged or master key incorrect', fg='red')}", file=click.get_text_stream("stderr"))
				sys.exit(config.EXIT_FAILURE)

		if not login and click.confirm("Update all records?", default=False):
			vault_file.update_all_sites(site[0], site[-1])
		else:
			login = click.prompt("Login")
			vault_file.update_record(site[0], login, { "site": site[-1] })
		vault_file.save()

	if len(master_key) >= 2 and master_key[0] != master_key[-1]:
		if not vault_file:
			vault_path = ask_vault_path(vault[0] if vault else "")
			vault_file = VaultFile(vault_path, master_key[0])
			if not vault_file.data:
				click.echo(f"GPAM: {click.style('Vault file has been damaged or master key incorrect', fg='red')}", file=click.get_text_stream("stderr"))
				sys.exit(config.EXIT_FAILURE)
		vault_file.update_master_key(master_key[1])
		vault_file.save()

	if len(vault) >= 2 and vault[0] != vault[-1]:
		config_file.update_vault_name(vault[0], vault[-1])
		config_file.save()

@gpam.command()
@click.argument("vault", type=str)
@click.argument("site", type=str, required=False)
@click.argument("login", type=str, required=False)
@click.option("--master-key", "-m", type=str)
def delete(vault, site, login, master_key):
	config_file = ConfigurationFile(config.CONFIG_FILE_PATH)
	if not config_file.data:
		click.echo(f"GPAM: {click.style('Configuration file has been damaged.', fg='red')}", file=click.get_text_stream("stderr"))
		sys.exit(config.EXIT_FAILURE)

	if not vault in config_file.get_all_vault_names():
			click.echo(f"GPAM: {click.style('Vault with specified name not exists', fg='red')}: {vault}", file=click.get_text_stream("stderr"))
			sys.exit(config.EXIT_FAILURE)	

	master_key = master_key if master_key else ask_master_key(confirm=False)
	vault_file = VaultFile(config_file.get_vault_path(vault), master_key)
	if not vault_file.data:
		click.echo(f"GPAM: {click.style('Vault file has been damaged or master key incorrect', fg='red')}", file=click.get_text_stream("stderr"))
		sys.exit(config.EXIT_FAILURE)

	if login or site:
		vault_file.delete_record(site, login)
	elif site:
		 vault_file.delete_all_sites(site)
	elif vault:
		config_file.delete_vault(vault)

	vault_file.save()
	config_file.save()

if __name__ == "__main__":
	Path(config.GPAM_HOME_DIR).mkdir(mode=0o777, parents=True, exist_ok=True)
	gpam()
