#Android NDK 编写共享库

编译环境为```ubuntu 13.10```版本，工具使用```android-ndk-r5b```,目标系统为```Android 2.3``

###编写动态库
建立jni目录，在jni目录下建文件和目录：```Android.mk  androidNdkShare.c  Application.mk  include  log.h  ``` 。

* androidNdkShare.c ：动态库中函数的定义
* include ：文件androidNdkShare.h为动态库中函数的声明
* log.h : 使用logcat打印日志

###编写可执行文件
建立jni目录，在jni目录下建文件：```Android.mk  Application.mk  log.h  test_exec.c``` 。
* log.h : 使用logcat打印日志
* test_exec.c : 测试动态库

###执行
把libandroidNdkShare.so文件通过adb push到/system/lib/目录下，并改变其权限。把test_exec文件通过adb push到/system/bin/目录下，并改变其权限。执行```./test_exec```，通过```logcat```命令可以看到有如下输出：

    D/androidNdkShare( 4438): The number of interfaces is 3
    D/androidNdkShare( 4438): Net device: wl0.1
    D/androidNdkShare( 4438): Interface is running
    D/androidNdkShare( 4438): Mac address is 5E:E7:BF:E0:34:36
    D/test_exec( 4438): getMAC return iRet=[0], device_mac=[5E:E7:BF:E0:34:36]
    D/androidNdkShare( 4438): The number of interfaces is 3
    D/androidNdkShare( 4438): Net device: wl0.1
    D/androidNdkShare( 4438): Interface is running
    D/androidNdkShare( 4438): Net device: rmnet0
    D/androidNdkShare( 4438): Interface is running
    D/androidNdkShare( 4438): IP address is 10.40.78.162
    D/test_exec( 4438): getIP return iRet=[0], device_ipaddr[10.40.78.162]

###关于日志

log.h文件内是对logcat日志的封装，可以按级别直接把日志打印到logcat的内存里，然后再用logcat工具查看。如果只是想简单的把日志打印到文件里面，下面的简单代码可能是有效的方法：

    #define WRITE_LOG(s, ...) do { \
    FILE *f = fopen("/data/local/tmp/log.txt", "a+"); \
      fprintf(f, s, __VA_ARGS__); \
      fflush(f); \
      fclose(f); \
    } while (0)

如果想要更加全面的日志系统，可以移植log4c或者是zlog过来，进行日志的管理。
