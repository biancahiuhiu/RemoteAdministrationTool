#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>

#ifndef DT_DIR
#define DT_DIR 4
#endif


int main() {
    DIR *dir;
    struct dirent *entry;

    // Deschide directorul /proc
    dir = opendir("/proc");
    if (dir == NULL) {
        perror("Error opening /proc");
        exit(EXIT_FAILURE);
    }

    // Parcurge directorul
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR) {
            char *endptr;
            long pid = strtol(entry->d_name, &endptr, 10);

            if (*endptr == '\0') {
                printf("PID: %ld\n", pid);
            }
        }
    }

    // Închide directorul
    closedir(dir);

    return 0;
}