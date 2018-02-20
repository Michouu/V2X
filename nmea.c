
#include "nmea.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <time.h>

#define boolstr(s) ((s) ? "true" : "false")

static int hex2int(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    return -1;
}


bool minmea_check(const char *sentence, bool strict)
{
    uint8_t checksum = 0x00;

    // Sequence length is limited.
    if (strlen(sentence) > MINMEA_MAX_LENGTH)
        return false;

    // A valid sentence starts with "$".
    if (*sentence++ != '$')
        return false;

    // The optional checksum is an XOR of all bytes between "$" and "*".
    while (*sentence && *sentence != '*' && isprint((unsigned char) *sentence))
        checksum ^= *sentence++;

    // If checksum is present...
    if (*sentence == '*') {
        // Extract checksum.
        sentence++;
        int upper = hex2int(*sentence++);
        if (upper == -1)
            return false;
        int lower = hex2int(*sentence++);
        if (lower == -1)
            return false;
        int expected = upper << 4 | lower;

        // Check for checksum mismatch.
        if (checksum != expected)
            return false;
    } else if (strict) {
        // Discard non-checksummed frames in strict mode.
        return false;
    }

    // The only stuff allowed at this point is a newline.
    if (*sentence && strcmp(sentence, "\n") && strcmp(sentence, "\r\n"))
        return false;

    return true;
}

static inline bool minmea_isfield(char c) {
    return isprint((unsigned char) c) && c != ',' && c != '*';
}

