#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include  "log.h"
#include  "../../share_library/jni/include/androidNdkShare.h"

int main(void){

    char device_mac[24] = {0};
    char device_ipaddr[24] = {0};
    int iRet = 0;
    iRet = getMAC("wl0.1", device_mac);
    LOGD("getMAC return iRet=[%d], device_mac=[%s]", iRet, device_mac);
    iRet = getIP("rmnet0", device_ipaddr);
    LOGD("getIP return iRet=[%d], device_ipaddr[%s]", iRet, device_ipaddr);
    return(0); 
}

