

#include "nmea.h"
#include "initStack.h"


bool gps_parse(Tst_conf_com *communication, Tst_NMEA_field *nmea_field)
{

	STRING line = NULL;
	communication -> nbrLignes = 0;

	TS32 currentLine =0 ;
	TS32 readingFile = 0;
	TS32 openSerialPort = 0;


	if (communication->serial_device_name != NULL)   // serial port selected
	{
		openSerialPort = openSerial_port(communication);  // opening the serial port
		while(1)
		{
			line = serialPort_read(openSerialPort);  // reading on the serial port
			controlNMEA(line,nmea_field, communication); // reading NMEA frames

		}
		close(openSerialPort);
		free(line);
	}

	else
	{
		line = malloc (MINMEA_MAX_LENGTH * sizeof (unsigned char));
		if (communication-> NMEA_file != NULL)
		{

#ifdef DEBUG
			while (fgets(line, MINMEA_MAX_LENGTH, communication-> NMEA_file) != NULL) // get number of ligne
			{
				communication -> nbrLignes ++;
			}
			rewind(communication-> NMEA_file); // return to the beginning of the file
#endif

			while (fgets(line, MINMEA_MAX_LENGTH, communication-> NMEA_file) != NULL)
			{
#ifdef DEBUG
				readingFile = (100 * currentLine) / communication->nbrLignes;
				system("clear");
				printf("File being played : [%d/100 Pourcent]\n", readingFile);
#endif
				controlNMEA(line,nmea_field, communication);
#ifdef DEBUG
				system("clear");
				currentLine ++;
#endif
			}

		}

		else
		{
			printf("Impossible to open the file\n");
			exit(ERROR_WHILE_OPENING_FILE);
		}
		fclose(communication-> NMEA_file);
	}



	return true;
}

