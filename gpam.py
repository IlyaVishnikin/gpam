import click

import config

from pathlib import Path

@click.group()
def gpam(): ...


@gpam.command()
@click.option("--vault", "-v", type=str, help="Vault name(if not set than uses default vault)")
@click.option("--login", "-l", type=str, help="New login name")
@click.option("--site", "-s", type=str, help="New site")
@click.option("--password", "-p", type=str, help="New password")
@click.option("--field", "-f", type=(str, str), multiple=True, help="Custome field")
@click.option("--master-key", "-m", type=str, help="Master key")
@click.option("--interacitve", "-i", is_flag=True, help="Interactive input of fields")
def new(vault, login, site, password, field):
	...

if __name__ == "__main__":
	Path(config.GPAM_HOME_DIR).mkdir(mode=0o777, parents=True, exist_ok=True)
	gpam()
