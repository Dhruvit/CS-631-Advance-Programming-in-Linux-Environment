/* Simple Web Server
 * Team : Threedot
 * CS 631 APUE : Final Project
 */

//Main File
//Headers
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "main.h"
#include <errno.h>
#include <bsd/stdlib.h>
#include <dirent.h>
#include <limits.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <stdbool.h>

char *homeDirectory;
char *exCgiDirectory;
int debugMode;
char *ipAddress;
char *logFile;
int port;
int cgiEnable;

/* Validate User Given IP format */
int validate_ip ( char * ipaddr )
{
    uint32_t ipv4;
    uint8_t ipv6[16];
    int type;

    type = inet_pton ( AF_INET, ipaddr, &ipv4 );

    if ( type == 1 ) 
    {
        return 1;
    }
    else
    {
        type = inet_pton ( AF_INET6, ipaddr, ipv6 ); 
        if ( type == 1 )         
        {
            return 1;
        }
    }
    return -1;
}

/* Validate User Given Port Range */
int validate_port_range(int port)
{
    if(port > 0 && port <= 65536)
    {
        return 1;
    }
      else
    {
        return 0;
    }
}

//Check Directory
bool is_dir(const char *file)
{
    struct stat s;
    if (lstat(file, &s) == -1) {
        return false;
    }
    return S_ISDIR(s.st_mode);
}

//Get Summary
void Summary()
{
	printf("Simple web server, binds to a given port number on the given IPv4 or IPv6 address, process incoming HTTP/1.0 requests.\n");
}

//Main Starts
int main( int argc, char **argv){

    setprogname((char *) argv[0]);
    int ch;

    while( ( ch = getopt( argc, argv, "c:dhi:l:p:") ) != -1 ){
        switch( ch ){
            case 'c':
		            exCgiDirectory = optarg;
                    if(!is_dir(exCgiDirectory))
                    {
                        fprintf(stderr, "Directory: %s does not exist\n", optarg);
        		        exit(EXIT_FAILURE);
   		            }
                    cgiEnable = 1;
                    break;
            case 'd':
                    debugMode = 1;
                    break;
            case 'h':
		             Summary();
		             exit(EXIT_SUCCESS);
                     break;
            case 'i': if(validate_ip(optarg) == -1)
                      {
                        fprintf(stderr,"Invalid IP Format\n");
                        exit(EXIT_FAILURE);
                      }
                      else
                      {
                        ipAddress = optarg;
                      }
                      break;
            case 'l':
                    logFile = optarg;
                    break;
            case 'p':
                    port = atoi(optarg);
                    if(validate_port_range(port))
                    {
                        port = atoi(optarg);
                    } else {
                        fprintf(stderr,"Invalid Port Range.\n");
                        exit(EXIT_FAILURE);
                    }
                    break;
            default:
                    fprintf(stderr, "Usage: %s [-dh] -c[dir] -i[address] -l[file] -p[port] dir\n", getprogname());
                    exit(EXIT_FAILURE);
        }
    }

    //if(realpath(argv[argc-1], homeDirectory) != NULL){
	//printf("root : %s",homeDirectory);

    if ((argc - optind) != 1)
    {
        fprintf(stderr,"Root directory not specified\n");
        fprintf(stderr, "Usage: %s [-dh] -c[dir] -i[address] -l[file] -p[port] dir\n", getprogname());
        exit(EXIT_FAILURE);
    }
    else
    {
        homeDirectory = argv[argc-1];
        if(!is_dir(homeDirectory))
        {
            fprintf(stderr, "%s does not exist\n",homeDirectory);
            exit(EXIT_FAILURE);
        }
    }
    //}
    Listen();
    return EXIT_SUCCESS;
}
//Main Ends
