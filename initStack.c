
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
	fprintf(stderr, "          -b <baudrate> (115200 bps)\n");
	fprintf(stderr, "          -f <NMEA file.txt> \n");
	fprintf(stderr, "\n");
	fprintf(stderr, "utilisation exemple : \n");
	fprintf(stderr, "%s -p S -d can0 \n", prg);
	fprintf(stderr, "%s -p G -d /dev/ttyUSB -b 115200 (baudrate) \n", prg);
	fprintf(stderr, "%s -p D -d can0 -d dev/ttyUSB0 -b 115200  \n", prg);

	fprintf(stderr, "%s -p G -f file.txt\n", prg);
}

void *thread_read_Can(void *arg)
{
	TS32 readSocket = 0;
	Tst_conf_com *pstCom = (Tst_conf_com*)arg;
	Tst_j1939Buf stParam_J1939;

	if ( initSocket(pstCom, &readSocket) == NO_ERROR)  //OPEN socket can
	{
		while (1)
		{
			if ((strncmp (pstCom->can_device_name, "can", 3)) != 0 || (strlen (pstCom->can_device_name)) != 4)
			{
				printf("Not an Interface Can \n");
				exit (ERROR_INTERFACE_NAME);
			}
			else
			{
			socketcan_read(&stParam_J1939, readSocket);  //reading on socket CAN
			extractVitesse (&stParam_J1939); // extracting PGN (Speed 0xFEF1)

			}

		}
		close (readSocket);
	}
	(void) arg;
}

void *thread_read_GPS(void *arg)
{
	Tst_conf_com *pstCom = (Tst_conf_com*)arg;
	Tst_NMEA_field nmea_field;

	gps_parse(pstCom, &nmea_field); // GPS option
	DISPLAY()

	(void) arg;
}


int main (int argc, char *argv[])
{

	unsigned char protocole_usage = 0;
	TS32 opt ;
	char *device_name = NULL;

	Tst_conf_com configuration;
	configuration.socket_protocole = CAN_RAW;

	while ((opt = getopt (argc, argv, "p:d:z:f:b:h")) != -1)  //parsing command line
	{

		switch (opt)
		{
			case 'p':
			protocole_usage = optarg[0];

				if (protocole_usage == 'S')
				{
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
			device_name = optarg;

				if (strncmp (device_name, "can",3) == 0)
				{
					configuration.can_device_name = device_name;
				}

				else
				{
					configuration.serial_device_name = device_name;
				}

			break;

			case 'z':
			configuration.socket_protocole = atoi (optarg);
			break;

			case 'f':
			configuration.NMEA_file = fopen (optarg,"r");
			break;

			case 'b':
			configuration.baudrate = atoi(optarg);
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



/*********** THREAD DECLARATION **************/
pthread_t threadRead_CAN;
pthread_t threadRead_GPS;


	switch (configuration.protocole)
	{
		case Ce_SOCKET_CAN:
		pthread_create (&threadRead_CAN, NULL, thread_read_Can, &configuration);
		pthread_join (threadRead_CAN, NULL);
		break;

		case Ce_GPS:
		pthread_create (&threadRead_GPS, NULL, thread_read_GPS, &configuration);
		pthread_join (threadRead_GPS, NULL);
		break;

		case Ce_DUAL:
			/*if (argc < 9)
				print_usage(argv[0]);*/
			//else
			//{
				printf("\n Reading on Serial Port and Can bus \n");
				printf(" Communication parameters : \n");
				pthread_create (&threadRead_CAN, NULL, thread_read_Can, &configuration);
				pthread_create (&threadRead_GPS, NULL, thread_read_GPS, &configuration);
			break;
			//}

		default:
		fprintf(stderr, "Unknown option %c\n", opt);
		return -1;
		break;

	}

	pthread_exit (NULL);

return 0;
}
