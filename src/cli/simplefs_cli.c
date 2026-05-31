#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <dirent.h>
#include <time.h>

#include "../include/simplefs_ioctl.h"

#define BATCH_SIZE 1024

void cli_usage(const char* name) {
    printf("Usage:\n");
    printf("  %s <mountpoint> zero - Zero all files\n", name);
    printf("  %s <mountpoint> erase - Clear entire filesystem\n", name);
    printf("  %s <mountpoint> metadata - Get all files metadata\n", name);
    printf("  %s <mountpoint> map <filename> - Get info about file sectors\n", name);
    printf("  %s <mountpoint> test - Write and read data\n", name);
}

int open_path(const char* path) {
    int fd = open(path, O_RDONLY | O_DIRECTORY);

	if (fd < 0) {
		perror("cannot open path\n");
        exit(-1);
    }

	return fd;
}

int zero(const char *path) {
    int fd = open_path(path);
    
    if (ioctl(fd, SIMPLEFS_IOCTL_ZERO) < 0) {
        perror("ioctl zero failed\n");
        close(fd);
        return 1;
    }
    
    printf("files zeroed\n");
    close(fd);
    return 0;
}

int erase(const char *path) {
    int fd = open_path(path);
    
    if (ioctl(fd, SIMPLEFS_IOCTL_ERASE) < 0) {
        perror("ioctl erase failed\n");
        close(fd);
        return 1;
    }
    
    printf("SimpleFS erased\n");
    close(fd);
    return 0;
}

int metadata(const char* path) {
    int fd = open_path(path);
    
    struct info_response info = {0};
    if (ioctl(fd, SIMPLEFS_IOCTL_INFO, &info) < 0) {
        perror("ioctl info failed\n");
        close(fd);
        return 1;
    }

    printf("Total file count: %u\n", info.file_count);
    if (info.file_count == 0) {
        printf("No files found\n");
        close(fd);
        return 0;
    }

    printf("%-20s %12s %12s  %s\n", "NAME", "OFFSET", "SIZE", "CRC32");

    struct metadata_entry entries[BATCH_SIZE];
    for (unsigned offset = 0; offset < info.file_count; offset += BATCH_SIZE) {
        
        struct metadata_query query = {
            .entries_ptr = (unsigned long long)entries,
            .offset = offset,
            .capacity = BATCH_SIZE,
            .count = 0
        };
        
        if (ioctl(fd, SIMPLEFS_IOCTL_METADATA, &query) < 0) {
            perror("ioctl metadata failed\n");
            close(fd);
            return 1;
        }

        for (unsigned i = 0; i < query.count; i++) {
            printf(
                "%-20s %12llu %12llu  0x%08x\n",
                entries[i].name,
                (unsigned long long)entries[i].offset,
                (unsigned long long)entries[i].size,
                entries[i].hash
            );
        }
    }
    
    close(fd);
    return 0;
}

int map(const char* path, const char* filename) {
    int fd = open_path(path);
    
    unsigned long capacity = 256;
    struct map_response response = {0};
    
    unsigned long long* sectors = calloc(capacity, sizeof(unsigned long long));
    struct map_query query = {
        .response_ptr = (unsigned long long)&response,
        .sectors_ptr = (uintptr_t)sectors,
        .sectors_capacity = capacity,
    };
    strncpy(query.name, filename, sizeof(query.name) - 1);
    
    if (ioctl(fd, SIMPLEFS_IOCTL_MAP, &query) < 0) {
        perror("ioctl map failed");
        close(fd);
        return 1;
    }
    
    unsigned long long end_sector = response.start_sector + response.sector_count - 1;

    printf("name:          %s\n", query.name);
    printf("start_sector:  %llu\n", (unsigned long long)response.start_sector);
    printf("sector_count:  %llu\n", (unsigned long long)response.sector_count);
    printf("size:          %llu\n", (unsigned long long)response.size);
    printf("Sectors:\n");
    for (unsigned long long i = 0; i < response.length; i++) {
        printf("  [%llu] %llu\n", i+1, sectors[i]);
    }
    if (response.length < response.sector_count) {
        printf("  ... and %llu more\n", response.sector_count - response.length);
    }
    
    close(fd);
    return 0;
}

int test(const char* path) {
    DIR *dir = opendir(path);
    if (!dir) {
        perror("cannot open dir");
        return 1;
    }
    
    struct dirent* entry;
    int total = 0;
    int passed = 0;
    
    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, "file", 4) != 0)
            continue;
        
        char filepath[512];
        snprintf(filepath, sizeof(filepath), "%s/%s", path, entry->d_name);
        
        int fd = open(filepath, O_RDWR);
        if (fd < 0) {
            printf("Cannot open %s", filepath);
            continue;
        }
        total++;
        
        unsigned int random_num = rand();
        unsigned int read_num = 0;

        if (pwrite(fd, &random_num, sizeof(random_num), 0) != sizeof(random_num)) {
            printf("Cannot write to %s\n", entry->d_name);
            close(fd);
            continue;
        }
        fsync(fd);
        if (pread(fd, &read_num, sizeof(read_num), 0) != sizeof(read_num)) {
            printf("Cannot read from %s\n", entry->d_name);
            close(fd); 
            continue;
        }
        
        if (random_num == read_num) {
            printf("%s: OK (wrote %u, read %u)\n", entry->d_name, random_num, read_num);
            passed++;
        } else {
            printf("%s: FAIL (wrote %u, read %u)\n", entry->d_name, random_num, read_num);
        }
        
        close(fd);
    }
    
    closedir(dir);
    printf("RESULT: %d/%d\n", passed, total);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        cli_usage(argv[0]);
        return 1;
    }

    srand(time(NULL));
    
    const char* name = argv[0];
    const char* path = argv[1];
    const char* cmd = argv[2];

    if (!strcmp(cmd, "test"))
        return test(path);

    if (!strcmp(cmd, "zero"))
        return zero(path);

    if (!strcmp(cmd, "erase"))
        return erase(path);

    if (!strcmp(cmd, "metadata"))
        return metadata(path);

    if (!strcmp(cmd, "map")) {
        if (argc < 4) {
            cli_usage(name);
            return 1;
        }
        const char* filename = argv[3];
        return map(path, filename);
    }
    
    cli_usage(name);
    return 1;
}