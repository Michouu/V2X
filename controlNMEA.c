#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>


#include "nmea.h"
#include "initStack.h"

#define TOTALTIME_SECONDE (st_nmea_field -> Time.hours + st_nmea_field -> Time.minutes + st_nmea_field -> Time.seconds + st_nmea_field -> Time.mseconds)
#define CONVERT_ms_TO_us 1000


void msecond_conversion (Tst_NMEA_field *nmea,struct minmea_sentence_gga *frame )
{
	nmea -> Time.hours     =  frame->time.hours * 3600000; // hours --> seconds
	nmea -> Time.minutes   =  frame->time.minutes * 60000; // minutes --> seconds
	nmea -> Time.seconds   =  frame->time.seconds * 1000;
	nmea -> Time.mseconds  =  frame->time.mseconds ;
}


bool controlNMEA (unsigned char *line,Tst_NMEA_field *st_nmea_field, Tst_conf_com *communication)
{
	int elapsedTime_temp = 0;
	int diffTime = 0;
	static int nbrLignes = 0;

	static int cpt = 0;
	static int testTime = 0;


#ifdef DEBUG
	printf("%s\n", line);
#endif

	switch (minmea_sentence_id(line, false))
	{
		case MINMEA_SENTENCE_RMC: {
			struct minmea_sentence_rmc frame;
			if (minmea_parse_rmc(&frame, line))
			{
			}
			else {
				printf(INDENT_SPACES "$xxRMC sentence is not parsed\n");
			}
		} break;

#ifdef DEBUG
		printf(INDENT_SPACES" RMC frames available \n");
#endif

		case MINMEA_SENTENCE_GGA: {
			struct minmea_sentence_gga frame;
			if (minmea_parse_gga(&frame, line))
			{
				msecond_conversion(st_nmea_field, &frame);
				elapsedTime_temp = TOTALTIME_SECONDE;
				cpt ++;

				if (cpt > 1)
				{
					diffTime = elapsedTime_temp - st_nmea_field ->Time.elapsedTime_sec;
				}
				st_nmea_field ->Time.elapsedTime_sec = TOTALTIME_SECONDE;

				if (st_nmea_field ->Time.elapsedTime_sec != testTime)
				{
					usleep(diffTime*CONVERT_ms_TO_us);
					st_nmea_field -> latitude = minmea_tocoord(&frame.latitude);
					st_nmea_field -> longitude = minmea_tocoord(&frame.longitude);
					st_nmea_field -> heading = minmea_tofloat(&frame.altitude);
					st_nmea_field -> latitude_direction = frame.latitude_direction;
					st_nmea_field -> longitude_direction = frame.longitude_direction;
					st_nmea_field -> fix_quality = frame.fix_quality;
					st_nmea_field -> satellites_tracked = frame.satellites_tracked;
				}
				testTime = st_nmea_field ->Time.elapsedTime_sec;
			}
			else {
				printf(INDENT_SPACES "$xxGGA sentence is not parsed\n");
			}

		} break;

#ifdef DEBUG
		printf(INDENT_SPACES" GGA frames available \n");
		printf("%d:%d:%d:%d\n", st_nmea_field -> Time.hours, st_nmea_field -> Time.minutes,
			st_nmea_field -> Time.seconds, (st_nmea_field -> Time.mseconds ) );
#endif

		case MINMEA_SENTENCE_VTG: {
			struct minmea_sentence_vtg frame;
			if (minmea_parse_vtg(&frame, line))
			{
				st_nmea_field -> degree_cap = minmea_tofloat(&frame.true_track_degrees);
				st_nmea_field -> vitesse = minmea_tofloat(&frame.speed_kph);
			}

			else {
				printf(INDENT_SPACES "$xxVTG sentence is not parsed\n");
			}
		} break;

		case MINMEA_INVALID: {
			printf(INDENT_SPACES "$xxxxx sentence is not valid\n");
		} break;

#ifdef DEBUG
		printf(INDENT_SPACES" VTG frames available \n");
#endif

		default: {
			printf(INDENT_SPACES "$xxxxx sentence is not parsed\n");
		} break;


	}
	return true;

}

