# Lab1定制环境
## 相较于原环境的特点
* 使用docker-compose简化环境启动
* 默认使用root用户
* 集成gdbserver，便于远程调试
* 集成oh-my-zsh，提供更好的shell体验
* 修改Makefile，使用C++11标准编译

## 使用方法
宿主机必须安装`docker-compose`:
> pip3 install docker-compose

执行目录下`enter-env.sh`，即进入容器内`zsh`。

## 远程调试教程
远程调试功能通过`gdbserver`实现，原理为容器内运行`gdbserver`服务端，并附加到在容器内编译的测试程序上，而后在本地开启gdb作为客户端，并连接到容器内的`gdbserver`。

注意：`gdbserver`开启后会等待gdb的连接，在未连接前，其不会开始执行被附加的进程，因此若`gdbserver`的表现和预想不一致，请先检查是否已连接gdb。

`docker-compose`中指定将容器内6666端口转发到宿主机6666端口，因此我们需要在容器内`0.0.0.0:6666`上开启`gdbserver`：
> gdbserver 0.0.0.0:6666 <test_program>

随后在宿主机上使用gdb进行连接，注意宿主机的6666端口已被转发到容器内的gdbserver上：
> $ gdb
>
> gdb> target remote 127.0.0.1:6666

即可使用gdb命令调试容器内运行的测试程序。

对于支持远程调试的IDE或编辑器，如Clion或vscode，可以直接添加远程调试配置，并指定目标为`127.0.0.1:6666`，随后开启调试会话即可调试程序。

需要注意，每次终止调试后`gdbserver`也会退出，因此需要再次运行`gdbserver`来再次调试。

对于Part1,可在**容器内**执行`debug_part1.sh`来快速开启`gdbserver`并运行`part1_tester`。

对于Part2，已修改`start.sh`，使其通过`gdbserver`启动`yfs_client`。由于前述`gdbserver`的特性，修改后的`start.sh`执行`gdbserver`后，将提示用户连接gdb，然后按enter方继续执行，因为只有这样才能让`yfs_client`开始执行并挂载目录。