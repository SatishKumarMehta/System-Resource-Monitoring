# Top-level Makefile for Linux System Resource Monitor Capstone Project
CC ?= gcc
CFLAGS ?= -Wall -Wextra -O2 -Iinclude
all: driver app
driver:
	@echo "==> Building Linux Kernel Device Driver..."
	$(MAKE) -C driver
app: app/sysmon_app.c include/sys_driver.h
	@echo "==> Building User-Space Monitoring Application..."
	$(CC) $(CFLAGS) app/sysmon_app.c -o app/sysmon_app
clean:
	@echo "==> Cleaning driver artifacts and user binary..."
	$(MAKE) -C driver clean
	rm -f app/sysmon_app
load:
	@echo "==> Inserting kernel module..."
	sudo insmod driver/sys_driver.ko 2>/dev/null || sudo insmod -f driver/sys_driver.ko
	@echo "==> Setting read/write permissions for /dev/sys_monitor..."
	sudo chmod 666 /dev/sys_monitor
	@echo "==> Driver successfully loaded! Run 'dmesg | tail' to view kernel logs."
unload:
	@echo "==> Removing kernel module..."
	sudo rmmod sys_driver
	@echo "==> Driver unloaded."
run: app
	@echo "==> Launching System Resource Monitor..."
	./app/sysmon_app
.PHONY: all driver app clean load unload run