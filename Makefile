CC=gcc
CSRC=src/main.c include/gpam/types/types.c include/gpam/utils/utils.c
CINC=-I./include -I/usr/include/libxml2
CLIB=-lxml2

OUTPUT_EXE=gpam

all: set_vault_location
	$(CC) $(CINC) -o $(OUTPUT_EXE) $(CSRC) $(CLIB)

set_vault_location:
	sed -i "6 s#vault_location#\"/home/$$(whoami)/.gpam.vaults.xml\"#" ./include/gpam/consts/consts.h