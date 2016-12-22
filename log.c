#include <stdio.h>
#include <stdarg.h>
#include <sys/time.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>

#include "log.h"
#include "types.h"

static char *log_path = "./Logs";

#define MODE (S_IRWXU | S_IRWXG | S_IRWXO)

int is_file_exist(const char *file_path)
{
     if(file_path == NULL)return 0;

     if(access(file_path, F_OK) == 0)return 1;

     return 0;
}

int is_dir_exist(const char *dir_path)
{
     if(dir_path == NULL)return 0;

     if(opendir(dir_path) == NULL)return 0;

     return 1;
}


static int mk_dir(const char *dir)
{
    DIR *logdir = NULL;

    if((logdir = opendir(dir)) == NULL){
        if(0 != (mkdir(dir, MODE))){
            return -1;
        }
    }

    return 0;
}

int create_simlog(uint16_t sim_num)
{
    char sim_name[] = {
        '.',
        '/',
        'L',
        'o',
        'g',
        's',
        '/',
        's',
        'i',
        'm',
        '0',//10
        '0',
        '0',
        '0',
        '.',
        'l',
        'o',
        'g'
    };
    FILE *sim_log;

    sim_name[10] = sim_num%10000/1000 + '0';
    sim_name[11] = sim_num%1000/100 + '0';
    sim_name[12] = sim_num%100/10 + '0';
    sim_name[13] = sim_num%10 + '0';
    sim_log = fopen(sim_name,"a");
    if(sim_log == NULL){
       printf("[Function:%s,Line:%d]Failed to create %s.", __func__, __LINE__, sim_name);
       return -1;
    }
    MCUs[(sim_num-1)/SIM_NUMS].SIM[(sim_num-1)%SIM_NUMS].log = sim_log;

    return 0;
}

int Log_Init(void)
{
    FILE *sim_log;

    if(mk_dir(log_path) != 0){
        printf("[Function:%s,Line:%d]Failed to create path \"Logs\".", __func__, __LINE__);
        return -1;
    }

    //create misc log
    misc_log = fopen("./Logs/misc.log","a");
    if(misc_log == NULL){
        printf("[Function:%s,Line:%d]Failed to create misc.log.", __func__, __LINE__);
        return -1;
    }

    //create sims log
    uint8_t i,j;
    char sim_name[] = {
        '.',
        '/',
        'L',
        'o',
        'g',
        's',
        '/',
        's',
        'i',
        'm',
        '0',//10
        '0',
        '0',
        '0',
        '.',
        'l',
        'o',
        'g'
    };
    uint16_t sim_num;
    for(i = 0;i < MCU_NUMS;i++){
        for(j = 0;j < SIM_NUMS;j++){
            sim_num = i*SIM_NUMS + j + 1;
            sim_name[10] = sim_num%10000/1000 + '0';
            sim_name[11] = sim_num%1000/100 + '0';
            sim_name[12] = sim_num%100/10 + '0';
            sim_name[13] = sim_num%10 + '0';
            sim_log = fopen(sim_name,"a");
            if(sim_log == NULL){
               printf("[Function:%s,Line:%d]Failed to create %s.", __func__, __LINE__, sim_name);
               return -1;
            }
            MCUs[i].SIM[j].log = sim_log;
        }
    }

    return 0;
}

int close_logs(void)
{
    FILE *sim_log;

    if(fclose(misc_log) != 0){
        printf("[Function:%s,Line:%d]Failed to create misc.log.", __func__, __LINE__);
        return -1;
    }

    //create sims log
    uint8_t i,j;

    uint16_t sim_num;
    for(i = 0;i < MCU_NUMS;i++){
        for(j = 0;j < SIM_NUMS;j++){
            sim_num = i*SIM_NUMS + j + 1;
            sim_log = MCUs[i].SIM[j].log;
            if(0 != fclose(sim_log)){
               printf("[Function:%s,Line:%d]Failed to close sim%4d.log", __func__, __LINE__, sim_num);
               return -1;
            }
        }
    }

    return 0;
}

int logs(FILE *log_file, const char *format, ...)
{
    va_list list;
    struct timeval time;
    time_t now;
    char buffer[30];
    //get time
    gettimeofday(&time, NULL);
    now = time.tv_sec;

    strftime(buffer, 30, "%Y-%m-%d %T", localtime(&now));
    va_start(list,format);

    fprintf(log_file, "[%s.%6ld]", buffer, time.tv_usec);
    vfprintf(log_file, format, list);

    va_end(list);

    return 0;
}
