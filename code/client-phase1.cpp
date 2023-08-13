#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
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
// #include <direct.h>

using namespace std;


#define SIZE 1024

void send_file(FILE *fp, int sockfd){
  int n;
  char data[SIZE] = {0};
 
  while(fgets(data, SIZE, fp) != NULL) {
    if (send(sockfd, data, sizeof(data), 0) == -1) {
      perror("[-]Error in sending file.");
      exit(1);
    }
    bzero(data, SIZE);
  }
}


class ClientSide
{
    int id;
    int port;
    int uniqueid;
    int numberneighbours;
    char *ip = "127.0.0.1";
    std::vector<int> neighbourid;
    std::vector<int> neighbourport;
    std::vector<int> neighbouruniqueid;
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
                numbersearchfiles = std::stoi(text); 
            }
            else if(linecount<= 4 + numbersearchfiles) //change:: added this else if as it was storing endfile character as well in searchfiles
            {
                searchfiles.push_back(text);
            }
        }

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

        numberownfiles = files.size();
        //Print files
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

    void doClientStuff()
    {
        for(int i=0;i<numberneighbours;i++)
        {
            int sock = 0, valread;
            struct sockaddr_in serv_addr;
            char buffer[1024] = {0};
            if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
            {
                printf("\n Socket creation error \n");
                return;
            }
        
            serv_addr.sin_family = AF_INET;
            serv_addr.sin_port = htons(neighbourport[i]);
            //cout<<neighbourport[0];
            
            // Convert IPv4 and IPv6 addresses from text to binary form
            if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0) 
            {
                printf("\nInvalid address/ Address not supported \n");
                return;
            }
            while(1){
            if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
            {
                // printf("\nConnection Failed \n");
                // return;
            }
            else
            break;
            }
            // send(sock , hello , strlen(hello) , 0 );
            // printf("Hello message sent\n");
            valread = 0;
            while(valread==0){
                valread = read( sock , buffer, 1024);
            }
            
            //cout<<valread<<endl;
            cout<<"Connected to "<<neighbourid[i]<<" with unique-ID ";
            printf("%s ",buffer );
            cout<<"on port "<<neighbourport[i]<<endl;
        }
    }
};


class ServerSide
{
    int id;
    int port;
    int uniqueid;
    int numberneighbours;
    char *ip = "127.0.0.1";
    std::vector<int> neighbourid;
    std::vector<int> neighbourport;
    std::vector<int> neighbouruniqueid;
    std::vector<std::string> searchfiles;
    int numbersearchfiles;
    std::vector<std::string> ownfiles;
    int numberownfiles;
    public:
    bool received;
    int server_fd, valread;
    int new_socket[10];
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
                numbersearchfiles = std::stoi(text); 
            }
            else if(linecount<= 4 + numbersearchfiles) //change:: added this else if as it was storing endfile character as well in searchfiles
            {
                searchfiles.push_back(text);
            }
        }

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



    void doServerStuff()
    {
        char buffer[1024] = {0};
        const char *hello = to_string(uniqueid).c_str();
        // Creating socket file descriptor
        if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
        {
            perror("socket failed");
            exit(EXIT_FAILURE);
        }
        
        // Forcefully attaching socket to the port 8080
        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                                                    &opt, sizeof(opt)))
        {
            perror("setsockopt");
            exit(EXIT_FAILURE);
        }
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons( port );
        
        // Forcefully attaching socket to the port 8080
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
            // valread = read( new_socket , buffer, 1024);
            // printf("%s\n",buffer );

            char tmp[256];
            getcwd(tmp, 256);

            // send(new_socket , tmp , strlen(hello) , 0 );
            send(new_socket[counter] , hello , strlen(hello) , 0 );
            counter++;
            // printf("Hello message sent\n");
        }
        return;
    }
};



void foo(char config[100], char pathtofiles[100])
{
    ClientSide c(config, pathtofiles);
    c.doClientStuff();
}

void foo2(char config[100], char pathtofiles[100])
{
    ServerSide c(config, pathtofiles);
    c.doServerStuff();
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

    while(true)
    {

    }

}