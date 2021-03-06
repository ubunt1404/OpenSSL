#!/bin/bash
openssl genrsa -out ca.key 2048
openssl req -new -x509 -days 30 -key ca.key -out ca.crt
openssl genrsa -out client.key 2048
openssl req -new -key client.key -out client.csr

mkdir -p ./demoCA/newcerts          #当前路径下创建这两个文件夹
touch ./demoCA/index.txt            #创建index.txt文件
touch ./demoCA/serial               #创建serial文件
echo '01' > ./demoCA/serial         #在serial文件中写入“01”

openssl ca -in client.csr -out client.crt -cert ca.crt -keyfile ca.key

