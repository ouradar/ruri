KERNEL := $(shell uname)
MACHINE := $(shell uname -m)
KERNEL_VER := $(shell uname -v)
CPUS := $(shell (nproc --all || sysctl -n hw.ncpu) 2>/dev/null || echo 1)
BRANCH := $(shell git rev-parse --abbrev-ref HEAD)
VERSION := 0.1

CC = gcc
CFLAGS = -O2
CFLAGS += -std=gnu2x

CPP = g++
CPPFLAGS = -O2
CPPFLAGS += -std=gnu++14

LDFLAGS = -L/lib/aarch64-linux-gnu -Wl,-whole-archive -luhd -Wl,-no-whole-archive -lboost_program_options -lpthread

C_PROGS = rx_samples_c tx_samples_c
CPP_PROGS = benchmark_rate

all: showinfo $(C_PROGS) $(CPP_PROGS)

clean:
	@echo "\033[38;5;203mCleaning...\033[0m"
	@echo "\033[38;5;203mRemoving $(C_PROGS) $(CPP_PROGS)\033[0m"
	@rm -f $(C_PROGS) $(CPP_PROGS)

showinfo:
	@echo $(EFLAG) "\033[0m\
	KERNEL_VER = \033[38;5;43m$(KERNEL_VER)\033[0m\n\
	KERNEL = \033[38;5;15m$(KERNEL)\033[0m\n\
	VERSION = \033[38;5;46m$(VERSION)\033[0m\n\
	MACHINE = \033[38;5;226m$(MACHINE)\033[0m\n\
	BRANCH = \033[38;5;208m$(BRANCH)\033[0m\n\
	CPUS = \033[38;5;203m$(CPUS)\033[0m\n\
	CFLAGS = \033[38;5;141m$(CFLAGS)\033[0m\n\
	C_PROGS = \033[38;5;45m$(C_PROGS)\033[0m\n\
	CPP_PROGS = \033[38;5;45m$(CPP_PROGS)\033[0m\n\
	"

$(C_PROGS): %: %.c
	@echo "\033[38;5;45m$@\033[0m"
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

$(CPP_PROGS): %: %.cpp
	@echo "\033[38;5;45m$@\033[0m"
	$(CPP) -o $@ $< $(LDFLAGS)
