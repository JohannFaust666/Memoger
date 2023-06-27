#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/sysinfo.h>
#include <sys/types.h>

// Мониторинг использованной оперативной памяти
long get_ram_total() {
    struct sysinfo mem_info;
    sysinfo(&mem_info);
    long total_ram = mem_info.totalram / (1024 * 1024);
    return total_ram;
}

// Мониторинг использованной оперативной памяти
long get_ram_usage() {
    struct sysinfo mem_info;
    sysinfo(&mem_info);
    long used_ram = (mem_info.totalram - mem_info.freeram) / (1024 * 1024);
    return used_ram;
}

// Мониторинг свободной оперативной памяти
long get_ram_free() {
    struct sysinfo mem_info;
    sysinfo(&mem_info);
    long free_ram = mem_info.freeram / (1024 * 1024);
    return free_ram;
}

// Мониторинг Swap памяти
long get_swap_usage() {
    struct sysinfo mem_info;
    sysinfo(&mem_info);
    long used_swap = (mem_info.totalswap - mem_info.freeswap) / (1024 * 1024);
    return used_swap;
}

// Размер директории в МБ
long directory_size(char* prepath) {
    DIR *dir;
    struct dirent *entry;
    char path[1024];
    struct stat info;
    long result = 0;

    if((dir = opendir(prepath)) == NULL) {
        syslog(LOG_ERR, "dir_size cant open dir:%s", prepath);
        return 0;
    }
    
    while((entry = readdir(dir)) != NULL) {
        if(strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") !=0){
            memset(path, 0, sizeof(path));
            sprintf(path, "%s/%s", prepath,entry->d_name);
            if(stat(path, &info) != 0) {
                syslog(LOG_ERR, "dir_size stat error %d", errno);
            }
            else if (S_ISDIR(info.st_mode)){
                result += directory_size(path);
            }
            else {
				result += info.st_size;
			}
        }
    }
    closedir(dir);
    free(entry);
    return result;
 }
