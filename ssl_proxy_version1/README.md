## 简介
	SSL代理服务器程序Version1版本，该程序运行在树莓派或其他Linux设备上，
	为一些因软、硬件限制(如单片机设备)不能进行SSL/TLS 加密socket通信的设备，
	提供OpenSSL代理转发到目标SSL/TLS主机的服务，主要采用Multithreading+epoll+OpenSSL的框架来实现，
  ini文件解析使用开源iniparser库提供的API来完成 https://github.com/ndevilla/iniparser
  
## 程序设计流程与框架
### 该项目多线程框架图
  
  一个线程监听一个端口，每个线程会监听解析文件中指定的相应端口，在配置文件中可以指定不同的端口连到相应的SSL服务器，
  例如监听到来自端口2021的数据转发到主机Server1；而来自端口2022的数据则转发到主机Server2；
  ![多线程架构](https://images.gitee.com/uploads/images/2020/0820/112640_565da807_5112813.png "多线程.png")

### 该项目中子线程内epoll主要完成如下工作

  - 如果监听的端口有连接请求则accept传统设备的普通socket连接请求，
  并触发一个SSL socket连接请求连接到目的主机服务器并维护这条链路
  
  - 如果监听的普通socket客户端有数据到来，则将数据转发到相应的目标SSL主机上
  
  - 同样如果收到来自目标SSL主机上的数据，则转发给监听的普通socket客户端

![子线程内epoll做的主要事情](https://images.gitee.com/uploads/images/2020/0820/000909_d2b12245_5112813.png "epoll.png")

