#C语言日志zlog的使用

[zlog的使用中文手册](http://hardysimpson.github.io/zlog/UsersGuide-CN.html)
已经很详细说明了如何使用，如何配置，这里只是说明一下把它移植到linux系统arm平台上。

###linux arm11移植

[修改后的适合arm平台的](http://pan.baidu.com/s/1bnUsS4v)。从以下步骤可以zlog的移植是非常简单的的：

* 打开src目录下的makefile文件：
```
# Fallback to gcc when $CC is not in $PATH.                                                                 
 CC:=arm-linux-gcc #增加这一句，也就是说用交叉编译工具进行编译
 CC:=$(shell sh -c 'type $(CC) >/dev/null 2>/dev/null && echo $(CC) || echo gcc')
```
* 然后可以直接```make```命令了，这是会在src目录下生成三个动态库```libzlog.so  libzlog.so.1  libzlog.so.1.1```，这时就会想了，这三个动态库我们把那个放到arm平台的目录下呢。下面看一下我们执行```sudo make install```命令输出的日志：
```
tyxm@ub:~/src/zlog-latest-stable$ sudo make install
[sudo] password for tyxm: 
cd src && make install
make[1]: 正在进入目录 `/home/tyxm/src/zlog-latest-stable/src'
mkdir -p /usr/local/include /usr/local/lib
cp -a zlog.h /usr/local/include
cp -a libzlog.so /usr/local/lib/libzlog.so.1.1
cd /usr/local/lib && ln -sf libzlog.so.1.1 libzlog.so.1
cd /usr/local/lib && ln -sf libzlog.so.1 libzlog.so
cp -a libzlog.a /usr/local/lib
make[1]:正在离开目录 `/home/tyxm/src/zlog-latest-stable/src'
```
可以看出关于动态库的部分，首先把libzlog.so拷贝到/usr/local/lib/目录下，并且改名为libzlog.so.1.1，然后为源文件libzlog.so.1.1建立软连接libzlog.so.1，再为软连接libzlog.so.1建立软连接libzlog.so，其实最终连接的库的名字为libzlog.so.1.1，所以移植到arm机上的库的名字必须为这个名字。

* 最后，开始使用了，把通过交叉编译的动态库拷贝到arm Linux lib目录下，把交叉编译生成的可执行文件拷贝到arm Linux上。写个简单的配置文件test_hello.conf(写法可以参考手册)。这个配置文件在一开始要加载，加载的时候要指定配置文件的目录。
```
[formats]
simple  = "%d([%F][%T])[%-5V][%-10F][%-5L] %m%n"
simple2 = "%d.%us %m%n"
[rules]
my_cat.*        >stderr;
my_cat.*        "./tmp.log";simple
my_cat.*        >stdout;simple2
```
