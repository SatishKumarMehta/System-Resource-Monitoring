#ifndef SYS_DRIVER_H
#define SYS_DRIVER_H
#include <linux/ioctl.h>
/* Device file name and identification */
#define DEVICE_NAME "sys_monitor"
#define CLASS_NAME  "sysmon_class"
/*
 * IOCTL Command Definitions
 * Magic number 'm' chosen for memory/monitor
 */
#define SYSMON_MAGIC 'm'
/* Query driver internal version number */
#define IOCTL_GET_VERSION      _IOR(SYSMON_MAGIC, 1, int)
/* Set memory warning threshold percentage (1 to 100) */
#define IOCTL_SET_THRESHOLD    _IOW(SYSMON_MAGIC, 2, int)
/* Get currently configured memory warning threshold */
#define IOCTL_GET_THRESHOLD    _IOR(SYSMON_MAGIC, 3, int)
#endif /* SYS_DRIVER_H */