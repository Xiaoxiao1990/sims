/**************
 *
 * Date : 2016-12-2
 * Author : ZLL
 *
 * VSIM client managerment.
 * **********/

#include <stdio.h>
#include <stdlib.h>             //exit()
#include <string.h>             //strerror()
#include <errno.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>          //ntoa()
#include <unistd.h>             //close()
#include <malloc.h>
#include <pthread.h>

#include "types.h"
#include "log.h"
#include "functions.h"

/**
 * @brief The Client_Info struct
 * A forward direction loop link node definition.
 */

Client_TypeDef *current_client;

/***
 * task managerment
 *
 * */
pthread_t pid_vsim_manager,pid_clients[MAX_LINKS];
pthread_mutex_t sw_client;
uint16_t Clients_CNT = 0;

/**
 * @brief client_data_init
 * initialize the data node
 * @param ct struct *
 */
static void client_data_init(Client_TypeDef *ct)
{
    uint16_t i;
    //client data link
    ct->num = 0;
    ct->conn_fd = -1;
    ct->ip.s_addr = 0;
    ct->port = 0;
    ct->time_out = 0;                           //clear
    for(i = 0;i < sizeof(ct->device_name);i++)
        ct->device_name[i] = '\0';
    ct->next = NULL;
    ct->prev = NULL;
}

/**
 * @brief add_new_client
 * add client to client data link
 * @param ct
 */
static void add_new_client(Client_TypeDef *ct)
{
    ct->prev = current_client;
    ct->next = current_client->next;
    current_client->next->prev = ct;
    current_client->next = ct;

    current_client = ct;
    printf("Add client[%d] to data link.\n",current_client->num);
}


/**
 * @brief delete_client_by_pos
 * delete client by structure pointer
 * @param ct
 * data structure pointer
 */
static void delete_client_by_pos(Client_TypeDef *ct)
{
    Client_TypeDef *dct = current_client;

    //backward one step in this case. avoid current_client pointer to NULL ,but the links have some more node.
    if(ct == current_client)current_client = current_client->prev;

    while(dct != ct){
        dct = dct->prev;
        if(dct == current_client){
            printf("Client [%d] not found !\n",ct->num);
            goto Node_Not_Found;
        }
    }
    //Delete this node dct.
    dct->prev->next = dct->next;
    dct->next->prev = dct->prev;

//    if((dct == dct->next) && (dct == dct->prev));//Needn't ?
//   else{
        free(dct);
        dct = NULL;
//    }

Node_Not_Found:;
}

/**
 * @brief delete_client_by_num
 * delet client by client number
 * @param num
 * client number
 */
//static void delete_client_by_num(uint16_t num)
//{
//    Client_TypeDef *dct = current_client; //backward search from current_client

//    //find out the node first
//    while(dct->num != num){
//        dct = dct->prev;
//        if(dct == current_client){
//            printf("Client [%d] not found !\n",num);
//            goto Node_Not_Found;
//        }
//    }
//    //ct is the destination node delete it
//    delete_client_by_pos(dct);

//Node_Not_Found:;
//}

#define CLIENT_FRAME_HEAD1                      (uint8_t)0xA5
#define CLINET_FRAME_HEAD2                      (uint8_t)0xA5
#define CLIENT_FRAME_LENGTH                     (uint8_t)

#define PARSE_CLIENT_HEAD1                      (uint8_t)0x00
#define PARSE_CLIENT_HEAD2                      (uint8_t)0x01
#define PARSE_CLINET_SID                        (uint8_t)0x02
#define PARSE_CLIENT_SLOT                       (uint8_t)0x03
#define PARSE_CLIENT_LENGHT                      (uint8_t)0x04
#define PARSE_CLIENT_DATA                       (uint8_t)0x05

