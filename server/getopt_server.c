#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <sys/types.h>   
#include <libgen.h>
static inline void print_usage(char *progname);

int getopt_server(int argc, char *argv[])
{
    int                       serv_port = 0;
    int                       daemon_run = 0;
    char                     *progname = NULL;
    int                       opt;

    struct option             long_options[] = 
    {   
        {"daemon", no_argument, NULL, 'b'},
        {"port", required_argument, NULL, 'p'},
        {"help", no_argument, NULL, 'h'},
        {NULL, 0, NULL, 0}
    };  

    progname = basename(argv[0]);

    /*  Parser the command line parameters */
    while ((opt = getopt_long(argc, argv, "bp:h", long_options, NULL)) != -1) 
    {   
        switch (opt)
        {   
            case 'b':
                daemon_run=1;
                break;

            case 'p':
                serv_port = atoi(optarg);
                break;

            case 'h':  /*  Get help information */
                print_usage(progname);
                return EXIT_SUCCESS;

            default:
                break;
        }   
    }   

    if( !serv_port ) 
    {   
        print_usage(progname);
        return -1; 
    }


    printf("%s server start to listen on port %d\n", argv[0],serv_port);
    /* set program running on background */
    if( daemon_run ) 
    {
        daemon(0, 0);
    }

	return serv_port;
}


static inline void print_usage(char *progname)
{ 
    printf("Usage: %s [OPTION]...\n", progname);
    
    printf(" %s is a socket server program, which used to verify client and echo back string from it\n", progname); 
    printf("\nMandatory arguments to long options are mandatory for short options too:\n"); 
    
    printf(" -b[daemon ]  set program running on background\n");
    printf(" -p[port    ]  Socket server port address\n");
    printf(" -h[help    ]  Display this help information\n");
    

    printf("\nExample: %s -b -p 8900\n", progname);
    return ;
}

/* test section
int main(int argc, char *argv[])
{
	int			port=0;
	port=getopt_server(argc, argv);
	printf("the port number is:%d\n",port);
	return 0;
}*/
