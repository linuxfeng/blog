#Android ndk core dump调试

###PC环境

* PC环境：```Linux ub 3.11.0-12-generic #19-Ubuntu SMP Wed Oct 9 16:12:00 UTC 2013 i686 i686 i686 GNU/Linux```
* Android NDK环境：```android-ndk-r5b```
* PC主机关于ndk的环境变量：
```
export NDK_HOME=$HOME/tar/android-ndk-r5b
export LINUX_ANDROIDEABI=$NDK_HOME/toolchains/arm-linux-androideabi-4.4.3/prebuilt/linux-x86/bin
export PATH=$PATH:$NDK_HOME:$LINUX_ANDROIDEABI
export NDK_MODULE_PATH=$NDK_HOME/sources
export ANDROID_NDK=$NDK_HOME
```
ndk的环境配置好以后，在终端里面非jni目录输入ndk-build会有提示：```Android NDK: Could not find application project directory !    
Android NDK: Please define the NDK_PROJECT_PATH variable to point to it.    
/home/tyxm/tar/android-ndk-r5b/build/core/build-local.mk:85: *** Android NDK: Aborting    。 停止。``` 这时根据命令补全也能找到arm-linux-androideabi-gdb命令，这个就是我们等会调试core dump要用的命令。

###Android机环境设置

* Android环境是2.3的
* 支持Core文件抛出：通过adb进入Android终端，输入命令：
```
# ulimit -a
ulimit -a
time(seconds)        unlimited
file(blocks)         unlimited
data(kbytes)         unlimited
stack(kbytes)        8192
coredump(blocks)     0
memory(kbytes)       unlimited
locked memory(kbytes) 64
process(processes)   987
nofiles(descriptors) 1024
```
其中coredump(blocks)后面的显示为0，这就是说当jni程序异常崩溃的时候抛出的
core文件的大小为0，即不会抛出core文件。这么我们可以把它是指为指定的大小，也可以通过命令```ulimit -c unlimited```设置为不限制大小。以下是两种方法：

1. 这种方法认为通过```ulimit -c ```不能是Android生成core dump,所以想到了修改init.rc文件。在init.rc中```setrlimit 13 40 40下```增加一条记录: ```setrlimit 4 -1 -1```。init.rc文件在已经启动的Android系统里无法修改，只有在刷系统前修改，所以可以在制作系统的时候做如上修改。(此方法没有验证)
2. 另一种是通过```ulimit -c unlimited```命令修改。我们的系统会在init.rc里面启动一个脚本，把这个命令加入脚本中。把要扑捉异常的程序也在这个脚本中启动使他们处在同一个环境(一定要把扑捉异常的程序也在这个脚本中启动。测试的时候发现在脚本内设置的```ulimit -c unlimited```，在脚本外再次用```ulimit -c ```看结果的时候，会发现依然为0)。
* ```echo "/data/logs/core-%e" > /proc/sys/kernel/core_pattern```设置core dump文件的生成的目录和名字，%e会替换成进程的名字，还有其他的参数可以加。

###例子

jni目录：

    /* test.c */
    #include <stdio.h>

    static void sub(void);
    int main(void){
        sub();
        return(0); 
    }
    static void sub(void){
        int *p = NULL;
        printf("%d", *p);
    }

    /*Android.mk */
    LOCAL_PATH := $(call my-dir)
    include $(CLEAR_VARS)  
    LOCAL_MODULE := test  
    LOCAL_SRC_FILES += $(notdir $(wildcard $(LOCAL_PATH)/*.c))
    LOCAL_C_INCLUDES := $(INCLUDES)
    include $(BUILD_EXECUTABLE)

    /* Application.mk*/
    APP_ABI :=  armeabi-v7a

* 在jni目录下执行命令ndk-build,会在上级目录下生成另外两个文件夹```libs  obj```,obj下面生成的是一些编译的中间文件和带符号的可执行文件，libs目录下生成的是一些动态库和可执行文件。在解析core dump文件的时候一定要用obj下面的带符号的可执行文件(libs下面的可执行文件生成的core dump文件能不能通过obj下面的带符号的可执行文件解析成功需要大量测试，通过test测试的时候是可以的)。
* 把生成core-test文件拿到obj/local/armeabi-v7a目录下面，通过命令```arm-linux-androideabi-gdb ./test```输出：

        GNU gdb 6.6
        Copyright (C) 2006 Free Software Foundation, Inc.
        GDB is free software, covered by the GNU General Public License, and you are
        welcome to change it and/or distribute copies of it under certain conditions.
        Type "show copying" to see the conditions.
        There is absolutely no warranty for GDB.  Type "show warranty" for details.
        This GDB was configured as "--host=x86_64-linux-gnu --target=arm-elf-linux"...
        (gdb) core-file core-test(输入的信息)
        Error while mapping shared library sections:
        /system/bin/linker: No such file or directory.
        Error while mapping shared library sections:
        libc.so: Success.
        Error while mapping shared library sections:
        libstdc++.so: Success.
        Error while mapping shared library sections:
        libm.so: Success.
        Symbol file not found for /system/bin/linker
        Symbol file not found for libc.so
        Symbol file not found for libstdc++.so
        Symbol file not found for libm.so
        warning: Unable to find dynamic linker breakpoint function.
        GDB will be unable to debug shared library initializers
        and track explicitly loaded dynamic code.
        Core was generated by `./test'.
        Program terminated with signal 11, Segmentation fault.
        #0  0x00008378 in main () at /home/tyxm/src/workspace/RBMaster/dep/Android_core/jni/test.c:13
        13      printf("%d", *p);
        (gdb) 
