# makefile
# --------
# `make` should compile the "default" agent used for playing against the baselines.
# other than that, feel free to add your own flags.

include sources.mk

CC = g++
LIB_SRC = lib/marisa.cpp lib/cdc.cpp lib/chess.cpp lib/movegen.cpp lib/helper.cpp wakasagihime.cpp
SOURCES = $(LIB_SRC) $(ADD_SOURCES)

# normal wakasagi
all:
	g++ -o wakasagi -O2 -DCHINESE_ENABLED=$(CHINESE) -march=native $(SOURCES)

# debug wakasagi
dbg:
	g++ -o wakasagi -g -DCHINESE_ENABLED=$(CHINESE) -march=native $(SOURCES)

# address sanitized wakasagi
why_segfault:
	g++ -o wakasagi -DCHINESE_ENABLED=$(CHINESE) -march=native $(SOURCES) -fsanitize=address,undefined

