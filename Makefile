# Global Makefile
export APP = main
export CC = gcc
export AR = ar
export CFLAGS = -Wall -std=c11 -D_XOPEN_SOURCE=700 -D_DEFAULT_SOURCE

# Directories
DIRS=Communication App

# Main rule
all: CFLAGS += -O2
all: $(patsubst %, _dir_%, $(DIRS))

$(patsubst %,_dir_%,$(DIRS)):
	cd $(patsubst _dir_%,%,$@) && make

$(patsubst %,_clean_%,$(DIRS)):
	cd $(patsubst _clean_%,%,$@) && make clean

# Debug mode compilation
verbose: CFLAGS += -DVERBOSE
verbose: $(patsubst %, _dir_%, $(DIRS))

debug: CFLAGS += -DDEBUG -g
debug: verbose

# Cleaning
clean: $(patsubst %, _clean_%, $(DIRS))
