#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <string>
#include <arpa/inet.h>
#include <fstream>
#include <vector>
#include <sstream>
#include <iterator>
#include <filesystem>
#include <iostream>
#include <dirent.h>
#include <fcntl.h>
#include <cstring>
#include <thread>
#include <bits/stdc++.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <openssl/md5.h>
#include <sys/sendfile.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/sendfile.h>

#include <sys/types.h>
#include <sys/stat.h>

using namespace std;


#define SIZE 65536

unsigned char result[MD5_DIGEST_LENGTH];

int isSubstring(string s1, string s2)
{
    int M = s1.length();
    int N = s2.length();
 
    /* A loop to slide pat[] one by one */
    for (int i = 0; i <= N - M; i++) {
        int j;
 
        /* For current index i, check for
 pattern match */
        for (j = 0; j < M; j++)
            if (s2[i + j] != s1[j])
                break;
 
        if (j == M)
            return i;
    }
 
    return -1;
}

// Print the MD5 sum as hex-digits.
void print_md5_sum(unsigned char* md) {
    int i;
    for(i=0; i <MD5_DIGEST_LENGTH; i++) {
            printf("%02x",md[i]);
    }
}

// Get the size of the file by its file descriptor
unsigned long get_size_by_fd(int fd) {
    struct stat statbuf;
    if(fstat(fd, &statbuf) < 0) exit(-1);
    return statbuf.st_size;
}

class ClientSide
{
    int id;
    int port;
    int uniqueid;
    int numberneighbours;
    int numbernneighbours;
    char *ip = "127.0.0.1";
    std::vector<int> neighbourid;
    std::vector<int> neighbourport;
    std::vector<int> neighbouruniqueid;

    std::vector<int> nneighbourid;
    std::vector<int> nneighbourport;
    std::vector<int> nneighbouruniqueid;
    std::vector<std::string> searchfiles;
    int numbersearchfiles;
    std::vector<std::string> ownfiles;
    int numberownfiles;
    public:
    bool received;
    int server_fd, new_socket, valread;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen;
    char unid[100];