bool minmea_scan(const char *sentence, char *direction_lat ,char *direction_long,const char *format,...)
{
    bool result = false;
    bool optional = false;
    va_list ap;
    va_start(ap, format);

    const char *field = sentence;
#define next_field() \
    do { \
        /* Progress to the next field. */ \
        while (minmea_isfield(*sentence)) \
            sentence++; \
        /* Make sure there is a field there. */ \
        if (*sentence == ',') { \
            sentence++; \
            field = sentence; \
        } else { \
            field = NULL; \
        } \
    } while (0)

    while (*format) {
        char type = *format++;

        if (type == ';') {
            // All further fields are optional.
            optional = true;
            continue;
        }

        if (!field && !optional) {
            // Field requested but we ran out if input. Bail out.
            goto parse_error;
        }

        switch (type) {
            case 'c': { // Single character field (char).
                char value = '\0';

                if (field && minmea_isfield(*field))
                    value = *field;

                *va_arg(ap, char *) = value;
            } break;

            case 'd': { // Single character direction field (int).
                int value = 0;

                if (field && minmea_isfield(*field)) {
                    switch (*field) {

                        case 'N':
                        *direction_lat='N';
                        case 'E':
                        value = 1;
                        *direction_long = 'E';
                        break;
                        case 'S':
                        *direction_lat='S';
                        case 'W':
                        value = -1;
                        *direction_long = 'W';
                        break;
                        default:
                        goto parse_error;
                    }
                }

                *va_arg(ap, int *) = value;
            } break;

            case 'f': { // Fractional value with scale (struct minmea_float).
                int sign = 0;
                int_least32_t value = -1;
                int_least32_t scale = 0;

                if (field) {
                    while (minmea_isfield(*field)) {
                        if (*field == '+' && !sign && value == -1) {
                            sign = 1;
                        } else if (*field == '-' && !sign && value == -1) {
                            sign = -1;
                        } else if (isdigit((unsigned char) *field)) {
                            int digit = *field - '0';
                            if (value == -1)
                                value = 0;
                            if (value > (INT_LEAST32_MAX-digit) / 10) {
                                /* we ran out of bits, what do we do? */
                                if (scale) {
                                    /* truncate extra precision */
                                    break;
                                } else {
                                    /* integer overflow. bail out. */
                                    goto parse_error;
                                }
                            }
                            value = (10 * value) + digit;
                            if (scale)
                                scale *= 10;
                        } else if (*field == '.' && scale == 0) {
                            scale = 1;
                        } else if (*field == ' ') {
                            /* Allow spaces at the start of the field. Not NMEA
                             * conformant, but some modules do this. */
                            if (sign != 0 || value != -1 || scale != 0)
                                goto parse_error;
                        } else {
                            goto parse_error;
                        }
                        field++;
                    }
                }

                if ((sign || scale) && value == -1)
                    goto parse_error;

                if (value == -1) {
                    /* No digits were scanned. */
                    value = 0;
                    scale = 0;
                } else if (scale == 0) {
                    /* No decimal point. */
                    scale = 1;
                }
                if (sign)
                    value *= sign;

                *va_arg(ap, struct minmea_float *) = (struct minmea_float) {value, scale};
            } break;

            case 'i': { // Integer value, default 0 (int).
                int value = 0;

                if (field) {
                    char *endptr;
                    value = strtol(field, &endptr, 10);
                    if (minmea_isfield(*endptr))
                        goto parse_error;
                }

                *va_arg(ap, int *) = value;
            } break;

            case 's': { // String value (char *).
                char *buf = va_arg(ap, char *);

                if (field) {
                    while (minmea_isfield(*field))
                        *buf++ = *field++;
                }

                *buf = '\0';
            } break;

            case 't': { // NMEA talker+sentence identifier (char *).
                // This field is always mandatory.
                int f = 0;
                if (!field)
                    goto parse_error;

                if (field[0] != '$')
                    goto parse_error;
                for ( f=0; f<5; f++)
                    if (!minmea_isfield(field[1+f]))
                        goto parse_error;

                    char *buf = va_arg(ap, char *);
                    memcpy(buf, field+1, 5);
                    buf[5] = '\0';
                } break;

            case 'D': { // Date (int, int, int), -1 if empty.
                struct minmea_date *date = va_arg(ap, struct minmea_date *);

                int d = -1, m = -1, y = -1;
                int f = 0;

                if (field && minmea_isfield(*field)) {
                    // Always six digits.
                    for ( f=0; f<6; f++)
                        if (!isdigit((unsigned char) field[f]))
                            goto parse_error;

                        char dArr[] = {field[0], field[1], '\0'};
                        char mArr[] = {field[2], field[3], '\0'};
                        char yArr[] = {field[4], field[5], '\0'};
                        d = strtol(dArr, NULL, 10);
                        m = strtol(mArr, NULL, 10);
                        y = strtol(yArr, NULL, 10);
                    }

                    date->day = d;
                    date->month = m;
                    date->year = y;
                } break;

            case 'T': { // Time (int, int, int, int), -1 if empty.
                struct minmea_time *time_ = va_arg(ap, struct minmea_time *);

                int h = -1, i = -1, s = -1, m = -1;
                int f = 0;

                if (field && minmea_isfield(*field)) {
                    // Minimum required: integer time.
                    for (f=0; f<6; f++)
                        if (!isdigit((unsigned char) field[f]))
                            goto parse_error;

                        char hArr[] = {field[0], field[1], '\0'};
                        char iArr[] = {field[2], field[3], '\0'};
                        char sArr[] = {field[4], field[5], '\0'};
                        h = strtol(hArr, NULL, 10);
                        i = strtol(iArr, NULL, 10);
                        s = strtol(sArr, NULL, 10);
                        field += 6;

                    // Extra: fractional time. Saved as microseconds.
                        if (*field++ == '.') {
                            uint32_t value = 0;
                            uint32_t scale = 1000LU;
                            while (isdigit((unsigned char) *field) && scale > 1) {
                                value = (value * 10) + (*field++ - '0');
                                scale /= 10;
                            }
                            m = value * scale;
                        } else {
                            m = 0;
                        }
                    }

                    time_->hours = h;
                    time_->minutes = i;
                    time_->seconds = s;
                    time_->mseconds = m;
                } break;

            case '_': { // Ignore the field.
            } break;

            default: { // Unknown.
                goto parse_error;
            }
        }

        next_field();
    }

    result = true;

    parse_error:
    va_end(ap);
    return result;
}


enum minmea_sentence_id minmea_sentence_id(const char *sentence, bool strict)
{
    if (!minmea_check(sentence, strict))
        return MINMEA_INVALID;

    char type[6];
    if (!minmea_scan(sentence, NULL,NULL,"t", type))
        return MINMEA_INVALID;

    if (!strcmp(type+2, "RMC"))
        return MINMEA_SENTENCE_RMC;
    if (!strcmp(type+2, "GGA"))
        return MINMEA_SENTENCE_GGA;
    if (!strcmp(type+2, "GSA"))
        return MINMEA_SENTENCE_GSA;
    if (!strcmp(type+2, "GLL"))
        return MINMEA_SENTENCE_GLL;
    if (!strcmp(type+2, "GST"))
        return MINMEA_SENTENCE_GST;
    if (!strcmp(type+2, "GSV"))
        return MINMEA_SENTENCE_GSV;
    if (!strcmp(type+2, "VTG"))
        return MINMEA_SENTENCE_VTG;
    if (!strcmp(type+2, "ZDA"))
        return MINMEA_SENTENCE_ZDA;

