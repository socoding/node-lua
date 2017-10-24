14. 考虑lua_service的定时gc[考虑一直没有唤醒的服务的lua垃圾回收]

17. 研究lua hook，如何中断死循环和远程调试(输入)

22.to be implemented  / to be fixed

23.udp http https

24.lua socket tcp socket 迁移

25.如何支持数据共享，解决内存过大问题：提供共享api

26.热更新

io 阻塞非阻塞？ setvbuf
{"__tostring", f_tostring},

热更新
优化lua shared proto 锁,不使用原子锁?
udp tty 在线调试

freebsd:
1. pkg 命令安装软件 pkg安装pkg自身 pkg install
2. sshd 服务器配置 /etc/ssh/sshd_config : PermitRootLogin yes
3. make 和 其它操作系统不一样,需要安装gmake

ｗａｒｎｉｎｇ　编译lua的共享dll库,需要在编译时开启 LUA_BUILD_AS_DLL 这个宏(以便在vc下边下编译时导出符号) (外部模块库需要同时开启 LUA_CORE 这个宏,以便使用 LUAMOD_API 这个宏来导出luaopen接口)
ｗａｒｎｉｎｇ　使用lua的dll扩展库,需要把lua整个打成dll动态库,主进程和其它扩展库都需要使用动态链接lua库的方式来编译或生成,不然lua的版本校验将不会通过(动态库实际使用lua.dll,编译和链接时使用lua.lib)
                如果是静态库，那么每个模块加载该静态库后，每个模块都分别有一个拷贝。如果是动态库，每个模块加载该动态库后，都只有一个拷贝。
http://blog.csdn.net/rheostat/article/details/18796911
				
Don't link your C module with liblua.a when you create a.so from it. 
For examples, see my page of Lua libraries: http://www.tecgraf.puc-rio.br/~lhf/ftp/lua/ .
You can link liblua.a statically into your main program but you have to export its symbols by adding -Wl,-E at link time.
That's how the Lua interpreter is built in Linux.

A node completation of lua which support sync and async remote procedure call, and task-multiplexing support(in muti-thread mode with no useless wakeup)


未初始化的指针错误：
有一个选择打开和关闭SDL检查的位置就是：项目属性->配置属性->C/C++->SDL检查，选测是或者否。

按照编译错误的提示来看应该是编译器没有识别inline参数。
查阅了一下inline是c++里面的东西，在c里面使用是会发生错误。
#define inline __inline

Makefile 添加宏定义: -DDEBUG
传递参数: make PLATFORM=aaaa

VS中添加预处理宏的方法：
除了在.c及.h中添加宏定义之外，还可以采用如下方法添加宏定义：
1、若只需要定义一个宏(如#define DEBUG)，可以右键点击工程-->属性-->c/c++-->预处理器-->预处理器定义，点击下拉框中的编辑，输入想要定义的宏；
2、如果还需要定义宏的内容(如#define inline __inline)，可以右键点击工程-->属性-->c/c++-->命令行，在其它选项中输入如下内容： /D"inline"=__inline 。


snprintf()函数并不是标准c/c++中规定的函数，所以在许多编译器中，厂商提供了其相应的实现的版本。
在gcc中，该函数名称就 snprintf()，而在VS中称为 _snprintf。 
所以在需要使用 snprintf()时改成_snprintf就可以了，或则在预编译处加入：
#if _MSC_VER
#define snprintf _snprintf
#define strcasecmp  stricmp
#define strncasecmp  strnicmp 
#endif

#define __STDC_LIMIT_MACROS


    将VS2010工程提交给Git管理时需要哪些文件：
    *.h  *.cpp  *.sln  *.vcxproj  *.vcxproj.filters  *.qrc
    以及Resources目录下的资源文件。
    如果使用Git的过滤配置，则还需要.gitignore文件。
    其他的诸如*.suo  *.sdf  *.opensdf  *.vcxproj.user均可以过滤掉！


