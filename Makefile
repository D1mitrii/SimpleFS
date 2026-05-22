obj-m := simplefs.o
simplefs-y := src/simplefs/simplefs.o src/simplefs/simplefs_ioctl.o

PWD := $(shell pwd)

KERNEL_DIR ?= /lib/modules/$(shell uname -r)/build

CFLAGS = -I$(PWD)/src/include
CLIENT = simplefs_cli
CLIENT_SRC = src/cli/simplefs_cli.c

all: modules client

modules:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules

client: $(CLIENT_SRC)
	gcc $(CFLAGS) -o $(CLIENT) $(CLIENT_SRC)

unload:
	sudo rmmod simplefs

clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) clean