
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <linux/can/raw.h>
#include <sys/types.h>
#include <string.h>

#include "initStack.h"


void print_usage (char *prg)
{
	fprintf(stderr, "\nUsage : %s [options]\n", prg);
	fprintf(stderr, " (use CTRL-C to terminate %s) \n\n", prg);
	fprintf(stderr, "Options : -p <type>    protocole : (S)ocketCan/(G)PS/(D)UAL  \n ");
	fprintf(stderr, "         -d <device> (can0:can1...)/(dev/tty*) \n");
	fprintf(stderr, "          -z <socketProtocole> (7 : SRAW)\n");
	fprintf(stderr, "          -f <NMEA file.txt> \n");
	fprintf(stderr, "\n");
	fprintf(stderr, "utilisation exemple : \n");
	fprintf(stderr, "%s -p S -d can0 \n", prg);
	fprintf(stderr, "%s -p G -d /dev/ttyUSB 115200 (baudrate) \n", prg);
	fprintf(stderr, "%s -p G -f file.txt\n", prg);
}

void *thread_read(void *arg)
{

	Tst_conf_com *pstCom = (Tst_conf_com*)arg;
	Tst_j1939Buf stParam_J1939;
	Tst_NMEA_field nmea_field;
	TS32 readSocket = 0;



	switch (pstCom->protocole)
	{
		case Ce_SOCKET_CAN:
			if ( initSocket(pstCom, &readSocket) == NO_ERROR)  //OPEN socket can
			{
				while (1)
				{
					socketcan_read(&stParam_J1939, readSocket);  //reading on socket CAN
					extractVitesse (&stParam_J1939); // extracting PGN (Speed 0xFEF1)
				}

				close (readSocket);
			}
		break;


		case Ce_GPS:
			gps_parse(pstCom, &nmea_field); // GPS option
			DISPLAY()
			break;

		case Ce_DUAL:
		printf("YAAAAAAAAAAA\n");

		break;

	}

		(void) arg;
		pthread_exit (NULL);
}


int main (int argc, char *argv[])
{

	unsigned char protocole_usage = 0;
	TS32 opt ;

	Tst_conf_com configuration;
	configuration.socket_protocole = CAN_RAW;


   while ((opt = getopt (argc, argv, "p:d:z:f:h")) != -1)  //parsing command line
   {
		CHAR *doc = "NULL";
		configuration.flag_in = 0;
		configuration.flag_file=0;

	switch (opt)
	{
		case 'p':
		protocole_usage = optarg[0];

			if (protocole_usage == 'S')
			{
				/*if ((strncmp (argv[4], "can", 3)) != 0 || (strlen (argv[4])) != 4)
				{
				 printf("Not an Interface Can \n");
				 print_usage(argv[0]);
				 exit (ERROR_INTERFACE_NAME);
				}*/
				configuration.protocole = Ce_SOCKET_CAN ;
			}

			else if (protocole_usage == 'G')
			{
				configuration.protocole = Ce_GPS;
			}

			else if( protocole_usage == 'D')  // 'D' for DUAL GPS + CAN
			{
				configuration.protocole = Ce_DUAL;
			}

			else
			{
				fprintf(stderr, " unknown protocole mode '%c' -ignored\n",optarg[0]);
				print_usage(argv[0]);
			}
		break;

		case 'd':
		configuration.c_device_name = optarg;

			if(*configuration.c_device_name != 'c')
			{
				configuration.flag_in = 1;
				configuration.baudrate  = atoi(optarg);
			}
			break;

			case 'z':
			configuration.socket_protocole = atoi (optarg);
			break;

			case 'f':
			doc = optarg ;
			configuration.flag_file = 1;
			configuration.NMEA_file = fopen (doc,"r");
			break;

			case 'h':
			print_usage(argv[0]);
			return -1;
			break;

			default:
			fprintf(stderr, "Unknown option %c\n", opt);
			return -1;
			break;

	}

   }

	if (argc < 2)
	{
		printf("USAGE: %s loop-iterations\n", argv[0]);
		return 1;
	}

	pthread_t threadRead;
	pthread_create (&threadRead, NULL, thread_read, &configuration);
	pthread_join (threadRead, NULL);

	return 0;
}
