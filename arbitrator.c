/********************************************************************
2016/12/5 v1.0: set GPIO in or out ot interrupt
*********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <poll.h>

#include "types.h"
//#include "log.h"

#define SYSFS_GPIO_DIR "/sys/class/gpio"
#define GPIO_LED 41
#define MAX_BUF 60
#define POLL_TIMEOUT (3 * 1000) /* 3 seconds */
#define OUT 1
#define IN 0

/**
 * brief: export the GPIO to user space
 * @Param: gpio: the GPIO number
 */
int gpio_export(unsigned int gpio)
{
       int fd ,len;
        char buf[MAX_BUF];
        fd = open(SYSFS_GPIO_DIR "/export" ,O_WRONLY);
        if (fd < 0) {
                perror("gpio/export");
                return fd;
        }
        len = snprintf(buf ,sizeof(buf) ,"%d" ,gpio);
        write(fd ,buf ,len);
        close(fd);
       return 0;
}

/**
 * brief: unexport the GPIO to user space
 * @Param: gpio: the GPIO number
 */
int gpio_unexport(unsigned int gpio)
{
        int fd ,len;
        char buf[MAX_BUF];
        fd = open(SYSFS_GPIO_DIR "/unexport" ,O_WRONLY);
        if (fd < 0) {
                perror("gpio/unexport");
                return fd;
        }
        len = snprintf(buf ,sizeof(buf) ,"%d" ,gpio);
        write(fd ,buf ,len);
        close(fd);
        return 0;
}

/**
 * brief: configure GPIO for input or output
 * @Param: gpio: the GPIO number
 * @Param: out_flag: the flag of output or input.It's value can be 1 or 0.
 */
int gpio_set_dir(unsigned int gpio ,int out_flag)
{
        int fd;
        char buf[MAX_BUF];
        snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/direction", gpio);
        fd = open(buf ,O_WRONLY);
        if (fd < 0) {
                perror(buf);
                return fd;
        }
        if (out_flag)
                write(fd ,"out" ,4);
        else
                write(fd ,"in" ,3);
        close(fd);
        return 0;
}

/**
 * brief: Set the value of GPIO
 * @Param: gpio: the GPIO number
 * @Param: value: the value of GPIO. Supports values of 0 and 1.
 */
int gpio_set_value(unsigned int gpio, unsigned int value)
{
    int fd;
    char buf[MAX_BUF];
    snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/value", gpio);
    fd = open(buf, O_WRONLY);
    if (fd < 0) {
        perror("gpio/set-value");
        return fd;
    }
    if (value)
        write(fd, "1", 2);
    else
        write(fd, "0", 2);
    close(fd);
    return 0;
}

/**
 * brief: get the value of GPIO
 * @Param: gpio: the GPIO number
 * @Param: value: pointer to the value of GPIO
 */
