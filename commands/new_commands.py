import click

from os.path import expanduser, join, abspath

from config 				import GPAM_DIRECTORY_LOCATION, GPAM_CONFIGURATION_FILE_LOCATION
from libs.ConfigurationFile import ConfigurationFile
from libs.VaultFile 		import VaultFile

@click.group()
def new(): ...

@new.command()
@click.option("-n", "--name", prompt="Vault name", type=str, help="New name of the vault")
@click.option("-p", "--path", type=click.Path(exists=False, dir_okay=False, writable=True, readable=True), help="Path to the vault")
@click.option("--verbose/--no-verbose", type=bool, default=False, help="Verbose output")
def vault(name: str, path: str, verbose: bool):
	path = abspath(expanduser(path)) if path else join(GPAM_DIRECTORY_LOCATION, f"vault.{name}.json")
	
	vault_file = VaultFile(path)
	if verbose: print(f"GPAM: Initialize a new vault file: {path}.")

	configuration_file = ConfigurationFile(GPAM_CONFIGURATION_FILE_LOCATION)
	if verbose: print(f"GPAM: Initialize configuration file: {GPAM_CONFIGURATION_FILE_LOCATION}.")

	configuration_file.add_vault(name, path)
	if verbose: print(f"GPAM: Adding the new vault({name}:{path}) to the configuration file.")

	vault_file.save()
	if verbose: print("GPAM: Saving the vault file.")
	
	configuration_file.save()
	if verbose: print("GPAM: Saving the configuration file")

@new.command()
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


new.add_command(vault)