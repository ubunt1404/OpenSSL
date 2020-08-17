#!/bin/bash
openssl genrsa -out CA.key 2048
openssl req -new -x509 -days 30 -key CA.key -out CA.crt
openssl genrsa -out stunnel1.key 2048
openssl req -new -key stunnel1.key -out stunnel1.csr

mkdir -p ./demoCA/newcerts          #当前路径下创建这两个文件夹
touch ./demoCA/index.txt            #创建index.txt文件
touch ./demoCA/serial               #创建serial文件
echo '01' > ./demoCA/serial         #在serial文件中写入“01”

openssl ca -in stunnel1.csr -out stunnel1.crt -cert CA.crt -keyfile CA.key