int gpio_get_value(unsigned int gpio, unsigned int *value)
{
        int fd;
        char ch;
        char buf[MAX_BUF];
        snprintf(buf ,sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/value" ,gpio);
        fd = open(buf ,O_RDONLY);
       if (fd < 0) {
                perror("gpio_get_value");
                return fd;
        }
        read(fd ,&ch ,1);
        if (ch == '1')
                *value = 1;
        else if(ch == '0')
              *value = 0;
        close(fd);
        return 0;
}

/**
 * brief: set the edge that trigger interrupt
 * @Param: gpio: the GPIO number
 * @Param: edge:  edge that trigger interrupt
 */
int gpio_set_edge(unsigned int gpio ,char *edge)
{
        int fd;
        char buf[MAX_BUF];
        snprintf(buf ,sizeof(buf) ,SYSFS_GPIO_DIR "/gpio%d/edge" ,gpio);
        fd = open(buf ,O_WRONLY);
        if (fd < 0) {
                perror("gpio_set_edge");
                return fd;
        }
//gpio_set_edge(gpio, "rising");
        write(fd ,edge ,strlen(edge) + 1);
        close(fd);
        return 0;
}

/**
 * brief: open gpio device and return the file descriptor
 * @Param: gpio: the GPIO number
 */
int gpio_fd_open(unsigned int gpio)
{
    int fd, len;
    char buf[MAX_BUF];
    len = snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/value", gpio);
    if(len < MAX_BUF){
        printf("GPIO directory error.\n");
        return -1;
    }
    fd = open(buf, O_RDONLY | O_NONBLOCK );
    if (fd < 0) {
        perror("gpio/fd_open");
    }
    return fd;
}

/**
 * brief: close gpio device
 * @Param: fd: the file descriptor of gpio
 */
int gpio_fd_close(int fd)
{
    return close(fd);
}

#define DC_POWER_PIN        25
#define RESET_PIN           26

void GPIO_OUT(unsigned int gpio,unsigned int value)
{
    gpio_export(gpio);
    gpio_set_dir(gpio, OUT);
    gpio_set_value(gpio, value);
    gpio_unexport(gpio);
}

void DC_Power_ON(void){
    //5V DC Power ON
    GPIO_OUT(DC_POWER_PIN, 1);
}

void DC_Power_OFF(void){
    //5V DC Power ON
    GPIO_OUT(DC_POWER_PIN, 0);
}
void Card_Board_Reset_ON(void){
    GPIO_OUT(RESET_PIN, 0);
}

void Card_Board_Reset_OFF(void){
    GPIO_OUT(RESET_PIN, 1);
}

int Arbitrator_Init(void){
    unsigned int gpio;

    //SPI0
    //GPIOA0~GPIOA7
    for(gpio = 0;gpio < 8;gpio++){
        if(0 != gpio_export(gpio)){
            printf("[%s:%d]Arbitrator initialize failed:export GPIOA%d error.\n", __FILE__, __LINE__, gpio);
            return -1;
        }
        if(0 != gpio_set_dir(gpio, OUT)){
            printf("[%s:%d]Arbitrator initialize failed:set GPIOA%d direction error.\n", __FILE__, __LINE__, gpio);
            return -1;
        }
        if(0 != gpio_set_value(gpio, 1)){
            printf("Reset SPI0 GPIO pin group value in 1 failed.\n");
            return -1;
        }
    }

    //SPI2
    //GPIOA17~GPIOA24
    for(gpio = 17;gpio < 25;gpio++){
        if(0 != gpio_export(gpio)){
            printf("[%s:%d]Arbitrator initialize failed:export GPIOA%d error.", __FILE__, __LINE__, gpio);
            return -1;
        }
        if(0 != gpio_set_dir(gpio, OUT)){
            printf("[%s:%d]Arbitrator initialize failed:set GPIOA%d direction error.", __FILE__, __LINE__, gpio);
            return -1;
        }
        if(0 != gpio_set_value(gpio, 1)){
            printf("Reset SPI2 GPIO pin group value in 1 failed.\n");
            return -1;
        }
    }

    return 0;
}

#define SPI0_ADDR_CS                        7
#define SPI2_ADDR_CS                        24
int Arbitrator(uint8_t mcu_num)
{
    unsigned int gpio;
    uint8_t bits;

    if(mcu_num < 72){//SPI0
        //recovery
        bits = mcu_num;
        if(0 != gpio_set_value(SPI0_ADDR_CS, 0)){
            printf("SPI0 arbitrator clear failed.\n");
            return -1;
        }
        for(gpio = 0;gpio < 7;gpio++){
            if(bits & 0x01){
                if(0 != gpio_set_value(gpio, 1))return -1;
            } else {
                if(0 != gpio_set_value(gpio, 0))return -1;
            }
            bits >>= 1;
        }
        if(0 != gpio_set_value(SPI0_ADDR_CS, 1)){
            printf("SPI0 arbitrator clear failed.\n");
            return -1;
        }
    } else {
        //recovery
        bits = mcu_num - 72;
        if(0 != gpio_set_value(SPI2_ADDR_CS, 1)){
            printf("SPI2 arbitrator clear failed.\n");
            return -1;
        }
        for(gpio = 17;gpio < 24;gpio++){
            if(bits & 0x01){
                if(0 != gpio_set_value(gpio, 1))return -1;
            } else {
                if(0 != gpio_set_value(gpio, 0))return -1;
            }
            bits >>= 1;
        }
        if(0 != gpio_set_value(SPI2_ADDR_CS, 1)){
            printf("SPI2 arbitrator clear failed.\n");
            return -1;
        }
    }

    return 0;
}

///**
// * @brief: main function
// * @Param: argc: number of parameters
// * @Param: argv: parameters list
// */
//int gpio_test(int argc, char **argv)
//{
//    struct pollfd *fdset;
//    int nfds = 1;
//    int gpio_fd, timeout, rc;
//    char *buf[MAX_BUF];
//    unsigned int gpio;
//    int len;
//    char *cmd;
//    unsigned int value;

//    fdset = (struct pollfd*)malloc(sizeof(struct pollfd));
//    if (argc < 3) {
//        printf("Usage: %s <gpio-pin> <direction> [value]\n\n", argv[0]);
//        exit(-1);
//    }
//    cmd = argv[2];
//    gpio = atoi(argv[1]);
//    gpio_export(gpio);
//    if (strcmp(cmd, "interrupt") == 0) {
//            printf("\n**************************GPIO interrupt***************************\n");
//              gpio_set_dir(gpio, IN);
//                gpio_set_edge(gpio, "rising");
//                gpio_fd = gpio_fd_open(gpio);
//                /* GPIO_LED configure */
//                //gpio_export(GPIO_LED);
//                //gpio_set_dir(GPIO_LED, OUT);
//                timeout = POLL_TIMEOUT;
//                while (1) {
//                    memset((void*)fdset, 0, sizeof(fdset));
//                    fdset->fd = gpio_fd;
//                    fdset->events = POLLPRI;
//                    rc = poll(fdset, nfds, timeout);
//                    if (rc < 0) {
//                        printf("\npoll() failed!\n");
//                        return -1;
//                    }
//                    if (rc == 0) {
//                        printf(".");
//                        /* LED off */
//                        //gpio_set_value(GPIO_LED, 1);
//                    }
//                    if (fdset->revents & POLLPRI) {
//                        len = read(fdset->fd, buf, MAX_BUF);
//                        printf("\nGPIO %d interrupt occurred\n", gpio);
//                        /* when GPIO interrupt occurred, LED turn on */
//                        //gpio_set_value(GPIO_LED, 0);
//                    }
//                    fflush(stdout);
//                }
//         gpio_fd_close(gpio_fd);
//    } else if (strcmp(cmd, "out") == 0) {
//         gpio_set_dir(gpio, OUT);
//                  if (argc = 4) {
//                          gpio_set_value(gpio, atoi(argv[3]));
//                          printf("gpio%d is set to %d\n", gpio, atoi(argv[3]));
//                  }
//    } else if (strcmp(cmd, "in") == 0) {
//                  gpio_set_dir(gpio, IN);
//                  printf("\n");
//                  while (1) {
//                          gpio_get_value(gpio, &value);
//                          printf("\rvalue:%d", value);
//                  }
//    } else if (strcmp(cmd, "unexport") == 0) {
//        gpio_unexport(gpio);
//    } else {
//                  printf("Usage: %s <gpio-pin> <direction> [value]\n\n", argv[0]);

//        exit(-1);

//          }

//    return 0;
//}

