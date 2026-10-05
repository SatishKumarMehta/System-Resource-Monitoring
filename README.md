Linux System Resource Monitor (Kernel Character Device Driver)
A low-level Linux Capstone Project demonstrating **Software Architecture** and **Linux Device Driver** concepts implemented exclusively in **C**.
---
##  Project Overview
In Unix-like operating systems, user applications run in **Ring 3 (User Space)** with restricted privileges and cannot directly inspect raw kernel structures or hardware states. **Ring 0 (Kernel Space)** holds the privileged operating system structures.
This project implements a complete **System Resource Monitor** by developing:
1. **A Linux Kernel Character Device Driver (`sys_driver.ko`)**: Creates a pseudo-device node at `/dev/sys_monitor`. When invoked, it interrogates kernel-level structures (`sysinfo`, `jiffies`, `task_struct`) and safely packages metrics across the kernel-user boundary using `copy_to_user()`.
2. **A User-Space Monitoring Dashboard (`sysmon_app`)**: A clean C terminal dashboard that utilizes Unix system calls (`open`, `read`, `ioctl`, `lseek`) to query the driver in real-time, configure memory usage thresholds, and display system health indicators.
---
  Software Architecture
```
+--------------------------------------------------------------------------+
|                        USER SPACE (Ring 3)                               |
|                                                                          |
|       +----------------------------------------------------------+       |
|       |                     sysmon_app.c                         |       |
|       |  - Interactive terminal dashboard                        |       |
|       |  - Configures memory alert thresholds via ioctl()        |       |
|       |  - Reads formatted system telemetry via read()           |       |
|       +----------------------------+-----------------------------+       |
+------------------------------------|-------------------------------------+
                                     | System Calls
                                     | (open, read, ioctl, close)
+------------------------------------|-------------------------------------+
|                        KERNEL SPACE (Ring 0)                             |
|                                    v                                     |
|       +----------------------------------------------------------+       |
|       |                 /dev/sys_monitor (cdev)                  |       |
|       +----------------------------+-----------------------------+       |
|                                    v                                     |
|       +----------------------------------------------------------+       |
|       |                      sys_driver.c                        |       |
|       |  - Character Device Registration (alloc_chrdev_region)   |       |
|       |  - File Operations Table (file_operations struct)        |       |
|       |  - Memory extraction via si_meminfo()                    |       |
|       |  - Uptime calculation via jiffies / HZ                   |       |
|       |  - Task tracking via RCU for_each_process()              |       |
|       |  - Memory boundary safety via copy_to_user()             |       |
|       |  - Dynamic configuration via unlocked_ioctl()            |       |
|       +----------------------------------------------------------+       |
+--------------------------------------------------------------------------+
```
---
  Linux Device Driver Concepts Implemented
- **Character Device Registration**: Uses `alloc_chrdev_region()` for dynamic major/minor number allocation, `cdev_init()`, and `cdev_add()`.
- **Automatic Device Creation**: Implements `class_create()` and `device_create()` to automatically register `/dev/sys_monitor` via udev without requiring manual `mknod`.
- **File Operations (`struct file_operations`)**:
  - `open`: Tracks concurrent active sessions.
  - `release`: Cleans up descriptors upon close.
  - `read`: Formats kernel statistics and safely copies data to user space.
  - `unlocked_ioctl`: Handles custom control commands (`IOCTL_SET_THRESHOLD`, `IOCTL_GET_VERSION`).
- **Kernel-to-User Memory Isolation**: Demonstrates `copy_to_user()` and `copy_from_user()` to enforce memory safety between kernel and user address spaces.
- **Kernel Concurrency & RCU**: Uses `rcu_read_lock()` / `rcu_read_unlock()` to safely traverse the kernel task list using `for_each_process()`.
---
 Project Directory Structure
```text
linux-system-resource-monitor/
├── Makefile                 # Top-level orchestrator Makefile
├── include/
│   └── sys_driver.h         # Shared header: Device name & IOCTL definitions
├── driver/
│   ├── Makefile             # Kbuild Makefile for compiling kernel module
│   └── sys_driver.c         # Linux Character Device Driver source (C)
├── app/
│   └── sysmon_app.c         # User-space monitoring utility (C)
└── README.md                # Capstone documentation and execution guide
```
---
##  Prerequisites
This project is built and tested exclusively on **Linux OS** (Ubuntu 20.04/22.04/24.04, Debian, or WSL2 with Linux kernel headers).
Install required build tools:
```bash
sudo apt update
sudo apt install -y build-essential linux-headers-$(uname -r)
```
##  Build and Execution Instructions
### 1. Build Both Driver and User Application
From the root of the project directory:
```bash
make
```
### 2. Load the Kernel Module
Load the driver into the running Linux kernel and set device permissions:
```bash
make load
```
*(Verify module insertion using `dmesg | tail` or `lsmod | grep sys_driver`)*
### 3. Run the Monitoring Application
Run with default memory warning threshold (80%):
```bash
make run
```
Or run with a custom threshold percentage (e.g., 65%):
```bash
./app/sysmon_app 65
```
### 4. Unload Driver and Clean
When finished testing:
```bash
make unload
make clean
```
---
##  Sample Terminal Output
```text
===============================================================
        LINUX SYSTEM RESOURCE MONITOR (C / DEVICE DRIVER)      
===============================================================
 Driver Module   : /dev/sys_monitor
 Driver Version  : v1.0
 Alert Threshold : 75% Memory Usage
 (Press Ctrl+C to exit monitor)
===============================================================
[+] System Uptime       : 02:41:18 (9678 seconds)
[+] Active Tasks/Threads: 194 running processes
[+] Total Memory        : 16048 MB
[+] Used Memory         : 7215 MB
[+] Free Memory         : 8833 MB
[+] Memory Load Bar     : [#############.................] 44%
---------------------------------------------------------------
 [STATUS: NORMAL] System operating safely within thresholds.     
---------------------------------------------------------------
```
---
