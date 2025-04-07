#定义变量指定生成的静态库和动态库名字
Lib_name=test

#定义一个变量指定库文件安装路径
Lib_path=`pwd`/../lib/

#定义一个变量指定头文件安装路径
Inc_path=`pwd`/../include

#定义编译器（若需要交叉编译，只需在此添加即可）
CC=gcc
AR=ar

#定义一个总目标,有两个依赖
all: dynamic_lib static_lib
	@make install
	@make clear


#dynamic_lib目标用来编译生成动态库，是all总目标的·一个依赖：
dynamic_lib:
	${CC} -shared -fPIC *.c -o lib${Lib_name}.so


#static_lib目标用来编译生成静态库，是all目标的一个依赖：
static_lib:
	${CC} -c *.c
	${AR} -rsc lib${Lib_name}.a *.o

#install是一个单独的目标，用于将编译生成的库文件拷贝到相应的安装路径下
#总目标all下有一个行为会执行该目标
install:
	cp lib${Lib_name}.* ${Lib_path}
	cp *.h ${Inc_path}


#uninstall是一个单独的目标，可以用于删除之前安装的库文件和头文件
uninstall:
	rm -f ${Lib_path}/lib${Lib_name}
	rm -f ${Inc_path}/*.h


#clear是一个单独的目标，用于删除编译生成的object临时文件，是all总目标的一个依赖
clear:
	rm -f *.o
	rm -f lib${Lib_name}.*

#clean是一个单独的目标，有一个依赖clear,可以删除object临时文件和库文件
clean: clear
	rm -f lib${Lib_name}.*


