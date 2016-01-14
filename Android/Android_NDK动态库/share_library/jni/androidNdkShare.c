#include <stdio.h>
#include <sys/ioctl.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <net/if.h> 
#include <arpa/inet.h>
#include <string.h>
#include "log.h"
#include "./include/androidNdkShare.h"



#define SUCCESS 0
#define FAIL -1
#define MAXINTERFACES 16
#define MAC_ADDR_LEN 18
#define ADDR_LEN 16
#define LINE_LEN 256
#define DNSMASQ_LEASES "/data/misc/dhcp/dnsmasq.leases"


/*
 * get device mac 
 * @device_name 网口的名字, 比如eth0
 * @mac_addr 返回的mac地址, 传进去的参数必须有自己的存储空间
 * 
 */
int getMAC(char *device_name, char *mac_addr){
    int sock_fd;
    struct ifreq buf[MAXINTERFACES];
    struct ifconf ifc;
    int interface_num;

    if((sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
        LOGD("Create socket failed");
        return(FAIL); 
    }

    ifc.ifc_len = sizeof(buf);
    ifc.ifc_req = buf;
    if(ioctl(sock_fd, SIOCGIFCONF, (char *)&ifc) < 0){
        LOGD("Get a list of interface addresses failed");
        return(FAIL); 
    }

    interface_num = ifc.ifc_len / sizeof(struct ifreq);
    LOGD("The number of interfaces is %d\n", interface_num);

    while(interface_num--) {
        LOGD("Net device: %s\n", buf[interface_num].ifr_name);

        if(ioctl(sock_fd, SIOCGIFFLAGS, (char *)&buf[interface_num]) < 0){
            LOGD("Get the active flag word of the device");
            return(FAIL); 
        }

        if(buf[interface_num].ifr_flags & IFF_PROMISC)
            LOGD("Interface is in promiscuous mode\n");

        if(buf[interface_num].ifr_flags & IFF_UP)
            LOGD("Interface is running\n");
        else
            LOGD("Interface is not running\n");

        if(ioctl(sock_fd, SIOCGIFHWADDR, (char *)&buf[interface_num]) < 0){
            LOGD("Get the hardware address of a device failed");
            return(FAIL); 
        }

        if(!strcmp(device_name, buf[interface_num].ifr_name)){
            snprintf(mac_addr, MAC_ADDR_LEN, "%02X:%02X:%02X:%02X:%02X:%02X",
                (unsigned char)buf[interface_num].ifr_hwaddr.sa_data[0],
                (unsigned char)buf[interface_num].ifr_hwaddr.sa_data[1],
                (unsigned char)buf[interface_num].ifr_hwaddr.sa_data[2],
                (unsigned char)buf[interface_num].ifr_hwaddr.sa_data[3],
                (unsigned char)buf[interface_num].ifr_hwaddr.sa_data[4],
                (unsigned char)buf[interface_num].ifr_hwaddr.sa_data[5]);
            LOGD("Mac address is %s\n", mac_addr);
            return(SUCCESS); 
        }
    }
    return(SUCCESS); 
}

/*
 * get device ip
 * @device_name 网口的名字, 比如eth0
 * @addr 返回的ip地址, 传进去的参数必须有自己的存储空间
 * 
 */
int getIP(char *device_name, char *addr){
    int sock_fd;
    struct ifreq buf[MAXINTERFACES];
    struct ifconf ifc;
    int interface_num;
    char *tmp_addr;

    if((sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
        LOGD("Create socket failed");
        return(FAIL); 
    }

    ifc.ifc_len = sizeof(buf);
    ifc.ifc_req = buf;
    if(ioctl(sock_fd, SIOCGIFCONF, (char *)&ifc) < 0){
        LOGD("Get a list of interface addresses failed");
        return(FAIL); 
    }

    interface_num = ifc.ifc_len / sizeof(struct ifreq);
    LOGD("The number of interfaces is %d\n", interface_num);

    while(interface_num--) {
        LOGD("Net device: %s\n", buf[interface_num].ifr_name);

        if(ioctl(sock_fd, SIOCGIFFLAGS, (char *)&buf[interface_num]) < 0){
            LOGD("Get the active flag word of the device");
            return(FAIL); 
        }

        if(buf[interface_num].ifr_flags & IFF_PROMISC)
            LOGD("Interface is in promiscuous mode\n");

        if(buf[interface_num].ifr_flags & IFF_UP)
            LOGD("Interface is running\n");
        else 
            LOGD("Interface is not running\n");

        if(ioctl(sock_fd, SIOCGIFADDR, (char *)&buf[interface_num]) < 0){
            LOGD("Get interface address failed");
            return(FAIL); 
        }
        if(!strcmp(device_name, buf[interface_num].ifr_name)){
            tmp_addr = inet_ntoa(((struct sockaddr_in*)(&buf[interface_num].ifr_addr))->sin_addr);
            if(tmp_addr == NULL){
                LOGD( "inet_ntoa get addr error\n");
                return(FAIL); 
            }
            memcpy(addr, tmp_addr, ADDR_LEN);
            LOGD("IP address is %s\n", addr);   
            return(SUCCESS); 
        }
    }
    return(FAIL); 
}
/*
 * read dnsmasq.leases file ,get mac, ip, user name
 * @list  user list , @len 允许获取最多用户数
 * 
 */
int get_leases_info(user_list list[], int len){
    FILE *fd_leases = NULL;
    char line_con[LINE_LEN];
    int list_len = 0;
    fd_leases = fopen(DNSMASQ_LEASES, "r");
    if (NULL == fd_leases) { 
        LOGD( "fopen [%s] error\n", DNSMASQ_LEASES);
        return FAIL;
    } 
    char *token;
    while(!feof(fd_leases) && (list_len < len)){
        memset(line_con, 0, LINE_LEN);
        if(NULL == fgets(line_con, LINE_LEN, fd_leases)){
            LOGD( "read lease file fial or end");
            break;
        }
        /*LOGD( "readl line=[%s]", line_con);*/
        if(NULL == (token = strtok(line_con, " "))){
            LOGD( "get suer info error");
            return(FAIL); 
        }
        if (NULL == (token = strtok(NULL, " "))) { 
            LOGD( "get suer info error");
            return(FAIL); 
        } 
        memcpy(list[list_len].userMAC, token, strlen(token));
        if (NULL == (token = strtok(NULL, " "))) { 
            LOGD( "get suer info error");
            return(FAIL); }
        memcpy(list[list_len].userIp, token, strlen(token));
        if (NULL == (token = strtok(NULL, " "))) { 
            LOGD( "get suer info error");
            return(FAIL); 
        }
        memcpy(list[list_len].hostName, token, strlen(token));
        list_len++; 
    }
    fclose(fd_leases);
    return(SUCCESS); 
}