    ClientSide(char* p, char* p2)
    {
        std::ifstream configfile;
        configfile.open(p); // Move to CLI
        int linecount = 0;
        received = false;


        while(configfile)
        {
            linecount++;
            std::string text;
            std::getline(configfile, text);

            if(linecount==1)
            {
                std::vector<int> numbers;

                size_t pos = 0;
                while ((pos = text.find(' ')) != std::string::npos) {
                    numbers.push_back(std::stoi(text.substr(0, pos)));
                    text.erase(0, pos + 1);
                }
                if(std::isdigit(text[0])){
                    numbers.push_back(std::stoi(text.substr(0, pos)));
                }

                id = numbers[0];
                port = numbers[1];
                uniqueid = numbers[2];
            }

            else if(linecount==2)
            {
                numberneighbours = std::stoi(text);
            }
            else if(linecount==3)
            {
                std::vector<int> numbers;

                size_t pos = 0;
                while ((pos = text.find(' ')) != std::string::npos) 
                {
                    numbers.push_back(std::stoi(text.substr(0, pos)));
                    text.erase(0, pos + 1);
                }
                if(std::isdigit(text[0])){
                    numbers.push_back(std::stoi(text.substr(0, pos)));
                }

                for(int j=0;j<numberneighbours;j++)
                {
                    neighbourid.push_back(numbers[2*j]);
                    neighbourport.push_back(numbers[2*j+1]);
                }
            }
            else if(linecount==4)
            {
                numbersearchfiles = std::stoi(text); //change:: numbersearchfiles to numberownfiles
            }
            else if(linecount<= 4 + numbersearchfiles) //change:: added this else if as it was storing endfile character as well in searchfiles
            {
                size_t pos = 0;
                while(isalpha(text[text.size()-1])==0  || isdigit(text[text.size()-1])) 
                {  
                    text.pop_back();
                }
                searchfiles.push_back(text.substr(0,text.size()));
            }
        }
        sort(searchfiles.begin(), searchfiles.end());
        DIR *dir; 
        struct dirent *diread;
        std::vector<char *> files;
        bool dir_present = false;
        if ((dir = opendir(p2)) != nullptr) 
        {
            while ((diread = readdir(dir)) != nullptr) 
            {
                if((diread->d_name)[0]!='.' && !((diread->d_name)[0]=='D' && (diread->d_name)[2]=='w' && (diread->d_name)[4]=='l'))//change:: to avoid storing . and .. in filenames
                {
                    files.push_back(diread->d_name);
                    string xx = diread->d_name;
                    ownfiles.push_back(xx);
                }
                else if(((diread->d_name)[0]=='D' && (diread->d_name)[2]=='w' && (diread->d_name)[4]=='l')){
                    dir_present = true;
                }
                
            }
            closedir (dir);
        }
        else 
        {
            perror ("opendir");
            return;
        }

        //Print files
        numberownfiles = files.size();
        numbernneighbours = 0;


        for(int k=0;k<numberownfiles;k++)
        {
            std::cout<<files[k]<<std::endl;
        }
        string path=p2;
        path = path.append("Downloaded/");
        int st;
        if(!dir_present)
        st = mkdir(path.c_str(),S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        
    }

    void doClientStuff(char* p, char* p2)
    {
        
        int usock[10] = {0};
        numbernneighbours = 0;
        for(int i=0;i<numberneighbours;i++)
        {
            int valread;
            struct sockaddr_in serv_addr;
            if ((usock[i] = socket(AF_INET, SOCK_STREAM, 0)) < 0)
            {
                printf("\n Socket creation error \n");
                return;
            }
        
            serv_addr.sin_family = AF_INET;
            serv_addr.sin_port = htons(neighbourport[i]);
            
            
            // Convert IPv4 and IPv6 addresses from text to binary form
            if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0) 
            {
                printf("\nInvalid address/ Address not supported \n");
                return;
            }
            while(1){
            if (connect(usock[i], (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
            {
                // printf("\nConnection Failed \n");
                // return;
            }
            else
            break;
            }
            // send(sock , hello , strlen(hello) , 0 );
            // printf("Hello message sent\n");
            char buffer[1024] = {0};
            valread = 0;  
            while(valread==0){
                valread = read( usock[i] , buffer, 1024);
            }
            string text = buffer;
            std::vector<int> numbers;
            size_t pos = 0;
            while ((pos = text.find('|')) != std::string::npos) 
            {
                numbers.push_back(std::stoi(text.substr(0, pos)));
                text.erase(0, pos + 1);
            }
            // numbers.push_back(std::stoi(text.substr(0, pos)));
            // if(std::isdigit(text[0])){
            //     numbers.push_back(std::stoi(text.substr(0, pos)));
            // }

            numbernneighbours += numbers.size()/2;
            for(int j=0;j<numbers.size()/2;j++)
            {
                if( numbers[2*j]!=id && std::find(neighbourid.begin(), neighbourid.end(), numbers[2*j]) == neighbourid.end() )
                {
                    nneighbourid.push_back(numbers[2*j]);
                    nneighbourport.push_back(numbers[2*j+1]);
                }
                else
                numbernneighbours--;
            }
            
            
        }


        int sock[100] = {0};
        int uid[100]={0};
        int depth[100]={0};
        int nums[100] = {-1};
        int sizes[100] = {-1};
        for(int i=0; i<100; i++){
            nums[i]= -1;
            sizes[i]= -1;
        }
        for(int i=0;i<numberneighbours + numbernneighbours;i++)
        {
            if(i<numberneighbours)
            {
                int valread;
                struct sockaddr_in serv_addr;
                if ((sock[i] = socket(AF_INET, SOCK_STREAM, 0)) < 0)
                {
                    printf("\n Socket creation error \n");
                    return;
                }
            
                serv_addr.sin_family = AF_INET;
                serv_addr.sin_port = htons(neighbourport[i] + 7);
                
                
                // Convert IPv4 and IPv6 addresses from text to binary form
                if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0) 
                {
                    printf("\nInvalid address/ Address not supported \n");
                    return;
                }
                while(1){
                if (connect(sock[i], (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
                {
                    // printf("\nConnection Failed \n");
                    // return;
                }
                else
                break;
                }
                // send(sock , hello , strlen(hello) , 0 );
                // printf("Hello message sent\n");
                char buffer[1024] = {0};
                valread = 0;  
                while(valread==0){
                    valread = read( sock[i] , buffer, 1024);
                }
                int nuid = atoi(buffer);
                cout<<"Connected to "<<neighbourid[i]<<" with unique-ID ";
                printf("%s ",buffer );
                cout<<"on port "<<neighbourport[i]<<endl;
                
                for(int j=0;j<numbersearchfiles;j++)
                {
                    char buffer2[1024] = {0};
                    const char* f = searchfiles[j].c_str();
                    send(sock[i] , f , strlen(f) , 0 );
                    valread = 0;
                    while(valread==0){
                        valread = read( sock[i] , buffer2, 1024);
                    }
                    if(buffer2[0]=='Y')
                    {
                        if((uid[j]!=0 && nuid<uid[j]) || uid[j]==0)
                        {
                            depth[j]=1;
                            uid[j] = nuid;
                            nums[j]=i;
                            string xpk = buffer2;
                            sizes[j] = stoi(xpk.substr(4));
                        }
                    }
                    
                }
                const char* confirm = "SahilForJSec";
                send(sock[i] , confirm , strlen(confirm) , 0 );

            }
            else
            {
                int valread;
                struct sockaddr_in serv_addr;
                if ((sock[i] = socket(AF_INET, SOCK_STREAM, 0)) < 0)
                {
                    printf("\n Socket creation error \n");
                    return;
                }
            
                serv_addr.sin_family = AF_INET;
                serv_addr.sin_port = htons(nneighbourport[i - numberneighbours] + 7);
                
                
                // Convert IPv4 and IPv6 addresses from text to binary form
                if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0) 
                {
                    printf("\nInvalid address/ Address not supported \n");
                    return;
                }
                while(1){
                if (connect(sock[i], (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
                {
                    // printf("\nConnection Failed \n");
                    // return;
                }
                else
                break;
                }
                // send(sock , hello , strlen(hello) , 0 );
                // printf("Hello message sent\n");
                char buffer[1024] = {0};
                valread = 0;  
                while(valread==0){
                    valread = read( sock[i] , buffer, 1024);
                }
                int nuid = atoi(buffer);
                
                for(int j=0;j<numbersearchfiles;j++)
                {
                    char buffer2[1024] = {0};
                    const char* f = searchfiles[j].c_str();
                    send(sock[i] , f , strlen(f) , 0 );
                    valread = 0;
                    while(valread==0){
                        valread = read( sock[i] , buffer2, 1024);
                    }
                    if(buffer2[0]=='Y')
                    {
                        if((uid[j]!=0 && nuid<uid[j] && depth[j]==2) || uid[j]==0)
                        {
                            depth[j]=2;
                            uid[j] = nuid;
                            nums[j]=i;
                            string xpk = buffer2;
                            sizes[j] = stoi(xpk.substr(4));
                            // cout<<"Connected to "<<nneighbourid[i - numberneighbours]<<" with unique-ID ";
                            // printf("%s ",buffer );
                            // cout<<"on port "<<nneighbourport[i - numberneighbours]<<endl;
                        }
                    }
                    
                }
                const char* confirm = "SahilForJSec";
                send(sock[i] , confirm , strlen(confirm) , 0 );
            }

        }
        
        bool printed[100] = {false};
        int nsock[1000];
        for(int i=0;i<numberneighbours + numbernneighbours;i++)
        {
            if(i<numberneighbours)
            {
                int valread;
                struct sockaddr_in serv_addr;
                if ((nsock[i] = socket(AF_INET, SOCK_STREAM, 0)) < 0)
                {
                    printf("\n Socket creation error \n");
                    return;
                }
            
                serv_addr.sin_family = AF_INET;
                serv_addr.sin_port = htons(neighbourport[i] + 6);
                
                
                // Convert IPv4 and IPv6 addresses from text to binary form
                if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0) 
                {
                    printf("\nInvalid address/ Address not supported \n");
                    return;
                }
                while(1){
                if (connect(nsock[i], (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
                {
                    // printf("\nConnection Failed \n");
                    // return;
                }
                else
                break;
                }

                char buffer[1024] = {0};
                valread = 0;  
                while(valread==0){
                    valread = read( nsock[i] , buffer, 1024);
                }
                int nuid = atoi(buffer);
                
                for(int j=0;j<numbersearchfiles;j++)
                {
                    if(nums[j]==-1){
                        if(!printed[j]){
                            cout<<"Found "<<searchfiles[j]<<" at 0 with MD5 0 at depth "<<depth[j]<<endl;
                            printed[j]=true;
                        }
                        
                        else
                        continue;
                    }
                    if(nums[j]==i)
                    {
                        send(nsock[i], searchfiles[j].c_str(), strlen(searchfiles[j].c_str()), 0);

                        int n;
                        string path = p2;
                        path = path.append("/Downloaded/");

                        path = path.append(searchfiles[j]);
                        FILE* fp=fopen(path.c_str(),"w");
                        while(1){
                            char buffer100[SIZE];
                            n = recv(nsock[i],buffer100,SIZE, 0);
                            if (n <= 0){
                                bzero(buffer100,sizeof(buffer100));
                                break;
                            }
                            // file_size = file_size - n;
                            

                            fwrite(buffer100,1,n,fp);
                            fflush(fp);
                            sizes[j] = sizes[j] - n;
                            

                            if (sizes[j] <= 0){
                                // cout<<"break hua"<<endl;
                                bzero(buffer100,sizeof(buffer100));
                                break;
                            }
                            
                            bzero(buffer100,sizeof(buffer100));
                        }
                        // cout<<"Found "<<searchfiles[j]<<" at "<<uid[j]<<" with MD5 0 at depth "<<depth[j]<<endl;
                        string path1 = p2;
                        // string path1 = "files/client";
                        // path.append(path1);
                        // path = path.append(to_string(id));
                        path1 = path1.append("Downloaded/");
                        path1 = path1.append(searchfiles[j]);
                        FILE* fp1;
                        fp1 = fopen(path1.c_str(),"r");

                        unsigned long file_size;
                        char* file_buffer;
                        int fd;
                        fd = fileno(fp1);
                        file_size = get_size_by_fd(fd);
                        file_buffer = static_cast<char*>(mmap(0, file_size, PROT_READ, MAP_SHARED, fd, 0));
                        MD5((unsigned char*) file_buffer, file_size, result);
                        munmap(file_buffer, file_size); 
                        // // cout<<endl;
                        cout<<"Found "<<searchfiles[j]<<" at "<<uid[j]<<" with MD5 ";
                        print_md5_sum(result);
                        cout<<" at depth "<<depth[j]<<endl;
                        fclose(fp);   
                    }

                }
                send(nsock[i], "SahilForJSec", strlen("SahilForJSec"), 0);
            }
            else
            {
                int valread;
                struct sockaddr_in serv_addr;
                if ((nsock[i] = socket(AF_INET, SOCK_STREAM, 0)) < 0)
                {
                    printf("\n Socket creation error \n");
                    return;
                }
            
                serv_addr.sin_family = AF_INET;
                serv_addr.sin_port = htons(nneighbourport[i - numberneighbours] + 6);
                
                
                // Convert IPv4 and IPv6 addresses from text to binary form
                if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0) 
                {
                    printf("\nInvalid address/ Address not supported \n");
                    return;
                }
                while(1){
                if (connect(nsock[i], (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
                {
                    // printf("\nConnection Failed \n");
                    // return;
                }
                else
                break;
                }

                char buffer[1024] = {0};
                valread = 0;  
                while(valread==0){
                    valread = read( nsock[i] , buffer, 1024);
                }
                int nuid = atoi(buffer);
                
                for(int j=0;j<numbersearchfiles;j++)
                {
                    if(nums[j]==-1){
                        if(!printed[j]){
                            cout<<"Found "<<searchfiles[j]<<" at 0 with MD5 0 at depth "<<depth[j]<<endl;
                            printed[j]=true;
                        }
                        
                        else
                        continue;
                    }
                    if(nums[j]==i)
                    {
                        send(nsock[i], searchfiles[j].c_str(), strlen(searchfiles[j].c_str()), 0);

                        int n;
                        string path = p2;
                        path = path.append("/Downloaded/");

                        path = path.append(searchfiles[j]);
                        FILE* fp=fopen(path.c_str(),"w");
                        while(1){
                            char buffer100[SIZE];
                            n = recv(nsock[i],buffer100,SIZE, 0);
                            if (n <= 0){
                                bzero(buffer100,sizeof(buffer100));
                                break;
                            }
                            // file_size = file_size - n;
                            

                            fwrite(buffer100,1,n,fp);
                            fflush(fp);
                            sizes[j] = sizes[j] - n;
                            

                            if (sizes[j] <= 0){
                                // cout<<"break hua"<<endl;
                                bzero(buffer100,sizeof(buffer100));
                                break;
                            }
                            
                            bzero(buffer100,sizeof(buffer100));
                        }
                        // cout<<"Found "<<searchfiles[j]<<" at "<<uid[j]<<" with MD5 0 at depth "<<depth[j]<<endl;
                        string path1 = p2;
                        // string path1 = "files/client";
                        // path.append(path1);
                        // path = path.append(to_string(id));
                        path1 = path1.append("Downloaded/");
                        path1 = path1.append(searchfiles[j]);
                        FILE* fp1;
                        fp1 = fopen(path1.c_str(),"r");

                        unsigned long file_size;
                        char* file_buffer;
                        int fd;
                        fd = fileno(fp1);
                        file_size = get_size_by_fd(fd);
                        file_buffer = static_cast<char*>(mmap(0, file_size, PROT_READ, MAP_SHARED, fd, 0));
                        MD5((unsigned char*) file_buffer, file_size, result);
                        munmap(file_buffer, file_size); 
                        // // cout<<endl;
                        cout<<"Found "<<searchfiles[j]<<" at "<<uid[j]<<" with MD5 ";
                        print_md5_sum(result);
                        cout<<" at depth "<<depth[j]<<endl;
                        fclose(fp);             
                    }

                }
                send(nsock[i], "SahilForJSec", strlen("SahilForJSec"), 0);
            }
        }
    }
};


class ServerSide
{
    int id;
    int port;
    int uniqueid;
    int numberneighbours;
    int numbernneighbours;
    char *ip = "127.0.0.1";
    std::vector<int> neighbourid;
    std::vector<int> neighbourport;
    std::vector<int> neighbouruniqueid;

    std::vector<int> nneighbourid;
    std::vector<int> nneighbourport;
    std::vector<int> nneighbouruniqueid;
    std::vector<std::string> searchfiles;
    int numbersearchfiles;
    std::vector<std::string> ownfiles;
    int numberownfiles;
    public:
    bool received;
    int server_fd, valread;
    int new_socket[10];
    int new_socket2[100];
    struct sockaddr_in address;
    int opt = 1;
    int addrlen;
    char unid[100];

    ServerSide(char* p, char* p2)
    {

        std::ifstream configfile;
        configfile.open(p); // Move to CLI
        int linecount = 0;
        received = false;


        while(configfile)
        {
            linecount++;
            std::string text;
            std::getline(configfile, text);

            if(linecount==1)
            {
                std::vector<int> numbers;

                size_t pos = 0;
                while ((pos = text.find(' ')) != std::string::npos) {
                    numbers.push_back(std::stoi(text.substr(0, pos)));
                    text.erase(0, pos + 1);
                }
                if(std::isdigit(text[0])){
                    numbers.push_back(std::stoi(text.substr(0, pos)));
                }

                id = numbers[0];
                port = numbers[1];
                uniqueid = numbers[2];
                
            }
            else if(linecount==2)
            {
                numberneighbours = std::stoi(text);
            }
            else if(linecount==3)
            {
                std::vector<int> numbers;

                size_t pos = 0;
                while ((pos = text.find(' ')) != std::string::npos) 
                {
                    numbers.push_back(std::stoi(text.substr(0, pos)));
                    text.erase(0, pos + 1);
                }
                if(std::isdigit(text[0])){
                    numbers.push_back(std::stoi(text.substr(0, pos)));
                }

                for(int j=0;j<numberneighbours;j++)
                {
                    neighbourid.push_back(numbers[2*j]);
                    neighbourport.push_back(numbers[2*j+1]);
                    
                }
            }
            else if(linecount==4)
            {
                numbersearchfiles = std::stoi(text); //change:: numbersearchfiles to numberownfiles
            }
            else if(linecount<= 4 + numbersearchfiles) //change:: added this else if as it was storing endfile character as well in searchfiles
            {
                size_t pos = 0;
               
                while(isalpha(text[text.size()-1])==0  || isdigit(text[text.size()-1])) 
                {
                    //std::cout<<"inif"<<std::endl;
                    //searchfiles.push_back(text.substr(0, pos));
                    //text.erase(0, pos + 1);
                    text.pop_back();
                }
                searchfiles.push_back(text.substr(0,text.size()));
            }
        }
        sort(searchfiles.begin(), searchfiles.end());
        DIR *dir; 
        struct dirent *diread;
        std::vector<char *> files;

        if ((dir = opendir(p2)) != nullptr) 
        {
            while ((diread = readdir(dir)) != nullptr) 
            {
                if((diread->d_name)[0]!='.' && !((diread->d_name)[0]=='D' && (diread->d_name)[2]=='w' && (diread->d_name)[4]=='l'))//change:: to avoid storing . and .. in filenames
                {
                    files.push_back(diread->d_name);
                    string xx = diread->d_name;
                    ownfiles.push_back(xx);
                }
                
            }
            closedir (dir);
        }
        else 
        {
            perror ("opendir");
            return;
        }

        numberownfiles = files.size();
        
        
    }


    void doServerStuff(char* p, char* p2)
    {
        // Creating socket file descriptor
        if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
        {
            perror("socket failed");
            exit(EXIT_FAILURE);
        }
        
        
        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                                                    &opt, sizeof(opt)))
        {
            perror("setsockopt");
            exit(EXIT_FAILURE);
        }
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons( port );
        
        if (bind(server_fd, (struct sockaddr *)&address,
                                    sizeof(address))<0)
        {
            perror("bind failed");
            exit(EXIT_FAILURE);
        }

        for(int counter=0;counter<numberneighbours;counter++)
        {
            if (listen(server_fd, 3) < 0)
            {
                perror("listen");
                exit(EXIT_FAILURE);
            }
            while(1)
            {
                if ((new_socket[counter] = accept(server_fd, (struct sockaddr *)&address,
                            (socklen_t*)&addrlen))<0)
                {
                    // perror("accept");
                    // exit(EXIT_FAILURE);
                }
                else
                break;
            }

            string sss = "";
            for(int i=0;i<numberneighbours;i++)
            {
                sss.append(to_string(neighbourid[i]));
                sss.append("|");
                sss.append(to_string(neighbourport[i]));
                sss.append("|");
                // sss.append("|");
                // sss.append(to_string(neighbouruniqueid[i]));
            }
            send(new_socket[counter] , sss.c_str() , strlen(sss.c_str()) , 0 );
        } 
        return;


    }
};



class ServerSide2
{
    int id;
    int port;
    int uniqueid;
    int numberneighbours;
    int numbernneighbours;
    char *ip = "127.0.0.1";
    std::vector<int> neighbourid;
    std::vector<int> neighbourport;
    std::vector<int> neighbouruniqueid;

    std::vector<int> nneighbourid;
    std::vector<int> nneighbourport;
    std::vector<int> nneighbouruniqueid;
    std::vector<std::string> searchfiles;
    int numbersearchfiles;
    std::vector<std::string> ownfiles;
    int numberownfiles;
    public:
    bool received;
    int server_fd, valread;
    int new_socket[100];
    int new_socket2[100];
    struct sockaddr_in address;
    int opt = 1;
    int addrlen;
    char unid[100];

    ServerSide2(char* p, char* p2)
    {

        std::ifstream configfile;
        configfile.open(p); // Move to CLI
        int linecount = 0;
        received = false;


        while(configfile)
        {
            linecount++;
            std::string text;
            std::getline(configfile, text);

            if(linecount==1)
            {
                std::vector<int> numbers;

                size_t pos = 0;
                while ((pos = text.find(' ')) != std::string::npos) {
                    numbers.push_back(std::stoi(text.substr(0, pos)));
                    text.erase(0, pos + 1);
                }
                if(std::isdigit(text[0])){
                    numbers.push_back(std::stoi(text.substr(0, pos)));
                }

                id = numbers[0];
                port = numbers[1] + 7;
                uniqueid = numbers[2];
                
            }
            else if(linecount==2)
            {
                numberneighbours = std::stoi(text);
            }
            else if(linecount==3)
            {
                std::vector<int> numbers;

                size_t pos = 0;
                while ((pos = text.find(' ')) != std::string::npos) 
                {
                    numbers.push_back(std::stoi(text.substr(0, pos)));
                    text.erase(0, pos + 1);
                }
                if(std::isdigit(text[0])){
                    numbers.push_back(std::stoi(text.substr(0, pos)));
                }

                for(int j=0;j<numberneighbours;j++)
                {
                    neighbourid.push_back(numbers[2*j]);
                    neighbourport.push_back(numbers[2*j+1] + 7);
                    
                }
            }
            else if(linecount==4)
            {
                numbersearchfiles = std::stoi(text); //change:: numbersearchfiles to numberownfiles
            }
            else if(linecount<= 4 + numbersearchfiles) //change:: added this else if as it was storing endfile character as well in searchfiles
            {
                size_t pos = 0;
               
                while(isalpha(text[text.size()-1])==0 || isdigit(text[text.size()-1])) 
                {
                    //std::cout<<"inif"<<std::endl;
                    //searchfiles.push_back(text.substr(0, pos));
                    //text.erase(0, pos + 1);
                    text.pop_back();
                }
                searchfiles.push_back(text.substr(0,text.size()));
            }
        }
        sort(searchfiles.begin(), searchfiles.end());
        DIR *dir; 
        struct dirent *diread;
        std::vector<char *> files;

        if ((dir = opendir(p2)) != nullptr) 
        {
            while ((diread = readdir(dir)) != nullptr) 
            {
                if((diread->d_name)[0]!='.' && !((diread->d_name)[0]=='D' && (diread->d_name)[2]=='w' && (diread->d_name)[4]=='l'))//change:: to avoid storing . and .. in filenames
                {
                    files.push_back(diread->d_name);
                    string xx = diread->d_name;
                    ownfiles.push_back(xx);
                }
                
            }
            closedir (dir);
        }
        else 
        {
            perror ("opendir");
            return;
        }

        numberownfiles = files.size();
        
        
    }


    void doServerStuff(char* p, char* p2)
    {
        // Creating socket file descriptor
        if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
        {
            perror("socket failed");
            exit(EXIT_FAILURE);
        }
        
        
        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                                                    &opt, sizeof(opt)))
        {
            perror("setsockopt");
            exit(EXIT_FAILURE);
        }
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons( port );
        
        if (bind(server_fd, (struct sockaddr *)&address,
                                    sizeof(address))<0)
        {
            perror("bind failed");
            exit(EXIT_FAILURE);
        }
        int counter = 0;


        while(true)
        {
            if (listen(server_fd, 3) < 0)
            {
                perror("listen");
                exit(EXIT_FAILURE);
            }
            while(1)
            {
                if ((new_socket[counter] = accept(server_fd, (struct sockaddr *)&address,
                            (socklen_t*)&addrlen))<0)
                {
                    // perror("accept");
                    // exit(EXIT_FAILURE);
                }
                else
                break;
            }
            const char *hello = to_string(uniqueid).c_str();
            send(new_socket[counter] , hello , strlen(hello) , 0 );
            

            while(1)
            {
                char buffer[1024] = {0};
                valread = read( new_socket[counter] , buffer, 1024);
                string s = buffer;
                
                if(s.compare("SahilForJSec") == 0)
                break;
                bool has = false;
                for(int k=0;k<numberownfiles;k++)
                {
                    if(s.compare(ownfiles[k])==0)
                    has=true;
                }
                if(has)
                {
                    string mlkk = "YES";

                    string filename = p2;
                    filename = filename.append(s);
                    ifstream in_file(filename, ios::binary);
                    in_file.seekg(0, ios::end);
                    int file_size = in_file.tellg();
                    string confirm = mlkk.append("|").append(to_string(file_size)); 
                    send(new_socket[counter] , confirm.c_str() , strlen(confirm.c_str()) , 0 );
                }
                else
                {
                   
                    char* confirm = "NO";
                    send(new_socket[counter] , confirm , strlen(confirm) , 0 );
                }
            }
            // valread = read( new_socket , buffer, 1024);
            // printf("%s\n",buffer );

            // send(new_socket , tmp , strlen(hello) , 0 );
            // send(new_socket[counter] , hello , strlen(hello) , 0 );
            
            counter++;
            // printf("Hello message sent\n");
        } 
        return;

    }
};



class ServerSide3
{
    int id;
    int port;
    int uniqueid;
    int numberneighbours;
    int numbernneighbours;
    char *ip = "127.0.0.1";
    std::vector<int> neighbourid;
    std::vector<int> neighbourport;
    std::vector<int> neighbouruniqueid;

    std::vector<int> nneighbourid;
    std::vector<int> nneighbourport;
    std::vector<int> nneighbouruniqueid;
    std::vector<std::string> searchfiles;
    int numbersearchfiles;
    std::vector<std::string> ownfiles;
    int numberownfiles;
    public:
    bool received;
    int server_fd, valread;
    int new_socket[10];
    int new_socket2[100];
    struct sockaddr_in address;
    int opt = 1;
    int addrlen;
    char unid[100];

    ServerSide3(char* p, char* p2)
    {

        std::ifstream configfile;
        configfile.open(p); // Move to CLI
        int linecount = 0;
        received = false;


        while(configfile)
        {
            linecount++;
            std::string text;
            std::getline(configfile, text);

            if(linecount==1)
            {
                std::vector<int> numbers;

                size_t pos = 0;
                while ((pos = text.find(' ')) != std::string::npos) {
                    numbers.push_back(std::stoi(text.substr(0, pos)));
                    text.erase(0, pos + 1);
                }
                if(std::isdigit(text[0])){
                    numbers.push_back(std::stoi(text.substr(0, pos)));
                }

                id = numbers[0];
                port = numbers[1] + 6;
                uniqueid = numbers[2];
                
            }
            else if(linecount==2)
            {
                numberneighbours = std::stoi(text);
            }
            else if(linecount==3)
            {
                std::vector<int> numbers;

                size_t pos = 0;
                while ((pos = text.find(' ')) != std::string::npos) 
                {
                    numbers.push_back(std::stoi(text.substr(0, pos)));
                    text.erase(0, pos + 1);
                }
                if(std::isdigit(text[0])){
                    numbers.push_back(std::stoi(text.substr(0, pos)));
                }

                for(int j=0;j<numberneighbours;j++)
                {
                    neighbourid.push_back(numbers[2*j]);
                    neighbourport.push_back(numbers[2*j+1] + 6);
                    
                }
            }
            else if(linecount==4)
            {
                numbersearchfiles = std::stoi(text); //change:: numbersearchfiles to numberownfiles
            }
            else if(linecount<= 4 + numbersearchfiles) //change:: added this else if as it was storing endfile character as well in searchfiles
            {
                size_t pos = 0;
               
                while(isalpha(text[text.size()-1])==0 || isdigit(text[text.size()-1])) 
                {
                    //std::cout<<"inif"<<std::endl;
                    //searchfiles.push_back(text.substr(0, pos));
                    //text.erase(0, pos + 1);
                    text.pop_back();
                }
                searchfiles.push_back(text.substr(0,text.size()));
            }
        }
        sort(searchfiles.begin(), searchfiles.end());
        DIR *dir; 
        struct dirent *diread;
        std::vector<char *> files;

        if ((dir = opendir(p2)) != nullptr) 
        {
            while ((diread = readdir(dir)) != nullptr) 
            {
                if((diread->d_name)[0]!='.' && !((diread->d_name)[0]=='D' && (diread->d_name)[2]=='w' && (diread->d_name)[4]=='l'))//change:: to avoid storing . and .. in filenames
                {
                    files.push_back(diread->d_name);
                    string xx = diread->d_name;
                    ownfiles.push_back(xx);
                }
                
            }
            closedir (dir);
        }
        else 
        {
            perror ("opendir");
            return;
        }

        numberownfiles = files.size();
        
        
    }


    void doServerStuff(char* p, char* p2)
    {
        
        // Creating socket file descriptor
        if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
        {
            perror("socket failed");
            exit(EXIT_FAILURE);
        }
        
        
        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                                                    &opt, sizeof(opt)))
        {
            perror("setsockopt");
            exit(EXIT_FAILURE);
        }
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons( port );
        
        if (bind(server_fd, (struct sockaddr *)&address,
                                    sizeof(address))<0)
        {
            perror("bind failed");
            exit(EXIT_FAILURE);
        }
        int counter = 0;
        while(true)
        {
            if (listen(server_fd, 3) < 0)
            {
                perror("listen");
                exit(EXIT_FAILURE);
            }
            while(1)
            {
                if ((new_socket[counter] = accept(server_fd, (struct sockaddr *)&address,
                            (socklen_t*)&addrlen))<0)
                {
                    // perror("accept");
                    // exit(EXIT_FAILURE);
                }
                else
                break;
            }
            const char *hello = to_string(uniqueid).c_str();
            send(new_socket[counter] , hello , strlen(hello) , 0 );
            

            while(1)
            {
                char buffer[1024] = {0};
                valread = read( new_socket[counter] , buffer, 1024);
                string s = buffer;
                
                if(s.compare("SahilForJSec") == 0){
                    // cout<<"read break from "<<counter<<endl;
                    break;
                }
                

                string filename = p2;
                filename = filename.append(s);

                FILE* fp = fopen(filename.c_str(), "rb");
                int fd = fileno(fp);

                struct stat file_stat;
                fstat(fd, &file_stat);
                sendfile(new_socket[counter], fd, NULL, file_stat.st_size);
                fclose(fp);

            }
            // valread = read( new_socket , buffer, 1024);
            // printf("%s\n",buffer );

            // send(new_socket , tmp , strlen(hello) , 0 );
            // send(new_socket[counter] , hello , strlen(hello) , 0 );
            
            counter++;
            // printf("Hello message sent\n");
        } 
        return;
    }

};


void foo(char config[100], char pathtofiles[100])
{
    ClientSide c(config, pathtofiles);
    c.doClientStuff(config, pathtofiles);
}

void foo2(char config[100], char pathtofiles[100])
{
    ServerSide c(config, pathtofiles);
    c.doServerStuff(config, pathtofiles);
}

void foo3(char config[100], char pathtofiles[100])
{
    ServerSide2 c(config, pathtofiles);
    c.doServerStuff(config, pathtofiles);
}

void foo4(char config[100], char pathtofiles[100])
{
    ServerSide3 c(config, pathtofiles);
    c.doServerStuff(config, pathtofiles);
}





int main(int argc, char** argv)
{
    std::string path = argv[1];
    char config[100];
    std::strcpy(config, path.c_str());
    std::string filespath = argv[2]; 
    char pathtofiles[100];
    std::strcpy(pathtofiles, filespath.c_str());


    std::thread th(foo, config, pathtofiles);
    std::thread th2(foo2, config, pathtofiles); 
    std::thread th3(foo3, config, pathtofiles); 
    std::thread th4(foo4, config, pathtofiles); 

    while(true)
    {

    }

}