typedef struct _user_list{
        char userMAC[20];
        char hostName[64];
        char userIp[32];
    }user_list;
/* get device mac */
int getMAC(char *device_name, char *mac_addr);
/* get device ip */
int getIP(char *device_name, char *addr);
/* get host wifi user list */
int get_leases_info(user_list list[], int len);


