#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <signal.h>
#include "../include/sys_driver.h"
#define DEVICE_PATH "/dev/" DEVICE_NAME
#define BUFFER_SIZE 512
static volatile int keep_running = 1;
/**
 * @brief Signal handler for graceful termination (Ctrl+C)
 */
void handle_sigint(int sig)
{
    (void)sig;
    keep_running = 0;
}
/**
 * @brief Render an ASCII graphical progress bar
 */
void print_progress_bar(int percentage)
{
    int bar_width = 30;
    int filled = (percentage * bar_width) / 100;
     printf("[");
    for (int i = 0; i < bar_width; i++) {
        if (i < filled)
            printf("#");
        else
            printf(".");
    }
    printf("] %d%%\n", percentage);
}
int main(int argc, char *argv[])
{
    int fd;
    int driver_version = 0;
    int threshold = 80; /* default 80% */
    char buffer[BUFFER_SIZE];
    /* Allow custom threshold as CLI argument: ./sysmon_app 70 */
    if (argc > 1) {
        threshold = atoi(argv[1]);
        if (threshold < 1 || threshold > 100) {
            fprintf(stderr, "Warning: Invalid threshold (%d%%). Using default 80%%.\n", threshold);
            threshold = 80;
        }
    }
    signal(SIGINT, handle_sigint);
    /* Open the device file exposed by the kernel driver */
    fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        perror("Error opening " DEVICE_PATH);
        fprintf(stderr, "\nTip: Ensure the kernel module is loaded and permissions are set:\n");
        fprintf(stderr, "  1. sudo insmod driver/sys_driver.ko\n");
        fprintf(stderr, "  2. sudo chmod 666 %s\n\n", DEVICE_PATH);
        return EXIT_FAILURE;
    }
    /* 1. Use IOCTL to query driver version */
    if (ioctl(fd, IOCTL_GET_VERSION, &driver_version) < 0) {
        perror("ioctl(IOCTL_GET_VERSION) failed");
    }
    /* 2. Use IOCTL to set custom warning threshold */
    if (ioctl(fd, IOCTL_SET_THRESHOLD, &threshold) < 0) {
        perror("ioctl(IOCTL_SET_THRESHOLD) failed");
    }
    printf("\033[2J\033[H"); /* Clear screen */
    printf("===============================================================\n");
    printf("        LINUX SYSTEM RESOURCE MONITOR (C / DEVICE DRIVER)      \n");
    printf("===============================================================\n");
    printf(" Driver Module   : %s\n", DEVICE_PATH);
    printf(" Driver Version  : v%d.%d\n", driver_version / 100, driver_version % 100);
    printf(" Alert Threshold : %d%% Memory Usage\n", threshold);
    printf(" (Press Ctrl+C to exit monitor)\n");
    printf("===============================================================\n\n");
     while (keep_running) {
        ssize_t bytes_read = read(fd, buffer, sizeof(buffer) - 1);
        if (bytes_read < 0) {
            perror("Error reading from device driver");
            break;
        }
        buffer[bytes_read] = '\0';
        /* Variables to parse */
        unsigned long uptime_sec = 0;
        char uptime_hms[32] = "00:00:00";
        unsigned long total_ram = 0;
        unsigned long free_ram = 0;
        unsigned long used_ram = 0;
        int active_tasks = 0;
        int current_threshold = 0;

        char *line = strtok(buffer, "\n");
        while (line != NULL) {
            if (strncmp(line, "UPTIME_SEC=", 11) == 0)
                uptime_sec = strtoul(line + 11, NULL, 10);
            else if (strncmp(line, "UPTIME_HMS=", 11) == 0)
                strncpy(uptime_hms, line + 11, sizeof(uptime_hms) - 1);
            else if (strncmp(line, "TOTAL_RAM_MB=", 13) == 0)
                total_ram = strtoul(line + 13, NULL, 10);
            else if (strncmp(line, "FREE_RAM_MB=", 12) == 0)
                free_ram = strtoul(line + 12, NULL, 10);
            else if (strncmp(line, "USED_RAM_MB=", 12) == 0)
                used_ram = strtoul(line + 12, NULL, 10);
            else if (strncmp(line, "ACTIVE_TASKS=", 13) == 0)
                active_tasks = atoi(line + 13);
            else if (strncmp(line, "THRESHOLD=", 10) == 0)
                current_threshold = atoi(line + 10);
            line = strtok(NULL, "\n");
        }
        int ram_usage_pct = 0;
        if (total_ram > 0) {
            ram_usage_pct = (int)((used_ram * 100) / total_ram);
        }
        printf("\033[8;1H");
        printf("[+] System Uptime       : %s (%lu seconds)\n", uptime_hms, uptime_sec);
        printf("[+] Active Tasks/Threads: %d running processes\n", active_tasks);
        printf("[+] Total Memory        : %lu MB\n", total_ram);
        printf("[+] Used Memory         : %lu MB\n", used_ram);
        printf("[+] Free Memory         : %lu MB\n", free_ram);
        printf("[+] Memory Load Bar     : ");
        print_progress_bar(ram_usage_pct);
        printf("\n---------------------------------------------------------------\n");
        if (ram_usage_pct >= current_threshold) {
            printf(" [ALERT!] WARNING: Memory usage (%d%%) exceeded threshold (%d%%)! \n",
                   ram_usage_pct, current_threshold);
        } else {
            printf(" [STATUS: NORMAL] System operating safely within thresholds.     \n");
        }
        printf("---------------------------------------------------------------\n");
        fflush(stdout);
        sleep(1);
        /* Re-open/seek to reset file position for next reading */
        lseek(fd, 0, SEEK_SET);
    }
    printf("\nExiting monitor cleanly. Closing device descriptor...\n");
    close(fd);
    return EXIT_SUCCESS;
}