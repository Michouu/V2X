
#include <linux/can.h>
#include <linux/can/raw.h>


#include "initStack.h"

#define MAX 4095

bool socketcan_read (Tst_j1939Buf *j1939Param, int sock)
{

	struct can_frame frame;
	int nbytes = 0;
	int i      = 0 ;

	nbytes = read(sock, &frame, sizeof(struct can_frame));
	j1939Param->id = frame.can_id;

	for (i = 0 ; i < 3; i++)
	{
		j1939Param->data[i] = frame.data[i];
	}

	return true;

}


unsigned char *serialPort_read (int serialPort)
{

	STRING read_buf = NULL;
	int bytes_read = 0;
	int i = 0;

	read_buf = malloc(MAX * sizeof(unsigned char));
	bytes_read = read(serialPort, read_buf, sizeof(read_buf));

	printf("\n\n Bytes read -%d\n", bytes_read);

	for (i = 0; i< 4095; i++)
	{
		printf("%c", read_buf[i]);
	}

	return read_buf;
}
