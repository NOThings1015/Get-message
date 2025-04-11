#定义变量指定库文件路径
Lib_path=${shell pwd}/../lib/


#定义变量指定头文件路径
Inc_path=${shell pwd}/../include


#定义变量指定库文件名
Lib_name=test


#定义变量指定服务器,客户端可执行文件名
App_name1=test_server
App_name2=test_client


#设置总目标all,编译生成可执行文件
all:
	make -C `pwd`/../src
	gcc server.c -o ${App_name1} -I${Inc_path} -L${Lib_path} -l${Lib_name} -lsqlite3
	gcc client.c -o ${App_name2} -I${Inc_path} -L${Lib_path} -l${Lib_name} -lsqlite3

#设置run目标执行可执行文件，设置环境变量
run_server:
	LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${Lib_path} ./$(App_name1) $(ARGS)


run_client:
#	export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${Lib_path}
	# 直接运行（临时指定库路径）
	LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${Lib_path} ./$(App_name2) $(ARGS)


clean:
	rm -f ${App_name1}
	rm -f ${App_name2}


