
#include <iostream>
#include <fstream>
#include <string>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include "http.h"
void Http::HttpInit(){

    //用作储存sock相关信息的结构体
    int n;
    struct sockaddr_in sockaddr;
    memset(&sockaddr,0,sizeof(sockaddr));//清空sockaddr的内容

    sockaddr.sin_family = AF_INET;//AF_INET代表socket范围是IPV4的网络
    sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);//INADDR_ANY代表不管数据包目标地址是哪里都接收
    sockaddr.sin_port = htons(8080);//htons代表将本地字节序(port信息)转化成网络字节序，

    sfd = socket(AF_INET,SOCK_STREAM,0);//创建IPV4(AF_INET)的TCP(SOCK_STREAM)的socket

    bind(sfd,(struct sockaddr *) &sockaddr,sizeof(sockaddr));//将socket与sockaddr绑定

    listen(sfd,1024);//开始监听，最长字符串长度1024

    std::cout<<"Please wait for the client information\n"<<std::endl;
}

void Http::HttpFirstLine(){
    int line_end;
    line_end = recv_content.find("\n");
    first_line = recv_content.substr(0, line_end);
   std::cout << "first line:"<<first_line<<std::endl;
}
void Http::HttpPraseMethod(){
    int line_end;
    std::string result;
    line_end = first_line.find(" ");
    result = first_line.substr(0,line_end);
    if(result.compare("POST") == 0){
        method = POST;
    }
    else if(result.compare("GET") == 0){
        method = GET;
    }
}
void Http::HttpPraseUrl(){
    switch(method){
        case GET:
        {
                int begin = 0 ,end = 0;
                begin = first_line.find(" ");
                end = first_line.find(" ",begin + 1);
                http_url = first_line.substr(begin+1 , end-begin-1);
                
                if(http_url.find("?") != std::string::npos)
                    http_url.pop_back();
        
                break;
        }
        case POST:
        {
            int begin = 0,end = 0;
                begin = first_line.find(" ");
                end = first_line.find(" ",end + 1);
                http_url = first_line.substr(begin + 1, end-begin-1);
                break;
        }
        default:
        {
            http_url.erase();
            break;
        }
            
    }
    std::cout << "http_url:"<<http_url<<std::endl;
    std::cout << "http_url_length:" <<http_url.length()<<std::endl;
}

void Http::HttpPraseStatu(){
    int seek =0;
    seek = first_line.find(" ");
    seek = first_line.find(" ",seek + 1);
    http_url = first_line.substr(seek + 1);
    std::cout << "method:"<<method<<std::endl;
}

void Http::do_request(){
    struct stat statbuf;
    

    std::string response="HTTP/1.1 200 ok\r\n/connection: close\r\n\r\n";
    if(http_url.compare("/") == 0){
        
        int s = send(cfd,response.c_str(),response.length(),0);//发送http响应头
        std::cout << "sending index.html" <<std::endl;

        int fd = open("index.html",O_RDONLY);//消息体
        if(stat("index.html",&statbuf)){
            perror("stat");
            exit(EXIT_FAILURE);
        }

        sendfile(cfd,fd,NULL,2500);
        //fwrite(cfd,fd,NULL,statbuf.st_size);//零拷贝发送消息体
        
        close(fd);
    }else{
        if(http_url.find("png") != std::string::npos)
            response += "Content-Type: image/png\r\n";

        int s = send(cfd,response.c_str(),response.length(),0);//发送响应
        std::cout << "sending "<< http_url<<std::endl;
        http_url.insert(0,".");

        if(stat(http_url.c_str(),&statbuf)){
            perror("stat");
            exit(EXIT_FAILURE);
        }
        std::cout << "http_url.c_str():"<<http_url.c_str() <<std::endl;
        FILE *fd;
        if(http_url.find("png") != std::string::npos){
            
            if((fd = fopen(http_url.c_str(),"r")) == NULL);{
                std::cout << "openfile error" <<std::endl;
                exit(1);
            }
            int size;
            fseek(fd, 0, SEEK_END);
            size = ftell(fd);
            fseek(fd,0,SEEK_SET);
            unsigned char msg[size];
            fread(msg, 1 ,size, fd);
            write(cfd,msg,size);
        }
        else{
            fd = fopen(http_url.c_str(),"r");//消息体
            int size;
            fseek(fd, 0, SEEK_END);
            size = ftell(fd);
            fseek(fd,0,SEEK_SET);
            unsigned char msg[size];
            fread(msg, 1 ,size, fd);
            write(cfd,msg,size);
           // sendfile(cfd,fd,NULL,statbuf.st_size);//零拷贝发送消息体
        }
         if(fd == NULL)
        {
             std::cout <<"open file error:"<<__func__<<std::endl;
        }     
        
        fclose(fd);
    }
    
    
}


void Http::HttpLoop(){
    while(1){
        struct sockaddr_in client;
        socklen_t client_addrlength = sizeof(client);
        std::cout<<"one step before accept\n"<<std::endl;
        if((cfd = accept(sfd,(struct sockaddr*)&client,&client_addrlength))==-1)//进入接收阻塞状态，直到收到信息
        {
            std::cout<<"accpet socket error:"<<strerror(errno)<<errno<<std::endl;
            exit(0);
        }

        if(cfd < 0){
            std::cout << "error\n" << std::endl;
        }
        else{
            char buf[1024];
            recv(cfd,buf,1024,0);
            buf[strlen(buf)+1]='\0';
            recv_content = buf;
            HttpFirstLine();
            
            HttpPraseMethod();
            

            HttpPraseUrl();
            
            //HttpPraseStatu();
            do_request();
            //std::cout << buf << "\nsuccussful\n"<<std::endl;
            memset(buf,0,sizeof(buf));
            
            close(cfd);
        }
    }

}