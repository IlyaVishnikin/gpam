import click

from os import mkdir
from os.path import exists

from config 				import GPAM_DIRECTORY_LOCATION
from commands.new_commands  import new

@click.group()
def gpam(): ...

gpam.add_command(new)

if __name__ == "__main__":
	if not exists(GPAM_DIRECTORY_LOCATION):
		mkdir(GPAM_DIRECTORY_LOCATION)
	gpam()