    return MINMEA_UNKNOWN;
}

bool minmea_parse_rmc(struct minmea_sentence_rmc *frame, const char *sentence)
{
    // $GPRMC,081836,A,3751.65,S,14507.36,E,000.0,360.0,130998,011.3,E*62
    char type[6];
    char validity;
    int latitude_direction;
    int longitude_direction;
    int variation_direction;
    if (!minmea_scan(sentence, NULL,NULL, "tTcfdfdffDfd",
        type,
        &frame->time,
        &validity,
        &frame->latitude, &latitude_direction,
        &frame->longitude, &longitude_direction,
        &frame->speed,
        &frame->course,
        &frame->date,
        &frame->variation, &variation_direction))
        return false;
    if (strcmp(type+2, "RMC"))
        return false;

    frame->valid = (validity == 'A');
    frame->latitude.value *= latitude_direction;
    frame->longitude.value *= longitude_direction;
    frame->variation.value *= variation_direction;

    return true;
}

bool minmea_parse_gga(struct minmea_sentence_gga *frame, const char *sentence)
{
    // $GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47
    char type[6];
    int latitude_direction;
    int longitude_direction;

    if (!minmea_scan(sentence, &frame->latitude_direction,&frame->longitude_direction, "tTfdfdiiffcfci_",
        type,
        &frame->time,
        &frame->latitude, &latitude_direction,
        &frame->longitude, &longitude_direction,
        &frame->fix_quality,
        &frame->satellites_tracked,
        &frame->hdop,
        &frame->altitude, &frame->altitude_units,
        &frame->height, &frame->height_units,
        &frame->dgps_age))
        return false;
    if (strcmp(type+2, "GGA"))
        return false;

    frame->latitude.value *= latitude_direction;
    frame->longitude.value *= longitude_direction;

    return true;
}

bool minmea_parse_gll(struct minmea_sentence_gll *frame, const char *sentence)
{
    // $GPGLL,3723.2475,N,12158.3416,W,161229.487,A,A*41$;
    char type[6];
    int latitude_direction;
    int longitude_direction;

    if (!minmea_scan(sentence, NULL,NULL, "tfdfdTc;c",
        type,
        &frame->latitude, &latitude_direction,
        &frame->longitude, &longitude_direction,
        &frame->time,
        &frame->status,
        &frame->mode))
    return false;
    if (strcmp(type+2, "GLL"))
        return false;

    frame->latitude.value *= latitude_direction;
    frame->longitude.value *= longitude_direction;

    return true;
}

bool minmea_parse_gst(struct minmea_sentence_gst *frame, const char *sentence)
{
    // $GPGST,024603.00,3.2,6.6,4.7,47.3,5.8,5.6,22.0*58
    char type[6];

    if (!minmea_scan(sentence, NULL,NULL, "tTfffffff",
        type,
        &frame->time,
        &frame->rms_deviation,
        &frame->semi_major_deviation,
        &frame->semi_minor_deviation,
        &frame->semi_major_orientation,
        &frame->latitude_error_deviation,
        &frame->longitude_error_deviation,
        &frame->altitude_error_deviation))
        return false;
    if (strcmp(type+2, "GST"))
        return false;

    return true;
}

bool minmea_parse_vtg(struct minmea_sentence_vtg *frame, const char *sentence)
{
    // $GPVTG,054.7,T,034.4,M,005.5,N,010.2,K*48
    // $GPVTG,156.1,T,140.9,M,0.0,N,0.0,K*41
    // $GPVTG,096.5,T,083.5,M,0.0,N,0.0,K,D*22
    // $GPVTG,188.36,T,,M,0.820,N,1.519,K,A*3F
    char type[6];
    char c_true, c_magnetic, c_knots, c_kph, c_faa_mode;

    if (!minmea_scan(sentence, NULL,NULL, "tfcfcfcfc;c",
        type,
        &frame->true_track_degrees,
        &c_true,
        &frame->magnetic_track_degrees,
        &c_magnetic,
        &frame->speed_knots,
        &c_knots,
        &frame->speed_kph,
        &c_kph,
        &c_faa_mode))
    return false;
    if (strcmp(type+2, "VTG"))
        return false;
    // check chars
    if (c_true != 'T' ||
        c_magnetic != 'M' ||
        c_knots != 'N' ||
        c_kph != 'K')
        return false;
    frame->faa_mode = (enum minmea_faa_mode)c_faa_mode;

    return true;
}