static int parse_client(uint8_t *buff, uint16_t buf_len)
{
    uint16_t i, sid, total_len, slot;
    uint8_t step = 0,mcu,sim;
    SIM_TypeDef *SIM;

    for(i = 0;i < buf_len;i++){
        //
        switch(step){
        case PARSE_CLIENT_HEAD1:
            if((uint8_t)buff[i] == CLIENT_FRAME_HEAD1)step++;
            else if(memcmp(buff, "TGT", 3) == 0){//heart beat from device
                return 1;
            }
            break;
        case PARSE_CLIENT_HEAD2:
            if((uint8_t)buff[i] == CLINET_FRAME_HEAD2)step++;
            else step--;
            break;
        case PARSE_CLINET_SID:
            sid = buff[i++] << 8;
            sid += buff[i];
            step++;
            break;
        case PARSE_CLIENT_SLOT:
            slot = buff[i++] << 8;
            slot += buff[i];
            if((slot > 0) && (slot < (SIM_NUMS*MCU_NUMS))){
                sim = (slot - 1)%SIM_NUMS;
                mcu = (slot - 1)/SIM_NUMS;
                SIM = &MCUs[mcu].SIM[sim];
                if(SIM->lock == DISABLE){
                    SIM->lock = ENABLE;
                    logs(SIM->log, "SIM[%d] unlocked.\n", slot);
                } else {
                    logs(SIM->log,"SIM[%d] locked.\n", slot);
                    return -1;
                }
            } else {
                logs(misc_log, "VSP bind to SIM error.Slot number out of range.\n");
                fprint_array(misc_log, buff);
                return -1;
            }
            break;
        case PARSE_CLIENT_LENGHT:
            total_len = buff[i++] << 8;
            total_len += buff[i];
            break;
        case PARSE_CLIENT_DATA:

            break;
        default:break;
        }
    }
    return 0;
}

/**
 * @brief client_stuff
 * @param arg
 * @return
 */
static void * client_stuff(void *arg)
{
    Client_TypeDef *ct = (Client_TypeDef *)arg;
    fd_set readfds;
    struct timeval time_out = {0,100};
    uint8_t buff[VSIM_SOCKET_BUF_LEN];
    int n;

    char *from = inet_ntoa(ct->ip);
    printf("[%s:%d] Connected !\n", from, ct->port);
    logs(misc_log, "[%s:%d] Connected !\n", from, ct->port);

    do{
        FD_ZERO(&readfds);
        FD_SET(ct->conn_fd,&readfds);

        n = select(ct->conn_fd + 1, &readfds, NULL, NULL, &time_out);//no block

        if(n == 0){
            //time out
            if(ct->time_out == 0)break;
        } else if(n < 0) {
            printf("[%s:%d]Client[%d] Recv. error!\n", from, ct->port, ct->num);
            break;
        } else {
            if(FD_ISSET(ct->conn_fd ,&readfds)){
                n = recv(ct->conn_fd, buff, VSIM_SOCKET_BUF_LEN, 0);
                if(n > 0){
                    buff[n] = '\0';
//                    printf("[%s:%d]Client[%d] says:%s\n", from, ct->port, ct->num, buff);
//                    if(parse_client(buff, n) == 1){//VSIM heart beat return.
//                        memcpy(ct->device_name, buff, VSIM_NAME_LEN);
//                        send(ct->conn_fd, buff, n, 0);
//                    }
                    ct->time_out = CLIENT_TIME_OUT_VALUE;
                } else {
                    printf("Client[%d] socket closed.\n", ct->num);
                    ct->time_out = 0;
                    break;         //socket closed.
                }
            }
        }

        //printf("Client[%d]:time out after %2d second(s).\n", ct->num, ct->time_out);
    }while(ct->time_out > 0);

    char *msg = "You're out!";

    send(ct->conn_fd, msg, strlen(msg), 0);
    //Client link time out.
    thread_sleep(2);//wait for mssage send out!
    close(ct->conn_fd);

    //Client OFF line
    printf("[%s:%d]Client[%d] off line.\n", from, ct->port, ct->num);
    logs(misc_log, "[%s:%d]Client[%d] off line.\n", from, ct->port, ct->num);

    //delete the client data node
    pthread_mutex_lock(&sw_client);

    delete_client_by_pos(ct);
    if(Clients_CNT > 0)Clients_CNT--;

    pthread_mutex_unlock(&sw_client);
    //exit thread
    pthread_exit(NULL);
}

/**
 * @brief VSIM_Managerment
 * manager the clients
 */
