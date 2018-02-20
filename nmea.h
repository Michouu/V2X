

#ifndef NMEA_H
#define NMEA_H



#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <time.h>
#include <math.h>




#define MINMEA_MAX_LENGTH 90

 /* Define different INT (8, 16, 35 or 64) */
typedef unsigned short      TU16;
typedef int                 TS32;
typedef unsigned int        TU32;

/* Define different FLOAT (32 or 64) */
typedef float               FLOAT32;

/* Define CHAR Type */
typedef char                CHAR; //nota: c_xx

enum minmea_sentence_id {
	MINMEA_INVALID = -1,
	MINMEA_UNKNOWN = 0,
	MINMEA_SENTENCE_RMC,
	MINMEA_SENTENCE_GGA,
	MINMEA_SENTENCE_GSA,
	MINMEA_SENTENCE_GLL,
	MINMEA_SENTENCE_GST,
	MINMEA_SENTENCE_GSV,
	MINMEA_SENTENCE_VTG,
	MINMEA_SENTENCE_ZDA,
};

struct minmea_float {
	int_least32_t value;
	int_least32_t scale;
};

struct minmea_date {
	TS32 day;
	TS32 month;
	TS32 year;
};

struct minmea_time {
	TS32 hours;
	TS32 minutes;
	TS32 seconds;
	TS32 mseconds;
};

struct minmea_sentence_rmc {
	struct minmea_time time;
	bool valid;
	struct minmea_float latitude;
	struct minmea_float longitude;
	struct minmea_float speed;
	struct minmea_float course;
	struct minmea_date date;
	struct minmea_float variation;
};

struct minmea_sentence_gga {
	struct minmea_time time;
	struct minmea_float latitude;
	struct minmea_float longitude;
	char latitude_direction;
	char longitude_direction;
	TS32 fix_quality;
	TS32 satellites_tracked;
	struct minmea_float hdop;
	struct minmea_float altitude;
	CHAR altitude_units;
	struct minmea_float height;
	CHAR height_units;
	TS32 dgps_age;
};

// FAA mode added to some fields in NMEA 2.3.
enum minmea_faa_mode {
	MINMEA_FAA_MODE_AUTONOMOUS = 'A',
	MINMEA_FAA_MODE_DIFFERENTIAL = 'D',
	MINMEA_FAA_MODE_ESTIMATED = 'E',
	MINMEA_FAA_MODE_MANUAL = 'M',
	MINMEA_FAA_MODE_SIMULATED = 'S',
	MINMEA_FAA_MODE_NOT_VALID = 'N',
	MINMEA_FAA_MODE_PRECISE = 'P',
};

struct minmea_sentence_gll {
	struct minmea_float latitude;
	struct minmea_float longitude;
	struct minmea_time time;
	CHAR status;
	CHAR mode;
};

struct minmea_sentence_gst {
	struct minmea_time time;
	struct minmea_float rms_deviation;
	struct minmea_float semi_major_deviation;
	struct minmea_float semi_minor_deviation;
	struct minmea_float semi_major_orientation;
	struct minmea_float latitude_error_deviation;
	struct minmea_float longitude_error_deviation;
	struct minmea_float altitude_error_deviation;
};


struct minmea_sat_info {
	TS32 nr;
	TS32 elevation;
	TS32 azimuth;
	TS32 snr;
};

struct minmea_sentence_gsv {
	TS32 total_msgs;
	TS32 msg_nr;
	TS32 total_sats;
	struct minmea_sat_info sats[4];
};

struct minmea_sentence_vtg {
	struct minmea_float true_track_degrees;
	struct minmea_float magnetic_track_degrees;
	struct minmea_float speed_knots;
	struct minmea_float speed_kph;
	enum minmea_faa_mode faa_mode;
};


/**
 * Check sentence validity and checksum. Returns true for valid sentences.
 */
 bool minmea_check(const char *sentence, bool strict);

/**
 * Determine talker identifier.
 */
 bool minmea_talker_id(char talker[3], const char *sentence);

/**
 * Determine sentence identifier.
 */
 enum minmea_sentence_id minmea_sentence_id(const char *sentence, bool strict);

/**
 * Scanf-like processor for NMEA sentences. Supports the following formats:
 * c - single character (char *)
 * d - direction, returned as 1/-1, default 0 (int *)
 * f - fractional, returned as value + scale (int *, int *)
 * i - decimal, default zero (int *)
 * s - string (char *)
 * t - talker identifier and type (char *)
 * T - date/time stamp (int *, int *, int *)
 * Returns true on success. See library source code for details.
 */
 bool minmea_scan(const char *sentence, char *direction_lat ,char *direction_long,const char *format,...);

/*
 * Parse a specific type of sentence. Return true on success.
 */
 bool minmea_parse_rmc(struct minmea_sentence_rmc *frame, const char *sentence);
 bool minmea_parse_gga(struct minmea_sentence_gga *frame, const char *sentence);
 bool minmea_parse_gll(struct minmea_sentence_gll *frame, const char *sentence);
 bool minmea_parse_gst(struct minmea_sentence_gst *frame, const char *sentence);
 bool minmea_parse_vtg(struct minmea_sentence_vtg *frame, const char *sentence);




/**
 * Rescale a fixed-point value to a different scale. Rounds towards zero.
 */
 static inline int_least32_t minmea_rescale(struct minmea_float *f, int_least32_t new_scale)
 {
 	if (f->scale == 0)
 		return 0;
 	if (f->scale == new_scale)
 		return f->value;
 	if (f->scale > new_scale)
 		return (f->value + ((f->value > 0) - (f->value < 0)) * f->scale/new_scale/2) / (f->scale/new_scale);
 	else
 		return f->value * (new_scale/f->scale);
 }

/**
 * Convert a fixed-point value to a floating-point value.
 * Returns NaN for "unknown" values.
 */
 static inline float minmea_tofloat(struct minmea_float *f)
 {
 	if (f->scale == 0)
 		return NAN;
 	return ((float) f->value) / ((float) f->scale);
 }

/**
 * Convert a raw coordinate to a floating point DD.DDD... value.
 * Returns NaN for "unknown" values.
 */
 static inline float minmea_tocoord(struct minmea_float *f)
 {
 	if (f->scale == 0)
 		return NAN;
 	int_least32_t degrees = f->value / (f->scale * 100);
 	int_least32_t minutes = f->value % (f->scale * 100);
 	return (float) degrees + (float) minutes / (60 * f->scale);
 }


#endif /* MINMEA_H */

/* vim: set ts=4 sw=4 et: */
