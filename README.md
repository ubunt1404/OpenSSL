## 1. 文件夹说明 
### one_way_authentication
	主要用来学习OpenSSL库，了解什么是单向认证并基于OpenSSL库编程实现C/S模型下的单向认证。
### two_way_authentication
	主要用来学习OpenSSL库，了解什么是双向认证并基于OpenSSL库编程实现C/S模型下的双向认证。
### ssl_proxy_version1
	SSL代理服务器程序Version1版本，该程序运行在树莓派或其他Linux设备上，
	为一些因软、硬件限制(如单片机设备)不能进行SSL/TLS 加密socket通信的设备，
	提供OpenSSL代理转发到目标SSL/TLS主机的服务，主要采用Multithreading+epoll+OpenSSL框架架构来实现。
### ssl_proxy_version2
	SSL代理服务器程序Version2版本,主要迭代了上一个Version1版本，
	新添加了双向认证的功能，能够在具备加密功能的设备、中间代理设备、目标服务器
	三者间建立SSL加密通信，实现数据的安全传输过程。
## 2. 程序设计流程与框架
### ssl_proxy_version1
- 该项目多线程框架图
![多线程架构](https://images.gitee.com/uploads/images/2020/0820/112640_565da807_5112813.png "多线程.png")

- 该项目中子线程内epoll主要完成如下工作
![子线程内epoll做的主要事情](https://images.gitee.com/uploads/images/2020/0820/000909_d2b12245_5112813.png "epoll.png")

### ssl_proxy_version2
多线程框架图
![双向加密SSL代理](https://images.gitee.com/uploads/images/2020/0820/120215_9d3f5808_5112813.png "双向加密下多线程架构.png")
## 3. Version2证书文件匹配关系
![version2中所有证书文件的对应关系](https://images.gitee.com/uploads/images/2020/0819/203738_ef610e12_5112813.jpeg "Version2_cert.jpg")

