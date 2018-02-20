#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>


#define RECURRENCE 10

#define INDENT_SPACES "  "

#define DISPLAY() 						printf("\n +----------------------------------+"); \
									printf("\n |             FILE Read            |"); \
									printf("\n +----------------------------------+\n"); \
									printf("\n*********************** Current Position ***************************\n\n"); \
									printf("* Latitude           : %f[%c] deg  (degres decimaux)\n", nmea_field.latitude, nmea_field.latitude_direction); \
									printf("* Longitude          : %f[%c] deg  (degres decimaux)\n", nmea_field.longitude, nmea_field.longitude_direction); \
									printf("* Fix_qualification  : %d  (0 = no valid, 1 = Fix GPS, 2 = Fix DGPS)\n", nmea_field.fix_quality); \
									printf("* Satellites_tracked : %d                           \n", nmea_field.satellites_tracked); \
									printf("* Altitude           : %f  (above MSL)              \n", nmea_field.heading); \
									printf("* Speed              : %f [Km/h]                    \n", nmea_field.vitesse); \
									printf("* Real cap           : %f deg                       \n\n", nmea_field.degree_cap); \
									printf("********************************************************************\n");


#include <stdio.h>



/* Define different INT (8, 16, 35 or 64) */
typedef unsigned short			TU16;
typedef int 					TS32;
typedef unsigned int 			TU32;

/* Define different FLOAT (32 or 64) */
typedef float 					FLOAT32;

/* Define CHAR Type */
typedef char 					CHAR; //nota: c_xx

typedef unsigned char*			STRING;

typedef enum {
	Ce_SOCKET_CAN,
	Ce_GPS,
	Ce_DUAL 			// GPS + CAN
}Te_Interface;

enum sentence{
	ID_INVALID,
	PGN_SPEED_VEHICLE
};

enum e_error{
	NO_ERROR = 0,
	ERROR_OPENING_SOCKET = -1,
	ERROR_SOCKET_BIND = -2,
	ERROR_INTERFACE_NAME = -3,
	ERROR_OPENING_SERIALPORT = -4,
	ERROR_WHILE_OPENING_FILE = -5
};

typedef struct{
	STRING		c_device_name;
	FILE 		*NMEA_file;
	TS32			flag_file;
	TS32			flag_in;
	TS32 		baudrate;
	Te_Interface 	protocole;
	TS32 		socket_protocole;
	TS32			nbrLignes;
}Tst_conf_com;

typedef struct{
	TS32 	id;
	TS32 	Pgn;
	TU32 	data [8];
	TS32 	vitesse;
}Tst_j1939Buf;

typedef struct {
	TS32 	 hours;
	TS32 	 minutes;
	TS32 	 seconds;
	TS32 mseconds;
	TS32 elapsedTime_sec;
}Tst_minmea_time;

// Structure will be send to V2X Stack
typedef struct{
	Tst_minmea_time Time ;
	TS32 		 diffTime;
	FLOAT32 		 vitesse;
	FLOAT32 		 longitude;
	FLOAT32 		 latitude;
	CHAR  		 latitude_direction;
	CHAR  		 longitude_direction;
	FLOAT32 		 heading;
	FLOAT32 		 degree_cap;
	TS32  		 fix_quality;
	TS32  		 satellites_tracked;
}Tst_NMEA_field;


// Information from CAN Bus
bool socketcan_read (Tst_j1939Buf *j1939Param, int sock);
bool extractVitesse (Tst_j1939Buf *extracting_param);
bool initSocket (Tst_conf_com *communication, int *sock);
enum sentence sentence_id  (int Id, int Pgn);


// Information from GPS
bool gps_parse(Tst_conf_com *communication, Tst_NMEA_field *nmea_field);
TS32 openSerial_port (Tst_conf_com *initSerial);
unsigned char *serialPort_read (int serialPort);
bool controlNMEA (unsigned char *line,Tst_NMEA_field *st_nmea_field, Tst_conf_com *communication);


