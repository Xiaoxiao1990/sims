#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/select.h>
#include <stdint.h>

#define ARBI_TRANS_COMPLETE         (uint8_t)(0xff)
#define ARBI_TRANS_ERROR            (uint8_t)(0xee)
#define ARBI_TRANS_RESET            (uint8_t)(0xdd)
#define MCU_GROUP_0                 (uint8_t)(0)
#define MCU_GROUP_1                 (uint8_t)(36)
#define MCU_GROUP_2                 (uint8_t)(72)
#define MCU_GROUP_3                 (uint8_t)(108)

#define ARBITRATOR_NUMS             (uint8_t)(3)
#define MCU_ON_ARBI_NUMS            (uint8_t)(36)
#define MAX_RETYR_TIMES             (uint8_t)(3)


static int Arbi_fd[3];

static int set_interface_attribs(int fd, int speed)
{
    struct termios tty;

    if (tcgetattr(fd, &tty) < 0) {
        printf("Error from tcgetattr: %s\n", strerror(errno));
        return -1;
    }

    cfsetospeed(&tty, (speed_t)speed);
    cfsetispeed(&tty, (speed_t)speed);

    tty.c_cflag |= (CLOCAL | CREAD);    /* ignore modem controls */
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;         /* 8-bit characters */
    tty.c_cflag &= ~PARENB;     /* no parity bit */
    tty.c_cflag &= ~CSTOPB;     /* only need 1 stop bit */
    tty.c_cflag &= ~CRTSCTS;    /* no hardware flowcontrol */

    /* setup for non-canonical mode */
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    tty.c_oflag &= ~OPOST;

    /* fetch bytes as they become available */
    tty.c_cc[VMIN] = 1;
    tty.c_cc[VTIME] = 1;

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        printf("Error from tcsetattr: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

/*******************************************************************
* 名称： UART0_Recv
* 功能： 接收串口数据
* 入口参数： fd :文件描述符
* recv :接收串口中数据存入recv
* data_len :一帧数据的长度
* 出口参数： 正确返回为0，错误返回为-1
*******************************************************************/
static int uart_recv(int fd, uint8_t *recv)
{
    int fs_sel;
    fd_set fs_read;
    struct timeval time;
    char rcv_buf[2] = {0,0};

    FD_ZERO(&fs_read);
    FD_SET(fd,&fs_read);

    time.tv_sec = 0;
    /**********************************
     * In a single time out test ,1000us
     * is a suitable timer value of loop
     * communication by 460800
     * *******************************/
    time.tv_usec = 1000;

    //使用select实现串口的多路通信
    fs_sel = select(fd+1,&fs_read,NULL,NULL,&time);
    if(fs_sel){
        if(read(fd,rcv_buf,1) > 0){
            *recv = rcv_buf[0];
            printf("Recv:%02X\n",*recv);
            return 0;
        }
        else{
            printf("Recevie Error.\n");
            return -1;
        }
    } else {
        return -1;
    }
}

/**
 * @brief uart_send
 * send one byte mcu number
 * @param fd
 * @param mcu_num
 * @return -1:fail,0:success
 */
static int uart_send(int fd, uint8_t mcu_num)
{
    printf("Send:%02X || ", mcu_num);
    uint8_t buf[2] = {0,0};
    int len = -1;

    buf[0] = mcu_num;
    len = write(fd, buf, 1);
    if (len != 1){
        printf("Error from write: %d, %d\n", len, errno);
        return -1;
    }
    tcdrain(fd);    //delay for output

    return 0;
}

/**
 * @brief Arbitrator_Init
 * Initialize arbitrators uart.
 * @return -1:fail,0:success
 */
int Arbitrator_Init(void)
{
    char const *uart_dev[] ={
        //"/dev/ttyUSB0",
        //"/dev/ttyUSB0",
        //"/dev/ttyUSB0"
        "/dev/ttySACS1",
        "/dev/ttySACS2",
        "/dev/ttySACS3"
    };
    uint8_t i = 0,retry = 0;
    uint8_t result = -1;

    for (i = 0;i < ARBITRATOR_NUMS;i++){
        printf("Reset arbitrator[%d]...\n",i);
        Arbi_fd[i] = open(uart_dev[i], O_RDWR | O_NOCTTY | O_SYNC);
        if (Arbi_fd[i] < 0) {
            printf("Error opening %s: %s\n", uart_dev[i], strerror(errno));
            return -1;
        }
        /*baudrate 460800, 8 bits, no parity, 1 stop bit */
        if(set_interface_attribs(Arbi_fd[i], B460800) != 0)printf("Uart[%d] set error.\n",i);

        retry = 0;
        //Reset Arbitrators
        do{
            if(uart_send(Arbi_fd[i], ARBI_TRANS_RESET) != 0)retry++;
            else break;
        }while(retry < MAX_RETYR_TIMES);
        if(retry > MAX_RETYR_TIMES){
            printf("Reset arbitrator[%d] error, write failed.\n", i);
            return -1;
        }
        retry = 0;
        do{
            if(uart_recv(Arbi_fd[i], &result) != 0)retry++;
        }while(retry < MAX_RETYR_TIMES);

        if(retry > MAX_RETYR_TIMES){
            printf("Reset arbitrator[%d] error, read failed.\n", i);
            return -1;
        }
        if(result != ARBI_TRANS_RESET){
            printf("\nReset arbitrator[%d] failed.\n",i);
            return -1;
        }
    }
    return 0;
}

/**
 * @brief release_arbitrator
 * @param arbi
 * @param trys
 * @return
 */
static int release_arbitrator(uint8_t arbi, uint8_t trys)
{
    int retry = 0;
    uint8_t result;
    printf("Release arbitrator[%d].\n", arbi);
    if(arbi >= ARBITRATOR_NUMS){
        printf("Arbitrator number error.\n");
        return -1;
    }

    while(uart_send(Arbi_fd[arbi], ARBI_TRANS_COMPLETE) != 0){
        if(++retry > trys)break;
    }

    if(retry > trys){
        printf("Release arbitrator[%d] error.\n", arbi);
        return -1;
    }
    retry = 0;

    while(uart_recv(Arbi_fd[arbi], &result) != 0){
        if(++retry > trys)break;
    }
    if(retry > trys){
        printf("Release arbitrator[%d] error.\n", arbi);
        return -1;
    }

    if(result != ARBI_TRANS_COMPLETE){
        printf("Clear arbitrator[%d] error.\n", arbi);
        return -1;
    }

    return 0;
}

/**
 * @brief Arbitrator
 * @param mcu_num
 * @return
 */
int Arbitrator(uint8_t mcu_num)
{
    int fd;
    uint8_t result = -1;

    printf("Select MCU group.\n");
    if(mcu_num == MCU_GROUP_0){
        //COMLETE MCU1/MCU2
        if(-1 == release_arbitrator(1, MAX_RETYR_TIMES)){
            printf("Release arbitrator 1 failed.\n");
            return -1;
        }
        if(-1 == release_arbitrator(2, MAX_RETYR_TIMES)){
            printf("Release arbitrator 2 failed.\n");
            return -1;
        }
        fd = Arbi_fd[0];
    }
    else if(mcu_num < MCU_GROUP_1){          //0~35
        fd = Arbi_fd[0];
    }else if(mcu_num == MCU_GROUP_1){        //36
        //COMLETE MCU0/MCU2
        if(-1 == release_arbitrator(0, MAX_RETYR_TIMES)){
            printf("Release arbitrator 0 failed.\n");
            return -1;
        }
        if(-1 == release_arbitrator(2, MAX_RETYR_TIMES)){
            printf("Release arbitrator 2 failed.\n");
            return -1;
        }
        fd = Arbi_fd[1];
    }else if(mcu_num < MCU_GROUP_2){         //36~71
        fd = Arbi_fd[1];

    }else if(mcu_num == MCU_GROUP_2){        //72
        //COMLETE MCU0/MCU1
        if(-1 == release_arbitrator(0, MAX_RETYR_TIMES)){
            printf("Release arbitrator 0 failed.\n");
            return -1;
        }
        if(-1 == release_arbitrator(1, MAX_RETYR_TIMES)){
            printf("Release arbitrator 1 failed.\n");
            return -1;
        }
        fd = Arbi_fd[2];
    }else if(mcu_num < MCU_GROUP_3){        //72~107
        fd = Arbi_fd[2];
    }else{
        //mcu_num error
        return -1;
    }

    //send mcu_num
    uint8_t retry_times = 0;
repeate:

    if(++retry_times > MAX_RETYR_TIMES)return -1;

    while(uart_send(fd, mcu_num % MCU_ON_ARBI_NUMS) != 0);
    //recv result
    while(uart_recv(fd, &result) != 0);

    switch (result) {
    case ARBI_TRANS_COMPLETE://never been here
        //complete
    case ARBI_TRANS_ERROR:
        //mcu_num error retry
    case ARBI_TRANS_RESET://never been here
        //reset all mcus
        goto repeate;
        break;
    default:
        if(result != (mcu_num % MCU_ON_ARBI_NUMS)){//error,try again
            goto repeate;
        }
        break;
    }

    return 0;
}

#ifdef MOUDULE_TEST

#define TIME_ON 1

#if TIME_ON == 1

#include <sys/time.h>

double time_use(struct timeval *start_time, struct timeval *end_time)
{
    double timeuse;
    return (timeuse = (double)1000000*(end_time->tv_sec - start_time->tv_sec) + end_time->tv_usec - start_time->tv_usec);
}

#endif

/***************************************************************
 * only for moudule test
 * ************************************************************/

int main()
{
    int res;
#if TIME_ON == 1
    struct timeval start_time,end_time;
#endif
    printf("Initial arbitrator...\n");

    res = Arbitrator_Init();
    if(res != 0){
        printf("Arbitration Initialize Error!\n");
        return -1;
    }
    int i;
    printf("Select a channel.\n");
#if TIME_ON == 1
    gettimeofday(&start_time, 0);
#endif

    for(i = 0;i < 108;i++){
        printf("Communicate with MCU[%d].\n",i);
        if(Arbitrator(i) != 0){
            printf("Arbitrate Error at MCU[%d].\n",i);
            break;
        }
    }
#if TIME_ON == 1
    gettimeofday(&end_time, 0);

    printf("Time use:%lf us.\n", time_use(&start_time, &end_time));
#endif
    return 0;
}
#endif
