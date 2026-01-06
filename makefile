# ----------------------------
# Makefile Options
# ----------------------------

NAME ?= RPN2ASM
DESCRIPTION ?= "Reverse Polish Notation"
COMPRESSED ?= NO
ARCHIVED ?= NO

CFLAGS ?= -Wall -Wextra -Oz
CFLAGS ?= -Wall -Wextra -Oz -Wmain-return-type
CXXFLAGS ?= -Wall -Wextra -Oz

# ----------------------------

ifndef CEDEV
$(error CEDEV environment path variable is not set)
endif

include $(shell cedev-config --makefile)

.PHONY: install format
install: bin/$(TARGET8XP)
	$(Q)tilp bin/$(TARGET8XP)

format:
	clang-format -i **/*.c