void VSIM_Management(void)
{
    int listenfd, connfd;
    struct sockaddr_in servaddr;

    if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        logs(misc_log, "Create socket server error:%s(errorno:%d)\n",strerror(errno),errno);
        exit(0);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERVER_PORT);

    if(bind(listenfd, (struct sockaddr *)&servaddr,sizeof(servaddr)) == -1){
        logs(misc_log ,"Bind socket server error:%s(errorno:%d)\n",strerror(errno),errno);
        exit(0);
    }

    if(listen(listenfd, MAX_LINKS) == -1){
        logs(misc_log ,"Listen socket error:%s(errorno:%d)\n",strerror(errno),errno);
        exit(0);
    }

    printf("==================== Waiting For Client's Request ====================\n");

    pthread_mutex_init(&sw_client, NULL);

    while(1){
        struct sockaddr_in client;
        socklen_t len = sizeof(client);
        //if((connfd = accept(listenfd, (struct sockaddr *)NULL, NULL)) == -1){
        if((connfd = accept(listenfd, (struct sockaddr *)&client, &len)) == -1){
            logs(misc_log ,"Accept socket error:%s(errorno:%d)\n",strerror(errno),errno);
            continue;
        }

        Client_TypeDef *ct = (Client_TypeDef *)malloc(sizeof(struct S_Client_Info));
        if(ct == NULL){
            logs(misc_log ,"No space to create an client descriptor.\n");
            //ERROR
            continue;
        }
        //current_client = ct;

        //clear
        client_data_init(ct);
        //memset(ct, 0,sizeof(Client_TypeDef));

        //Create an new client
        //Add client info.
        ct->conn_fd = connfd;
        ct->ip.s_addr = client.sin_addr.s_addr;
        ct->port = client.sin_port;
        ct->time_out = CLIENT_TIME_OUT_VALUE;

        if(Clients_CNT > 0){
            uint16_t flag;
            uint16_t c_num;
            Client_TypeDef *dct;
            for(c_num = 1; c_num != 0; c_num++)
            {//find a number not been used.
                flag = 1;
                dct = current_client;
                do{
                    if(c_num == dct->num)
                    {
                        flag = 0;
                        break;
                    }else dct = dct->next;
                }while(current_client != dct);

                if(flag)break;
            }
            ct->num = c_num;
            add_new_client(ct);//current_client point to ct
        }
        else{//header
            ct->num = Clients_CNT;
            ct->next = ct;
            ct->prev = ct;
            current_client = ct;
        }
        //clients counter
        Clients_CNT++;
        int ret = 0;

        ret = pthread_create(&current_client->tid, NULL, &client_stuff, current_client);
        if(ret != 0){
            logs(misc_log, "Create thread for new client error.\n");
            exit(1);
        }
        pthread_detach(current_client->tid);
//       if((pthread_create(&pid_clients[client_num-1], (void *)NULL, (void *)client_stuff, NULL)) < 0){
 //          printf("Create pthread error.\n");
  //         exit(1);
  //     }
        /*
        char buff[VSIM_SOCKET_BUF_LEN];
        int n;
        //create a new thread to process
        char *from = inet_ntoa(client.sin_addr);
        printf("[%s:%d] Connected !\n", from, client.sin_port);
        n = recv(connfd, buff, VSIM_SOCKET_BUF_LEN, 0);
        buff[n] = '\0';

        printf("Recv msg from client[%s:%d]:%s\n", from, client.sin_port, buff);
        close(connfd);
        */
    }

    close(listenfd);
    logs(misc_log, "Fatal error:%s abort!",__func__);
}
/*
void Client_Init(void)
{
    //Initalize head of client data
    current_client = (Client_TypeDef *)malloc(sizeof(struct S_Client_Info));
    client_data_init(current_client);
    current_client->next = current_client;
    current_client->prev = current_client;
}
*/
/**
 * @brief main
 * for test VSIM managerment.
 * @param argc :none
 * @param argv :none
 * @return 0
 */
//int main(int argc, char *argv[])
//{
//    pthread_t time_wheel;
//    int ret;

//    //Client_Init();

//    ret = pthread_create(&time_wheel, NULL, &SYS_Time_Wheel, NULL);
//    if(ret != 0){
//        printf("Create pthread error.\n");
//        exit(1);
//    }
//    pthread_detach(time_wheel);

//    VSIM_Managerment();

//    return 0;
//}
