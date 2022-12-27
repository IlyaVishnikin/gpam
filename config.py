from os.path import expanduser, join

GPAM_HOME_DIR = join(expanduser("~"), "gpam")
CONFIG_FILE_PATH = join(GPAM_HOME_DIR, "config.json")

EXIT_FAILURE = 1
EXIT_SUCCESS = 0
