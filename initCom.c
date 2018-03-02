#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


#include <pthread.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <string.h>

#include <fcntl.h>   /* File control definitions */
#include <errno.h>   /* Error number definitions */
#include <termios.h> /* POSIX terminal control definitions */

#include "initStack.h"

pthread_cond_t condition = PTHREAD_COND_INITIALIZER; /* Création de la condition */
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

bool initSocket (Tst_conf_com *initCan_socket, int *sock) {

   if(initCan_socket -> protocole == Ce_SOCKET_CAN )
   {
	printf("\n +----------------------------------+");
	printf("\n |        CAN Bus Read              |");
	printf("\n +----------------------------------+\n");
   }

	struct sockaddr_can addr;
	struct ifreq ifr;

	if ((*sock = socket(PF_CAN, SOCK_RAW, initCan_socket -> socket_protocole)) < 0) {
		perror("Error while opening socket");
		return ERROR_OPENING_SOCKET;
	}

	strcpy (ifr.ifr_name, initCan_socket-> can_device_name);
	ioctl (*sock, SIOCGIFINDEX, &ifr);

	printf("\t Using interface name '%s'.\n", ifr.ifr_name);


	addr.can_family = AF_CAN;
	addr.can_ifindex = ifr.ifr_ifindex;

	if (bind(*sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror ("Error in socket bind");
		return ERROR_SOCKET_BIND;
	}

	pthread_mutex_lock (&mutex); /* On verrouille le mutex */
	pthread_cond_signal (&condition); /* On délivre le signal : condition remplie */
	pthread_mutex_unlock (&mutex); /* On déverrouille le mutex */


}



TS32 openSerial_port (Tst_conf_com *initSerial)
{

	int serialPort = 0;
	struct termios options;
	int speed = 0;

   if(initSerial -> protocole == Ce_GPS )
   {
	printf("\n +----------------------------------+");
	printf("\n |        Serial Port Read          |");
	printf("\n +----------------------------------+\n");
   }

	serialPort = open(initSerial-> serial_device_name,O_RDWR | O_NOCTTY );
	if(serialPort == -1)
	{
		printf("error ! in opening %s ", initSerial->serial_device_name);
		return ERROR_OPENING_SERIALPORT;
	}

	else
	{
		printf("\t Using serial port '%s' at %d bps.\n", initSerial->serial_device_name, initSerial->baudrate);
		tcgetattr(serialPort,&options);

		cfsetispeed(&options, B115200);
		cfsetospeed(&options, B115200);

		options.c_cflag |= (CLOCAL | CREAD); //No parity 8N1
		options.c_cflag |=~CRTSCTS;
		options.c_cflag &= ~(IXON | IXOFF | IXANY);
		options.c_iflag &= ~(ICANON | ECHO | ECHOE | ISIG);
		options.c_oflag &= ~OPOST;
		options.c_cflag &= ~PARENB; /* Disables the Parity Enable bit(PARENB),So No Parity   */
		options.c_cflag &= ~CSTOPB; /* CSTOPB = 2 Stop bits,here it is cleared so 1 Stop bit */
		options.c_cflag &= ~CSIZE;
		options.c_cflag |= CS8;


		options.c_cc[VMIN] = 200;
		options.c_cc[VTIME] = 0;

	if ((tcsetattr(serialPort,TCSANOW,&options)) != 0)
		printf("\n ERROR ! in setting attributes");
	else

//#ifdef DEBUG
		printf(" BaudRate = %d \n StopBits = 1 \n Parity = none\n\n", initSerial->baudrate);
//#endif
		tcflush(serialPort, TCIFLUSH);

	}



	return serialPort;

}

