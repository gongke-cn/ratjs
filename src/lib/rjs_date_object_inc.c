/*****************************************************************************
 *                             Rat Javascript                                *
 *                                                                           *
 * Copyright 2022 Gong Ke                                                    *
 *                                                                           *
 * Permission is hereby granted, free of charge, to any person obtaining a   *
 * copy of this software and associated documentation files (the             *
 * "Software"), to deal in the Software without restriction, including       *
 * without limitation the rights to use, copy, modify, merge, publish,       *
 * distribute, sublicense, and/or sell copies of the Software, and to permit *
 * persons to whom the Software is furnished to do so, subject to the        *
 * following conditions:                                                     *
 *                                                                           *
 * The above copyright notice and this permission notice shall be included   *
 * in all copies or substantial portions of the Software.                    *
 *                                                                           *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS   *
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF                *
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN *
 * NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,  *
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR     *
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE *
 * USE OR OTHER DEALINGS IN THE SOFTWARE.                                    *
 *****************************************************************************/

#include "ratjs_internal.h"

/**Date object.*/
typedef struct {
    RJS_Object object; /**< Base object data.*/
    RJS_Number date;   /**< Date value.*/
} RJS_Date;

#define RJS_MS_PER_DAY          86400000
#define RJS_HOURS_PER_DAY       24
#define RJS_MINUTES_PER_HOUR    60
#define RJS_SECONDS_PER_MINUTE  60
#define RJS_MS_PER_SECOND       1000
#define RJS_MS_PER_MINUTE       (RJS_MS_PER_SECOND * RJS_SECONDS_PER_MINUTE)
#define RJS_MS_PER_HOUR         (RJS_MS_PER_MINUTE * RJS_MINUTES_PER_HOUR)

/*Week day strings.*/
static const char*
week_day_strings[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

/*Month string.*/
static const char*
month_strings[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

/*Days in a month.*/
static int
month_days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

/*Month days from start of the year.*/
static int
month_year_days[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};

/*Scan the reference things in the date object.*/
static void
date_op_gc_scan (RJS_Runtime *rt, void *ptr)
{
    RJS_Date *d = ptr;

    rjs_object_op_gc_scan(rt, &d->object);
}

/*Free the date object.*/
static void
date_op_gc_free (RJS_Runtime *rt, void *ptr)
{
    RJS_Date *d = ptr;

    rjs_object_deinit(rt, &d->object);

    RJS_DEL(rt, d);
}

/*Date object operation functions.*/
static const RJS_ObjectOps
date_ops = {
    {
        RJS_GC_THING_DATE,
        date_op_gc_scan,
        date_op_gc_free
    },
    RJS_ORDINARY_OBJECT_OPS
};

/*Get the day value.*/
static int64_t
day (int64_t t)
{
    return floor(((double)t) / RJS_MS_PER_DAY);
}

/*Get the time within a day.*/
static int
time_within_day (int64_t t)
{
    int v;

    v = t % RJS_MS_PER_DAY;
    if (v < 0)
        v += RJS_MS_PER_DAY;

    return v;
}

/*Get the days in the year.*/
static int
days_in_year (int64_t y)
{
    if (y % 4)
        return 365;
    if (y % 100)
        return 366;
    if (y % 400)
        return 365;
    
    return 366;
}

/*Get day from the year.*/
static int64_t
day_from_year (RJS_Number y)
{
    return (365 * (y - 1970)) + floor((y - 1969) / 4) - floor((y - 1901) / 100) + floor((y - 1601) / 400);
}

/*Get time from the year.*/
static int64_t
time_from_year (int64_t y)
{
    return RJS_MS_PER_DAY * day_from_year(y);
}

/*Get the year from the time value.*/
static int64_t
year_from_time (int64_t t)
{
    int64_t d = day(t);
    int64_t y;

    y = d / 365 + 1970;

    if (d >= 0) {
        while (1) {
            int64_t yt = time_from_year(y);

            if (yt <= t)
                break;

            y --;
        }
    } else {
        y --;
        while (1) {
            int64_t yt = time_from_year(y + 1);

            if (yt > t)
                break;

            y ++;
        }
    }

    return y;
}

/*Get the days within the year.*/
static int
day_within_year (int64_t t)
{
    return day(t) - day_from_year(year_from_time(t));
}

/*Check if the year is a leap year.*/
static int
is_leap_year (int64_t y)
{
    int d = days_in_year(y);

    return (d == 366) ? 1 : 0;
}

/*Check if the time in a leap year.*/
static int
in_leap_year (int64_t t)
{
    return is_leap_year(year_from_time(t));
}

/*Get the week day.*/
static int
week_day (int64_t t)
{
    int v;

    v = (day(t) + 4) % 7;
    if (v < 0)
        v += 7;

    return v;
}

/*Get the month from the time value.*/
static int
month_from_time (int64_t t)
{
    int d    = day_within_year(t);
    int leap = in_leap_year(t);
    int m;

    for (m = 1; m < 12; m ++) {
        int md = month_year_days[m];

        if (m > 1)
            md += leap;

        if (d < md)
            return m - 1;
    }

    return 11;
}

/*Get the date from the time value.*/
static int
date_from_time (int64_t t)
{
    int d     = day_within_year(t);
    int month = month_from_time(t);
    int leap  = in_leap_year(t);
    int r;

    r = d - month_year_days[month] + 1;

    if (month > 1)
        r -= leap;

    return r;
}

/*Get hour from time.*/
static int
hour_from_time (int64_t t)
{
    int v;

    v = (t / RJS_MS_PER_HOUR) % RJS_HOURS_PER_DAY;
    if (v < 0)
        v += RJS_HOURS_PER_DAY;

    return v;
}

/*Get minute from time.*/
static int
min_from_time (int64_t t)
{
    int v;

    v = (t / RJS_MS_PER_MINUTE) % RJS_MINUTES_PER_HOUR;
    if (v < 0)
        v += RJS_MINUTES_PER_HOUR;

    return v;
}

/*Get second from time.*/
static int
sec_from_time (int64_t t)
{
    int v;

    v = (t / RJS_MS_PER_SECOND) % RJS_SECONDS_PER_MINUTE;
    if (v < 0)
        v += RJS_SECONDS_PER_MINUTE;

    return v;
}

/*Get microseconds from time.*/
static int
ms_from_time (int64_t t)
{
    int v;

    v = t % RJS_MS_PER_SECOND;
    if (v < 0)
        v += RJS_MS_PER_SECOND;

    return v;
}

/*Get the date value now.*/
static int64_t
date_value_now (RJS_Runtime *rt)
{
    struct timeval tv;

    gettimeofday(&tv, NULL);

    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

/*Local time zone adjust.*/
static int64_t
local_tza (RJS_Runtime *rt, int64_t t, RJS_Bool utc)
{
    int64_t off = timezone * RJS_MS_PER_SECOND;

    return -off;
}

/*Get the local time.*/
static int64_t
local_time (RJS_Runtime *rt, int64_t t)
{
    return t + local_tza(rt, t, RJS_TRUE);
}

/*Get the UTC time.*/
static int64_t
utc (RJS_Runtime *rt, int64_t t)
{
    return t - local_tza(rt, t, RJS_FALSE);
}

/*Convert the date to string.*/
static RJS_Result
to_date_string (RJS_Runtime *rt, RJS_Number tv, RJS_Value *rv)
{
    RJS_Result r;

    if (isnan(tv)) {
        r = rjs_string_from_chars(rt, rv, "Invalid Date", -1);
    } else {
        RJS_Number t;
        int        wday, mon, mday, year, hour, min, sec;
        int        off, abs_off, off_min, off_hour;
        char       buf[256];

        t = local_time(rt, tv);

        wday = week_day(t);
        mon  = month_from_time(t);
        mday = date_from_time(t);
        year = year_from_time(t);
        hour = hour_from_time(t);
        min  = min_from_time(t);
        sec  = sec_from_time(t);

        off = local_tza(rt, tv, RJS_TRUE);

        abs_off  = RJS_ABS(off);
        off_min  = min_from_time(abs_off);
        off_hour = hour_from_time(abs_off);

        snprintf(buf, sizeof(buf), "%s %s %02d %s%04d %02d:%02d:%02d GMT%s%02d%02d",
                week_day_strings[wday],
                month_strings[mon],
                mday,
                (year < 0) ? "-" : "",
                RJS_ABS(year),
                hour,
                min,
                sec,
                (off < 0) ? "-" : "+",
                off_hour,
                off_min);

        r = rjs_string_from_chars(rt, rv, buf, -1);
    }

    return r;
}

/*Get the time value from the object.*/
static RJS_Result
this_time_value (RJS_Runtime *rt, RJS_Value *o, RJS_Number *pt)
{
    if (rjs_value_get_gc_thing_type(rt, o) == RJS_GC_THING_DATE) {
        RJS_Date *d = (RJS_Date*)rjs_value_get_object(rt, o);

        *pt = d->date;
        return RJS_OK;
    }

    return rjs_throw_type_error(rt, _("the value is not a date"));
}

/*Clip the time value.*/
static RJS_Number
time_clip (RJS_Runtime *rt, RJS_Number n)
{
    size_t     top = rjs_value_stack_save(rt);
    RJS_Value *v   = rjs_value_stack_push(rt);

    if (isinf(n) || isnan(n)) {
        n = NAN;
        goto end;
    }

    if (RJS_ABS(n) > 8.64e15) {
        n = NAN;
        goto end;
    }

    rjs_value_set_number(rt, v, n);
    rjs_to_integer_or_infinity(rt, v, &n);
end:
    rjs_value_stack_restore(rt, top);
    return n;
}

/*Make the time value.*/
static RJS_Number
make_time (RJS_Runtime *rt, RJS_Number h, RJS_Number m, RJS_Number s, RJS_Number ms)
{
    size_t     top = rjs_value_stack_save(rt);
    RJS_Value *v   = rjs_value_stack_push(rt);
    RJS_Number t;

    if (isinf(h) || isnan(h) || isinf(m) || isnan(m)
            || isinf(s) || isnan(s) || isinf(ms) || isnan(ms)) {
        t = NAN;
    } else {
        rjs_value_set_number(rt, v, h);
        rjs_to_integer_or_infinity(rt, v, &h);

        rjs_value_set_number(rt, v, m);
        rjs_to_integer_or_infinity(rt, v, &m);

        rjs_value_set_number(rt, v, s);
        rjs_to_integer_or_infinity(rt, v, &s);

        rjs_value_set_number(rt, v, ms);
        rjs_to_integer_or_infinity(rt, v, &ms);

        t = h * RJS_MS_PER_HOUR + m * RJS_MS_PER_MINUTE + s * RJS_MS_PER_SECOND + ms;
    }

    rjs_value_stack_restore(rt, top);
    return t;
}

/*Make the day value.*/
static RJS_Number
make_day (RJS_Runtime *rt, RJS_Number y, RJS_Number m, RJS_Number d)
{
    size_t     top = rjs_value_stack_save(rt);
    RJS_Value *v   = rjs_value_stack_push(rt);
    RJS_Number dv;

    if (isinf(y) || isnan(y) || isinf(m) || isnan(m) || isinf(d) || isnan(d)) {
        dv = NAN;
    } else {
        int64_t t;
        int     leap, mon, yd;

        rjs_value_set_number(rt, v, y);
        rjs_to_integer_or_infinity(rt, v, &y);

        rjs_value_set_number(rt, v, m);
        rjs_to_integer_or_infinity(rt, v, &m);

        rjs_value_set_number(rt, v, d);
        rjs_to_integer_or_infinity(rt, v, &d);

        y += floor(m / 12);
        if (isinf(y)) {
            dv = NAN;
            goto end;
        }

        mon = ((int64_t)m) % 12;
        if (mon < 0)
            mon += 12;

        t    = time_from_year(y);
        leap = in_leap_year(t);

        yd = month_year_days[mon];

        if (mon > 1)
            yd += leap;

        dv = day(t) + yd + d - 1;
    }
end:
    rjs_value_stack_restore(rt, top);
    return dv;
}

/*Make the date value.*/
static RJS_Number
make_date (RJS_Runtime *rt, RJS_Number d, RJS_Number t)
{
    RJS_Number date;

    if (isinf(d) || isnan(d) || isinf(t) || isnan(t)) {
        date = NAN;
    } else {
        date = d * RJS_MS_PER_DAY + t;

        if (isinf(date))
            date = NAN;
    }

    return date;
}

/*Eatup space.*/
static int
eatup_space (RJS_Runtime *rt, RJS_Input *input)
{
    int c;

    while (1) {
        c = rjs_input_get_uc(rt, input);
        if (!rjs_uchar_is_white_space(c))
            break;
    }

    return c;
}

/*Parse space.*/
static RJS_Result
parse_space (RJS_Runtime *rt, RJS_Input *input)
{
    int c;

    c = rjs_input_get_uc(rt, input);
    if (!rjs_uchar_is_white_space(c))
        return RJS_ERR;

    c = eatup_space(rt, input);
    rjs_input_unget_uc(rt, input, c);
    return RJS_OK;
}

/*Parse the number.*/
static int
parse_date_number (RJS_Runtime *rt, RJS_Input *input, int len)
{
    int c;
    int v    = 0;
    int rlen = 0;

    while (1) {
        c = rjs_input_get_uc(rt, input);
        if (!rjs_uchar_is_digit(c)) {
            rjs_input_unget_uc(rt, input, c);
            break;
        }

        v *= 10;
        v += c - '0';

        rlen ++;

        if ((len > 0) && (rlen >= len))
            break;
    }

    if (rlen == 0)
        return -1;

    return v;
}

/*Parse the year.*/
static RJS_Result
parse_year (RJS_Runtime *rt, RJS_Input *input, int *py)
{
    int y;
    int c;
    int sign = 1;

    c = rjs_input_get_uc(rt, input);
    if (c == '-') {
        sign = -1;

        c = eatup_space(rt, input);
    } else if (c == '+') {
        c = eatup_space(rt, input);
    }

    rjs_input_unget_uc(rt, input, c);

    if ((y = parse_date_number(rt, input, -1)) < 0)
        return RJS_ERR;

    if (sign == -1) {
        if (y == 0)
            return RJS_ERR;

        *py = -y;
    } else {
        *py = y;
    }

    return RJS_OK;
}

/*Parse week day.*/
static RJS_Result
parse_week_day (RJS_Runtime *rt, RJS_Input *input)
{
    char  buf[4];
    char *pc = buf;
    int   i;
    int   c;

    for (i = 0; i < 3; i ++) {
        c = rjs_input_get_uc(rt, input);
        if (!rjs_uchar_is_alpha(c))
            return RJS_ERR;

        *pc ++ = c;
    }

    *pc = 0;

    for (i = 0; i < 7; i ++) {
        if (!strcasecmp(week_day_strings[i], buf))
            return RJS_OK;
    }

    return RJS_ERR;
}

/*Parse month.*/
static int
parse_month (RJS_Runtime *rt, RJS_Input *input)
{
    char  buf[4];
    char *pc = buf;
    int   i;
    int   c;

    for (i = 0; i < 3; i ++) {
        c = rjs_input_get_uc(rt, input);
        if (!rjs_uchar_is_alpha(c))
            return -1;

        *pc ++ = c;
    }

    *pc = 0;

    for (i = 0; i < 12; i ++) {
        if (!strcasecmp(month_strings[i], buf))
            return i + 1;
    }

    return -1;
}

/*Parse hour, minute, second and millisecond.*/
static RJS_Result
parse_hour (RJS_Runtime *rt, RJS_Input *input, int *ph, int *pm, int *ps, int *pms)
{
    int c;
    int hour = 0, min = 0, sec = 0, ms = 0;

    if ((hour = parse_date_number(rt, input, 2)) < 0)
        return RJS_ERR;

    c = rjs_input_get_uc(rt, input);
    if (c == ':') {
        if ((min = parse_date_number(rt, input, 2)) < 0)
            return RJS_ERR;

        c = rjs_input_get_uc(rt, input);
        if (c == ':') {
            if ((sec = parse_date_number(rt, input, 2)) < 0)
                return RJS_ERR;

            c = rjs_input_get_uc(rt, input);
            if (c == '.') {
                if ((ms = parse_date_number(rt, input, 3)) < 0)
                    return RJS_ERR;

                c = rjs_input_get_uc(rt, input);
            }
        }
    }

    rjs_input_unget_uc(rt, input, c);

    *ph  = hour;
    *pm  = min;
    *ps  = sec;
    *pms = ms;

    return RJS_OK;
}

/*Parse the timezone name.*/
static RJS_Result
parse_tz_name (RJS_Runtime *rt, RJS_Input *input)
{
    int c;

    while (1) {
        c = rjs_input_get_uc(rt, input);
        if (c == RJS_INPUT_END)
            return RJS_ERR;
        if (c == ')')
            return RJS_OK;
    }

    return RJS_ERR;
}

/*Parse the date string.*/
static RJS_Result
date_parse (RJS_Runtime *rt, RJS_Value *v, RJS_Number *t)
{
    RJS_Input   input;
    int         c;
    int         y, mon, day, hour, min, sec, ms, off_sign, off_hour, off_min;
    RJS_Number  tv;
    RJS_Bool    is_local = RJS_FALSE;
    RJS_Result  r        = RJS_ERR;

    mon      = 1;
    day      = 1;
    hour     = 0;
    min      = 0;
    sec      = 0;
    ms       = 0;
    off_sign = 1;
    off_hour = 0;
    off_min  = 0;

    rjs_string_input_init(rt, &input, v);

    c = eatup_space(rt, &input);
    if (c == RJS_INPUT_END)
        goto end;

    if (rjs_uchar_is_alpha(c)) {
        RJS_Bool has_space = RJS_FALSE;

        rjs_input_unget_uc(rt, &input, c);

        if (parse_week_day(rt, &input) == RJS_ERR)
            goto end;

        c = rjs_input_get_uc(rt, &input);
        if (rjs_uchar_is_white_space(c)) {
            has_space = RJS_TRUE;

            c = eatup_space(rt, &input);
        }

        if (c == ',') {
            c = eatup_space(rt, &input);
            rjs_input_unget_uc(rt, &input, c);

            if ((day = parse_date_number(rt, &input, 2)) < 0)
                goto end;

            if (parse_space(rt, &input) == RJS_ERR)
                goto end;

            if ((mon = parse_month(rt, &input)) < 0)
                goto end;

            if (parse_space(rt, &input) == RJS_ERR)
                goto end;

            if (parse_year(rt, &input, &y) == RJS_ERR)
                goto end;

            has_space = RJS_FALSE;
            c = rjs_input_get_uc(rt, &input);
            if (rjs_uchar_is_white_space(c)) {
                has_space = RJS_TRUE;
                c = eatup_space(rt, &input);
            }

            if (rjs_uchar_is_digit(c) && has_space) {
                rjs_input_unget_uc(rt, &input, c);

                if (parse_hour(rt, &input, &hour, &min, &sec, &ms) == RJS_ERR)
                    goto end;

                c = eatup_space(rt, &input);
            }

            if ((c == 'g') || (c == 'G')) {
                c = rjs_input_get_uc(rt, &input);
                if ((c != 'm') && (c != 'M'))
                    goto end;
                c = rjs_input_get_uc(rt, &input);
                if ((c != 't') && (c != 'T'))
                    goto end;

                c = eatup_space(rt, &input);
            }
        } else if (has_space) {
            rjs_input_unget_uc(rt, &input, c);

            if ((mon = parse_month(rt, &input)) < 0)
                goto end;

            if (parse_space(rt, &input) == RJS_ERR)
                goto end;

            if ((day = parse_date_number(rt, &input, 2)) < 0)
                goto end;

            if (parse_space(rt, &input) == RJS_ERR)
                goto end;

            if (parse_year(rt, &input, &y) == RJS_ERR)
                goto end;

            has_space = RJS_FALSE;
            c = rjs_input_get_uc(rt, &input);
            if (rjs_uchar_is_white_space(c)) {
                has_space = RJS_TRUE;
                c = eatup_space(rt, &input);
            }

            if (rjs_uchar_is_digit(c) && has_space) {
                rjs_input_unget_uc(rt, &input, c);

                if (parse_hour(rt, &input, &hour, &min, &sec, &ms) == RJS_ERR)
                    goto end;

                c = eatup_space(rt, &input);
            }

            if ((c == 'g') || (c == 'G')) {
                c = rjs_input_get_uc(rt, &input);
                if ((c != 'm') && (c != 'M'))
                    goto end;
                c = rjs_input_get_uc(rt, &input);
                if ((c != 't') && (c != 'T'))
                    goto end;

                c = eatup_space(rt, &input);
            }

            if ((c == '+') || (c == '-')) {
                if (c == '+')
                    off_sign = 1;
                else
                    off_sign = -1;

                if ((off_hour = parse_date_number(rt, &input, 2)) < 0)
                    goto end;

                if ((off_min = parse_date_number(rt, &input, 2)) < 0)
                    goto end;

                c = rjs_input_get_uc(rt, &input);
            }

            if (c == '(') {
                if (parse_tz_name(rt, &input) == RJS_ERR)
                    goto end;
            } else {
                rjs_input_unget_uc(rt, &input, c);
            }
        } else {
            goto end;
        }
    } else {
        RJS_Bool has_time = RJS_FALSE;

        rjs_input_unget_uc(rt, &input, c);

        if (parse_year(rt, &input, &y) == RJS_ERR)
            goto end;

        c = rjs_input_get_uc(rt, &input);
        if (c == '-') {
            if ((mon = parse_date_number(rt, &input, 2)) < 0)
                goto end;

            c = rjs_input_get_uc(rt, &input);
            if (c == '-') {
                if ((day = parse_date_number(rt, &input, 2)) < 0)
                    goto end;

                c = rjs_input_get_uc(rt, &input);
            }
        }

        if (c == 'T') {
            if (parse_hour(rt, &input, &hour, &min, &sec, &ms) == RJS_ERR)
                goto end;

            c = rjs_input_get_uc(rt, &input);

            has_time = RJS_TRUE;
        }

        if (c == 'Z') {
        } else if ((c == '+') || (c == '-')) {
            if (c == '+')
                off_sign = 1;
            else
                off_sign = -1;

            if ((off_hour = parse_date_number(rt, &input, 2)) < 0)
                goto end;

            c = rjs_input_get_uc(rt, &input);
            if (c != ':')
                goto end;

            if ((off_min = parse_date_number(rt, &input, 2)) < 0)
                goto end;
        } else {
            rjs_input_unget_uc(rt, &input, c);

            if (has_time)
                is_local = RJS_TRUE;
        }
    }

    c = eatup_space(rt, &input);
    if (c != RJS_INPUT_END)
        goto end;

    if ((mon < 1) || (mon > 12))
        goto end;
    if (mon == 2) {
        int leap = is_leap_year(y);

        if (day > month_days[mon - 1] + leap)
            goto end;
    } else {
        if (day > month_days[mon - 1])
            goto end;
    }
    if ((hour > 23) || (min > 59) || (sec > 59))
        goto end;
    if ((off_hour > 23) || (off_min > 59))
        goto end;

    tv = make_date(rt, make_day(rt, y, mon - 1, day), make_time(rt, hour, min, sec, ms));

    if (is_local)
        tv = utc(rt, tv);

    if (off_hour || off_min) {
        int off_ms;

        off_ms = off_hour * RJS_MS_PER_HOUR + off_min * RJS_MS_PER_MINUTE;
        if (off_sign == -1)
            off_ms = - off_ms;

        tv -= off_ms;
    }

    tv = time_clip(rt, tv);

    *t = tv;
    r  = RJS_OK;
end:
    if (r == RJS_ERR)
        *t = NAN;

    rjs_input_deinit(rt, &input);
    return RJS_OK;
}

/*Date constuctor.*/
static RJS_NF(Date_constructor)
{
    RJS_Number t;
    RJS_Result r;
    RJS_Date  *date;
    size_t     top   = rjs_value_stack_save(rt);
    RJS_Value *prim  = rjs_value_stack_push(rt);
    RJS_Value *tv    = rjs_value_stack_push(rt);

    if (!nt) {
        t = date_value_now(rt);

        r = to_date_string(rt, t, rv);
        goto end;
    }

    if (argc == 0) {
        t = date_value_now(rt);
    } else if (argc == 1) {
        RJS_Value *arg = rjs_value_buffer_item(rt, args, 0);

        if (rjs_value_get_gc_thing_type(rt, arg) == RJS_GC_THING_DATE) {
            this_time_value(rt, arg, &t);
        } else {
            if ((r = rjs_to_primitive(rt, arg, prim, -1)) == RJS_ERR)
                goto end;

            if (rjs_value_is_string(rt, prim)) {
                if ((r = date_parse(rt, prim, &t)) == RJS_ERR)
                    goto end;
            } else {
                if ((r = rjs_to_number(rt, prim, &t)) == RJS_ERR)
                    goto end;
            }
        }

        t = time_clip(rt, t);
    } else {
        RJS_Number y, m, dt, h, min, s, ms;
        RJS_Value *arg;

        arg = rjs_value_buffer_item(rt, args, 0);
        if ((r = rjs_to_number(rt, arg, &y)) == RJS_ERR)
            goto end;

        arg = rjs_value_buffer_item(rt, args, 1);
        if ((r = rjs_to_number(rt, arg, &m)) == RJS_ERR)
            goto end;

        if (argc > 2) {
            arg = rjs_value_buffer_item(rt, args, 2);
            if ((r = rjs_to_number(rt, arg, &dt)) == RJS_ERR)
                goto end;
        } else {
            dt = 1;
        }

        if (argc > 3) {
            arg = rjs_value_buffer_item(rt, args, 3);
            if ((r = rjs_to_number(rt, arg, &h)) == RJS_ERR)
                goto end;
        } else {
            h = 0;
        }

        if (argc > 4) {
            arg = rjs_value_buffer_item(rt, args, 4);
            if ((r = rjs_to_number(rt, arg, &min)) == RJS_ERR)
                goto end;
        } else {
            min = 0;
        }

        if (argc > 5) {
            arg = rjs_value_buffer_item(rt, args, 5);
            if ((r = rjs_to_number(rt, arg, &s)) == RJS_ERR)
                goto end;
        } else {
            s = 0;
        }

        if (argc > 6) {
            arg = rjs_value_buffer_item(rt, args, 6);
            if ((r = rjs_to_number(rt, arg, &ms)) == RJS_ERR)
                goto end;
        } else {
            ms = 0;
        }

        if (!isnan(y)) {
            rjs_value_set_number(rt, tv, y);
            rjs_to_integer_or_infinity(rt, tv, &y);

            if ((y >= 0) && (y <= 99))
                y += 1900;
        }

        t = make_date(rt, make_day(rt, y, m, dt), make_time(rt, h, min, s, ms));
        t = utc(rt, t);
        t = time_clip(rt, t);
    }

    RJS_NEW(rt, date);

    date->date = t;

    r = rjs_ordinary_init_from_constructor(rt, &date->object, nt, RJS_O_Date_prototype, &date_ops, rv);
    if (r == RJS_ERR)
        RJS_DEL(rt, date);
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

static const RJS_BuiltinFuncDesc
date_constructor_desc = {
    "Date",
    7,
    Date_constructor
};

/*Date.now*/
static RJS_NF(Date_now)
{
    RJS_Number t;

    t = date_value_now(rt);

    rjs_value_set_number(rt, rv, t);
    return RJS_OK;
}

/*Date.parse*/
static RJS_NF(Date_parse)
{
    RJS_Value *arg = rjs_argument_get(rt, args, argc, 0);
    size_t     top = rjs_value_stack_save(rt);
    RJS_Value *str = rjs_value_stack_push(rt);
    RJS_Number t;
    RJS_Result r;

    if ((r = rjs_to_string(rt, arg, str)) == RJS_ERR)
        goto end;

    if ((r = date_parse(rt, str, &t)) == RJS_ERR)
        goto end;

    rjs_value_set_number(rt, rv, t);
    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Date.UTC*/
static RJS_NF(Date_UTC)
{
    RJS_Number y, m, dt, h, min, s, ms, t;
    RJS_Value *arg;
    RJS_Result r;
    size_t     top = rjs_value_stack_save(rt);
    RJS_Value *tv  = rjs_value_stack_push(rt);

    arg = rjs_argument_get(rt, args, argc, 0);
    if ((r = rjs_to_number(rt, arg, &y)) == RJS_ERR)
        goto end;

    if (argc > 1) {
        arg = rjs_value_buffer_item(rt, args, 1);
        if ((r = rjs_to_number(rt, arg, &m)) == RJS_ERR)
            goto end;
    } else {
        m = 0;
    }

    if (argc > 2) {
        arg = rjs_value_buffer_item(rt, args, 2);
        if ((r = rjs_to_number(rt, arg, &dt)) == RJS_ERR)
            goto end;
    } else {
        dt = 1;
    }

    if (argc > 3) {
        arg = rjs_value_buffer_item(rt, args, 3);
        if ((r = rjs_to_number(rt, arg, &h)) == RJS_ERR)
            goto end;
    } else {
        h = 0;
    }

    if (argc > 4) {
        arg = rjs_value_buffer_item(rt, args, 4);
        if ((r = rjs_to_number(rt, arg, &min)) == RJS_ERR)
            goto end;
    } else {
        min = 0;
    }

    if (argc > 5) {
        arg = rjs_value_buffer_item(rt, args, 5);
        if ((r = rjs_to_number(rt, arg, &s)) == RJS_ERR)
            goto end;
    } else {
        s = 0;
    }

    if (argc > 6) {
        arg = rjs_value_buffer_item(rt, args, 6);
        if ((r = rjs_to_number(rt, arg, &ms)) == RJS_ERR)
            goto end;
    } else {
        ms = 0;
    }

    if (!isnan(y)) {
        rjs_value_set_number(rt, tv, y);
        rjs_to_integer_or_infinity(rt, tv, &y);

        if ((y >= 0) && (y <= 99))
            y += 1900;
    }

    t = make_date(rt, make_day(rt, y, m, dt), make_time(rt, h, min, s, ms));
    t = time_clip(rt, t);

    rjs_value_set_number(rt, rv, t);
    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

static const RJS_BuiltinFuncDesc
date_function_descs[] = {
    {
        "now",
        0,
        Date_now
    },
    {
        "parse",
        1,
        Date_parse
    },
    {
        "UTC",
        7,
        Date_UTC
    },
    {NULL}
};

/*Date.prototype.getDate*/
static RJS_NF(Date_prototype_getDate)
{
    RJS_Result r;
    RJS_Number t;

    if ((r = this_time_value(rt, thiz, &t)) == RJS_ERR)
        return r;

    if (!isnan(t)) {
        t = date_from_time(local_time(rt, t));
    }

    rjs_value_set_number(rt, rv, t);
    return RJS_OK;
}

/*Date.prototype.getDay*/
static RJS_NF(Date_prototype_getDay)
{
    RJS_Result r;
    RJS_Number t;

    if ((r = this_time_value(rt, thiz, &t)) == RJS_ERR)
        return r;

    if (!isnan(t)) {
        t = week_day(local_time(rt, t));
    }

    rjs_value_set_number(rt, rv, t);
    return RJS_OK;
}

/*Date.prototype.getFullYear*/
static RJS_NF(Date_prototype_getFullYear)
{
    RJS_Result r;
    RJS_Number t;

    if ((r = this_time_value(rt, thiz, &t)) == RJS_ERR)
        return r;

    if (!isnan(t)) {
        t = year_from_time(local_time(rt, t));
    }

    rjs_value_set_number(rt, rv, t);
    return RJS_OK;
}

/*Date.prototype.getHours*/
static RJS_NF(Date_prototype_getHours)
{
    RJS_Result r;
    RJS_Number t;

    if ((r = this_time_value(rt, thiz, &t)) == RJS_ERR)
        return r;

    if (!isnan(t)) {
        t = hour_from_time(local_time(rt, t));
    }

    rjs_value_set_number(rt, rv, t);
    return RJS_OK;
}

/*Date.prototype.getMilliseconds*/
static RJS_NF(Date_prototype_getMilliseconds)
{
    RJS_Result r;
    RJS_Number t;

    if ((r = this_time_value(rt, thiz, &t)) == RJS_ERR)
        return r;

    if (!isnan(t)) {
        t = ms_from_time(local_time(rt, t));
    }

    rjs_value_set_number(rt, rv, t);
    return RJS_OK;
}

/*Date.prototype.getMinutes*/
static RJS_NF(Date_prototype_getMinutes)
{
    RJS_Result r;
    RJS_Number t;

    if ((r = this_time_value(rt, thiz, &t)) == RJS_ERR)
        return r;

    if (!isnan(t)) {
        t = min_from_time(local_time(rt, t));
    }

    rjs_value_set_number(rt, rv, t);
    return RJS_OK;
}

/*Date.prototype.getMonth*/
static RJS_NF(Date_prototype_getMonth)
{
    RJS_Result r;
    RJS_Number t;

    if ((r = this_time_value(rt, thiz, &t)) == RJS_ERR)
        return r;

    if (!isnan(t)) {
        t = month_from_time(local_time(rt, t));
    }

    rjs_value_set_number(rt, rv, t);
    return RJS_OK;
}

/*Date.prototype.getSeconds*/
static RJS_NF(Date_prototype_getSeconds)
{
    RJS_Result r;
    RJS_Number t;

    if ((r = this_time_value(rt, thiz, &t)) == RJS_ERR)
        return r;

    if (!isnan(t)) {
        t = sec_from_time(local_time(rt, t));
    }

    rjs_value_set_number(rt, rv, t);
    return RJS_OK;
}

/*Date.prototype.getTime*/
static RJS_NF(Date_prototype_getTime)
{
    RJS_Result r;
    RJS_Number t;

    if ((r = this_time_value(rt, thiz, &t)) == RJS_ERR)
        return r;

    rjs_value_set_number(rt, rv, t);
    return RJS_OK;
}

/*Date.prototype.getTimezoneOffset*/
static RJS_NF(Date_prototype_getTimezoneOffset)
{
    RJS_Result r;
    RJS_Number t;

    if ((r = this_time_value(rt, thiz, &t)) == RJS_ERR)
        return r;

    if (!isnan(t)) {
        t = (t - local_time(rt, t)) / RJS_MS_PER_MINUTE;
    }

    rjs_value_set_number(rt, rv, t);
    return RJS_OK;
}

/*Date.prototype.getUTCDate*/
static RJS_NF(Date_prototype_getUTCDate)
{
    RJS_Result r;
    RJS_Number t;

    if ((r = this_time_value(rt, thiz, &t)) == RJS_ERR)
        return r;

    if (!isnan(t)) {
        t = date_from_time(t);
    }

    rjs_value_set_number(rt, rv, t);
    return RJS_OK;
}

/*Date.prototype.getUTCDay*/
static RJS_NF(Date_prototype_getUTCDay)
{
    RJS_Result r;
    RJS_Number t;

    if ((r = this_time_value(rt, thiz, &t)) == RJS_ERR)
        return r;

    if (!isnan(t)) {
        t = week_day(t);
    }

    rjs_value_set_number(rt, rv, t);
    return RJS_OK;
}

/*Date.prototype.getUTCFullYear*/
static RJS_NF(Date_prototype_getUTCFullYear)
{
    RJS_Result r;
    RJS_Number t;

    if ((r = this_time_value(rt, thiz, &t)) == RJS_ERR)
        return r;

    if (!isnan(t)) {
        t = year_from_time(t);
    }

    rjs_value_set_number(rt, rv, t);
    return RJS_OK;
}

/*Date.prototype.getUTCHours*/
static RJS_NF(Date_prototype_getUTCHours)
{
    RJS_Result r;
    RJS_Number t;

    if ((r = this_time_value(rt, thiz, &t)) == RJS_ERR)
        return r;

    if (!isnan(t)) {
        t = hour_from_time(t);
    }

    rjs_value_set_number(rt, rv, t);
    return RJS_OK;
}

/*Date.prototype.getUTCMilliseconds*/
static RJS_NF(Date_prototype_getUTCMilliseconds)
{
    RJS_Result r;
    RJS_Number t;

    if ((r = this_time_value(rt, thiz, &t)) == RJS_ERR)
        return r;

    if (!isnan(t)) {
        t = ms_from_time(t);
    }

    rjs_value_set_number(rt, rv, t);
    return RJS_OK;
}

/*Date.prototype.getUTCMinutes*/
static RJS_NF(Date_prototype_getUTCMinutes)
{
    RJS_Result r;
    RJS_Number t;

    if ((r = this_time_value(rt, thiz, &t)) == RJS_ERR)
        return r;

    if (!isnan(t)) {
        t = min_from_time(t);
    }

    rjs_value_set_number(rt, rv, t);
    return RJS_OK;
}

/*Date.prototype.getUTCMonth*/
static RJS_NF(Date_prototype_getUTCMonth)
{
    RJS_Result r;
    RJS_Number t;

    if ((r = this_time_value(rt, thiz, &t)) == RJS_ERR)
        return r;

    if (!isnan(t)) {
        t = month_from_time(t);
    }

    rjs_value_set_number(rt, rv, t);
    return RJS_OK;
}

/*Date.prototype.getUTCSeconds*/
static RJS_NF(Date_prototype_getUTCSeconds)
{
    RJS_Result r;
    RJS_Number t;

    if ((r = this_time_value(rt, thiz, &t)) == RJS_ERR)
        return r;

    if (!isnan(t)) {
        t = sec_from_time(t);
    }

    rjs_value_set_number(rt, rv, t);
    return RJS_OK;
}

/*Date.prototype.setDate*/
static RJS_NF(Date_prototype_setDate)
{
    RJS_Value *date = rjs_argument_get(rt, args, argc, 0);
    RJS_Date  *d;
    RJS_Result r;
    RJS_Number t, dt, u;

    if ((r = this_time_value(rt, thiz, &t)) == RJS_ERR)
        return r;

    if ((r = rjs_to_number(rt, date, &dt)) == RJS_ERR)
        return r;

    if (isnan(t)) {
        rjs_value_set_number(rt, rv, NAN);
        return RJS_OK;
    }

    t = local_time(rt, t);
    u = make_date(rt, make_day(rt, year_from_time(t), month_from_time(t), dt), time_within_day(t));
    u = time_clip(rt, utc(rt, u));

    d = (RJS_Date*)rjs_value_get_object(rt, thiz);
    d->date = u;

    rjs_value_set_number(rt, rv, u);
    return RJS_OK;
}

/*Date.prototype.setFullYear*/
static RJS_NF(Date_prototype_setFullYear)
{
    RJS_Value *year  = rjs_argument_get(rt, args, argc, 0);
    RJS_Value *month = rjs_argument_get(rt, args, argc, 1);
    RJS_Value *date  = rjs_argument_get(rt, args, argc, 2);
    RJS_Date  *d;
    RJS_Result r;
    RJS_Number t, y, m, dt, u;

    if ((r = this_time_value(rt, thiz, &t)) == RJS_ERR)
        return r;

    if ((r = rjs_to_number(rt, year, &y)) == RJS_ERR)
        return r;

    if (isnan(t)) {
        t = 0;
    } else {
        t = local_time(rt, t);
    }

    if (argc > 1) {
        if ((r = rjs_to_number(rt, month, &m)) == RJS_ERR)
            return r;
    } else {
        m = month_from_time(t);
    }

    if (argc > 2) {
        if ((r = rjs_to_number(rt, date, &dt)) == RJS_ERR)
            return r;
    } else {
        dt = date_from_time(t);
    }

    u = make_date(rt, make_day(rt, y, m, dt), time_within_day(t));
    u = time_clip(rt, utc(rt, u));

    d = (RJS_Date*)rjs_value_get_object(rt, thiz);
    d->date = u;

    rjs_value_set_number(rt, rv, u);
    return RJS_OK;
}

/*Date.prototype.setHours*/
static RJS_NF(Date_prototype_setHours)
{
    RJS_Value *hour = rjs_argument_get(rt, args, argc, 0);
    RJS_Value *min  = rjs_argument_get(rt, args, argc, 1);
    RJS_Value *sec  = rjs_argument_get(rt, args, argc, 2);
    RJS_Value *ms   = rjs_argument_get(rt, args, argc, 3);
    RJS_Date  *d;
    RJS_Result r;
    RJS_Number t, h, m, s, milli, u;

    if ((r = this_time_value(rt, thiz, &t)) == RJS_ERR)
        return r;

    if ((r = rjs_to_number(rt, hour, &h)) == RJS_ERR)
        return r;

    if (argc > 1) {
        if ((r = rjs_to_number(rt, min, &m)) == RJS_ERR)
            return r;
    }

    if (argc > 2) {
        if ((r = rjs_to_number(rt, sec, &s)) == RJS_ERR)
            return r;
    }

    if (argc > 3) {
        if ((r = rjs_to_number(rt, ms, &milli)) == RJS_ERR)
            return r;
    }

    if (isnan(t)) {
        rjs_value_set_number(rt, rv, NAN);
        return RJS_OK;
    }

    t = local_time(rt, t);

    if (argc <= 1)
        m = min_from_time(t);
    if (argc <= 2)
        s = sec_from_time(t);
    if (argc <= 3)
        milli = ms_from_time(t);

    u = make_date(rt, day(t), make_time(rt, h, m, s, milli));
    u = time_clip(rt, utc(rt, u));

    d = (RJS_Date*)rjs_value_get_object(rt, thiz);
    d->date = u;

    rjs_value_set_number(rt, rv, u);
    return RJS_OK;
}

/*Date.prototype.setMilliseconds*/
static RJS_NF(Date_prototype_setMilliseconds)
{
    RJS_Value *ms = rjs_argument_get(rt, args, argc, 0);
    RJS_Date  *d;
    RJS_Result r;
    RJS_Number t, milli, u;

    if ((r = this_time_value(rt, thiz, &t)) == RJS_ERR)
        return r;

    if ((r = rjs_to_number(rt, ms, &milli)) == RJS_ERR)
        return r;

    if (isnan(t)) {
        rjs_value_set_number(rt, rv, NAN);
        return RJS_OK;
    }

    t = local_time(rt, t);

    u = make_date(rt, day(t), make_time(rt, hour_from_time(t), min_from_time(t), sec_from_time(t), milli));
    u = time_clip(rt, utc(rt, u));

    d = (RJS_Date*)rjs_value_get_object(rt, thiz);
    d->date = u;

    rjs_value_set_number(rt, rv, u);
    return RJS_OK;
}

/*Date.prototype.setMinutes*/
static RJS_NF(Date_prototype_setMinutes)
{
    RJS_Value *min = rjs_argument_get(rt, args, argc, 0);
    RJS_Value *sec = rjs_argument_get(rt, args, argc, 1);
    RJS_Value *ms  = rjs_argument_get(rt, args, argc, 2);
    RJS_Date  *d;
    RJS_Result r;
    RJS_Number t, m, s, milli, u;

    if ((r = this_time_value(rt, thiz, &t)) == RJS_ERR)
        return r;

    if ((r = rjs_to_number(rt, min, &m)) == RJS_ERR)
        return r;

    if (argc > 1) {
        if ((r = rjs_to_number(rt, sec, &s)) == RJS_ERR)
            return r;
    }

    if (argc > 2) {
        if ((r = rjs_to_number(rt, ms, &milli)) == RJS_ERR)
            return r;
    }

    if (isnan(t)) {
        rjs_value_set_number(rt, rv, NAN);
        return RJS_OK;
    }

    t = local_time(rt, t);

    if (argc <= 1)
        s = sec_from_time(t);
    if (argc <= 2)
        milli = ms_from_time(t);

    u = make_date(rt, day(t), make_time(rt, hour_from_time(t), m, s, milli));
    u = time_clip(rt, utc(rt, u));

    d = (RJS_Date*)rjs_value_get_object(rt, thiz);
    d->date = u;

    rjs_value_set_number(rt, rv, u);
    return RJS_OK;
}

/*Date.prototype.setMonth*/
static RJS_NF(Date_prototype_setMonth)
{
    RJS_Value *month = rjs_argument_get(rt, args, argc, 0);
    RJS_Value *date  = rjs_argument_get(rt, args, argc, 1);
    RJS_Date  *d;
    RJS_Result r;
    RJS_Number t, m, dt, u;

    if ((r = this_time_value(rt, thiz, &t)) == RJS_ERR)
        return r;

    if ((r = rjs_to_number(rt, month, &m)) == RJS_ERR)
        return r;

    if (argc > 1) {
        if ((r = rjs_to_number(rt, date, &dt)) == RJS_ERR)
            return r;
    }

    if (isnan(t)) {
        rjs_value_set_number(rt, rv, NAN);
        return RJS_OK;
    }

    t = local_time(rt, t);

    if (argc <= 1)
        dt = date_from_time(t);

    u = make_date(rt, make_day(rt, year_from_time(t), m, dt), time_within_day(t));
    u = time_clip(rt, utc(rt, u));

    d = (RJS_Date*)rjs_value_get_object(rt, thiz);
    d->date = u;

    rjs_value_set_number(rt, rv, u);
    return RJS_OK;
}

/*Date.prototype.setSeconds*/
static RJS_NF(Date_prototype_setSeconds)
{
    RJS_Value *sec = rjs_argument_get(rt, args, argc, 0);
    RJS_Value *ms  = rjs_argument_get(rt, args, argc, 1);
    RJS_Date  *d;
    RJS_Result r;
    RJS_Number t, s, milli, u;

    if ((r = this_time_value(rt, thiz, &t)) == RJS_ERR)
        return r;

    if ((r = rjs_to_number(rt, sec, &s)) == RJS_ERR)
        return r;

    if (argc > 1) {
        if ((r = rjs_to_number(rt, ms, &milli)) == RJS_ERR)
            return r;
    }

    if (isnan(t)) {
        rjs_value_set_number(rt, rv, NAN);
        return RJS_OK;
    }

    t = local_time(rt, t);

    if (argc <= 1)
        milli = ms_from_time(t);

    u = make_date(rt, day(t), make_time(rt, hour_from_time(t), min_from_time(t), s, milli));
    u = time_clip(rt, utc(rt, u));

    d = (RJS_Date*)rjs_value_get_object(rt, thiz);
    d->date = u;

    rjs_value_set_number(rt, rv, u);
    return RJS_OK;
}

/*Date.prototype.setTime*/
static RJS_NF(Date_prototype_setTime)
{
    RJS_Value *time = rjs_argument_get(rt, args, argc, 0);
    RJS_Date  *d;
    RJS_Result r;
    RJS_Number t, u;

    if ((r = this_time_value(rt, thiz, &t)) == RJS_ERR)
        return r;

    if ((r = rjs_to_number(rt, time, &t)) == RJS_ERR)
        return r;

    u = time_clip(rt, t);

    d = (RJS_Date*)rjs_value_get_object(rt, thiz);
    d->date = u;

    rjs_value_set_number(rt, rv, u);
    return RJS_OK;
}

/*Date.prototype.setUTCDate*/
static RJS_NF(Date_prototype_setUTCDate)
{
    RJS_Value *date = rjs_argument_get(rt, args, argc, 0);
    RJS_Date  *d;
    RJS_Result r;
    RJS_Number t, dt, u;

    if ((r = this_time_value(rt, thiz, &t)) == RJS_ERR)
        return r;

    if ((r = rjs_to_number(rt, date, &dt)) == RJS_ERR)
        return r;

    if (isnan(t)) {
        rjs_value_set_number(rt, rv, NAN);
        return RJS_OK;
    }

    u = make_date(rt, make_day(rt, year_from_time(t), month_from_time(t), dt), time_within_day(t));
    u = time_clip(rt, u);

    d = (RJS_Date*)rjs_value_get_object(rt, thiz);
    d->date = u;

    rjs_value_set_number(rt, rv, u);
    return RJS_OK;
}

/*Date.prototype.setUTCFullYear*/
static RJS_NF(Date_prototype_setUTCFullYear)
{
    RJS_Value *year  = rjs_argument_get(rt, args, argc, 0);
    RJS_Value *month = rjs_argument_get(rt, args, argc, 1);
    RJS_Value *date  = rjs_argument_get(rt, args, argc, 2);
    RJS_Date  *d;
    RJS_Result r;
    RJS_Number t, y, m, dt, u;

    if ((r = this_time_value(rt, thiz, &t)) == RJS_ERR)
        return r;

    if ((r = rjs_to_number(rt, year, &y)) == RJS_ERR)
        return r;

    if (isnan(t)) {
        t = 0;
    }

    if (argc > 1) {
        if ((r = rjs_to_number(rt, month, &m)) == RJS_ERR)
            return r;
    } else {
        m = month_from_time(t);
    }

    if (argc > 2) {
        if ((r = rjs_to_number(rt, date, &dt)) == RJS_ERR)
            return r;
    } else {
        dt = date_from_time(t);
    }

    u = make_date(rt, make_day(rt, y, m, dt), time_within_day(t));
    u = time_clip(rt, u);

    d = (RJS_Date*)rjs_value_get_object(rt, thiz);
    d->date = u;

    rjs_value_set_number(rt, rv, u);
    return RJS_OK;
}

/*Date.prototype.setUTCHours*/
static RJS_NF(Date_prototype_setUTCHours)
{
    RJS_Value *hour = rjs_argument_get(rt, args, argc, 0);
    RJS_Value *min  = rjs_argument_get(rt, args, argc, 1);
    RJS_Value *sec  = rjs_argument_get(rt, args, argc, 2);
    RJS_Value *ms   = rjs_argument_get(rt, args, argc, 3);
    RJS_Date  *d;
    RJS_Result r;
    RJS_Number t, h, m, s, milli, u;

    if ((r = this_time_value(rt, thiz, &t)) == RJS_ERR)
        return r;

    if ((r = rjs_to_number(rt, hour, &h)) == RJS_ERR)
        return r;

    if (argc > 1) {
        if ((r = rjs_to_number(rt, min, &m)) == RJS_ERR)
            return r;
    }

    if (argc > 2) {
        if ((r = rjs_to_number(rt, sec, &s)) == RJS_ERR)
            return r;
    }

    if (argc > 3) {
        if ((r = rjs_to_number(rt, ms, &milli)) == RJS_ERR)
            return r;
    }

    if (isnan(t)) {
        rjs_value_set_number(rt, rv, NAN);
        return RJS_OK;
    }

    if (argc <= 1)
        m = min_from_time(t);
    if (argc <= 2)
        s = sec_from_time(t);
    if (argc <= 3)
        milli = ms_from_time(t);

    u = make_date(rt, day(t), make_time(rt, h, m, s, milli));
    u = time_clip(rt, u);

    d = (RJS_Date*)rjs_value_get_object(rt, thiz);
    d->date = u;

    rjs_value_set_number(rt, rv, u);
    return RJS_OK;
}

/*Date.prototype.setUTCMilliseconds*/
static RJS_NF(Date_prototype_setUTCMilliseconds)
{
    RJS_Value *ms = rjs_argument_get(rt, args, argc, 0);
    RJS_Date  *d;
    RJS_Result r;
    RJS_Number t, milli, u;

    if ((r = this_time_value(rt, thiz, &t)) == RJS_ERR)
        return r;

    if ((r = rjs_to_number(rt, ms, &milli)) == RJS_ERR)
        return r;

    if (isnan(t)) {
        rjs_value_set_number(rt, rv, NAN);
        return RJS_OK;
    }

    u = make_date(rt, day(t), make_time(rt, hour_from_time(t), ms_from_time(t), sec_from_time(t), milli));
    u = time_clip(rt, u);

    d = (RJS_Date*)rjs_value_get_object(rt, thiz);
    d->date = u;

    rjs_value_set_number(rt, rv, u);
    return RJS_OK;
}

/*Date.prototype.setUTCMinutes*/
static RJS_NF(Date_prototype_setUTCMinutes)
{
    RJS_Value *min = rjs_argument_get(rt, args, argc, 0);
    RJS_Value *sec = rjs_argument_get(rt, args, argc, 1);
    RJS_Value *ms  = rjs_argument_get(rt, args, argc, 2);
    RJS_Date  *d;
    RJS_Result r;
    RJS_Number t, m, s, milli, u;

    if ((r = this_time_value(rt, thiz, &t)) == RJS_ERR)
        return r;

    if ((r = rjs_to_number(rt, min, &m)) == RJS_ERR)
        return r;

    if (argc > 1) {
        if ((r = rjs_to_number(rt, sec, &s)) == RJS_ERR)
            return r;
    }

    if (argc > 2) {
        if ((r = rjs_to_number(rt, ms, &milli)) == RJS_ERR)
            return r;
    }

    if (isnan(t)) {
        rjs_value_set_number(rt, rv, NAN);
        return RJS_OK;
    }

    if (argc <= 1)
        s = sec_from_time(t);
    if (argc <= 2)
        milli = ms_from_time(t);

    u = make_date(rt, day(t), make_time(rt, hour_from_time(t), m, s, milli));
    u = time_clip(rt, u);

    d = (RJS_Date*)rjs_value_get_object(rt, thiz);
    d->date = u;

    rjs_value_set_number(rt, rv, u);
    return RJS_OK;
}

/*Date.prototype.setUTCMonth*/
static RJS_NF(Date_prototype_setUTCMonth)
{
    RJS_Value *month = rjs_argument_get(rt, args, argc, 0);
    RJS_Value *date  = rjs_argument_get(rt, args, argc, 1);
    RJS_Date  *d;
    RJS_Result r;
    RJS_Number t, m, dt, u;

    if ((r = this_time_value(rt, thiz, &t)) == RJS_ERR)
        return r;

    if ((r = rjs_to_number(rt, month, &m)) == RJS_ERR)
        return r;

    if (argc > 1) {
        if ((r = rjs_to_number(rt, date, &dt)) == RJS_ERR)
            return r;
    }

    if (isnan(t)) {
        rjs_value_set_number(rt, rv, NAN);
        return RJS_OK;
    }

    if (argc <= 1)
        dt = date_from_time(t);

    u = make_date(rt, make_day(rt, year_from_time(t), m, dt), time_within_day(t));
    u = time_clip(rt, u);

    d = (RJS_Date*)rjs_value_get_object(rt, thiz);
    d->date = u;

    rjs_value_set_number(rt, rv, u);
    return RJS_OK;
}

/*Date.prototype.setUTCSeconds*/
static RJS_NF(Date_prototype_setUTCSeconds)
{
    RJS_Value *sec = rjs_argument_get(rt, args, argc, 0);
    RJS_Value *ms  = rjs_argument_get(rt, args, argc, 1);
    RJS_Date  *d;
    RJS_Result r;
    RJS_Number t, s, milli, u;

    if ((r = this_time_value(rt, thiz, &t)) == RJS_ERR)
        return r;

    if ((r = rjs_to_number(rt, sec, &s)) == RJS_ERR)
        return r;

    if (argc > 1) {
        if ((r = rjs_to_number(rt, ms, &milli)) == RJS_ERR)
            return r;
    }

    if (isnan(t)) {
        rjs_value_set_number(rt, rv, NAN);
        return RJS_OK;
    }

    if (argc <= 1)
        milli = ms_from_time(t);

    u = make_date(rt, day(t), make_time(rt, hour_from_time(t), min_from_time(t), s, milli));
    u = time_clip(rt, u);

    d = (RJS_Date*)rjs_value_get_object(rt, thiz);
    d->date = u;

    rjs_value_set_number(rt, rv, u);
    return RJS_OK;
}

/*Date.prototype.toDateString*/
static RJS_NF(Date_prototype_toDateString)
{
    RJS_Result r;
    RJS_Number t;

    if ((r = this_time_value(rt, thiz, &t)) == RJS_ERR)
        return r;

    if (isnan(t)) {
        r = rjs_string_from_chars(rt, rv, "Invalid Date", -1);
    } else {
        int  y, m, d, wd;
        char buf[256];

        t = local_time(rt, t);

        y  = year_from_time(t);
        m  = month_from_time(t);
        d  = date_from_time(t);
        wd = week_day(t);

        snprintf(buf, sizeof(buf), "%s %s %02d %s%04d",
                week_day_strings[wd],
                month_strings[m],
                d,
                (y < 0) ? "-" : "",
                RJS_ABS(y));

        r = rjs_string_from_chars(rt, rv, buf, -1);
    }

    return r;
}

/*Date.prototype.toISOString*/
static RJS_NF(Date_prototype_toISOString)
{
    RJS_Result r;
    RJS_Number t;

    if ((r = this_time_value(rt, thiz, &t)) == RJS_ERR)
        return r;

    if (isnan(t)) {
        r = rjs_throw_range_error(rt, _("date value overflow"));
    } else {
        int  y, m, d, h, min, sec, ms;
        char buf[256];

        y   = year_from_time(t);
        m   = month_from_time(t);
        d   = date_from_time(t);
        h   = hour_from_time(t);
        min = min_from_time(t);
        sec = sec_from_time(t);
        ms  = ms_from_time(t);

        snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02d.%03dZ",
                y, m + 1, d, h, min, sec, ms);

        r = rjs_string_from_chars(rt, rv, buf, -1);
    }

    return r;
}

/*Date.prototype.toJSON*/
static RJS_NF(Date_prototype_toJSON)
{
    size_t     top = rjs_value_stack_save(rt);
    RJS_Value *o   = rjs_value_stack_push(rt);
    RJS_Value *tv  = rjs_value_stack_push(rt);
    RJS_Result r;

    if ((r = rjs_to_object(rt, thiz, o)) == RJS_ERR)
        goto end;

    if ((r = rjs_to_primitive(rt, o, tv, RJS_VALUE_NUMBER)) == RJS_ERR)
        goto end;

    if (rjs_value_is_number(rt, tv)) {
        RJS_Number n = rjs_value_get_number(rt, tv);

        if (isinf(n) || isnan(n)) {
            rjs_value_set_null(rt, rv);
            r = RJS_OK;
            goto end;
        }
    }

    r = rjs_invoke(rt, o, rjs_pn_toISOString(rt), NULL, 0, rv);
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Date.prototype.toString*/
static RJS_NF(Date_prototype_toString)
{
    RJS_Result r;
    RJS_Number t;

    if ((r = this_time_value(rt, thiz, &t)) == RJS_ERR)
        return r;

    return r = to_date_string(rt, t, rv);
}

/*Date.prototype.toTimeString*/
static RJS_NF(Date_prototype_toTimeString)
{
    RJS_Result r;
    RJS_Number tv;

    if ((r = this_time_value(rt, thiz, &tv)) == RJS_ERR)
        return r;

    if (isnan(tv)) {
        r = rjs_string_from_chars(rt, rv, "Invalid Date", -1);
    } else {
        RJS_Number t;
        int        h, m, s, off, abs_off, off_min, off_hour;
        char       buf[256];

        t = local_time(rt, tv);

        h = hour_from_time(t);
        m = min_from_time(t);
        s = sec_from_time(t);

        off = local_tza(rt, tv, RJS_TRUE);

        abs_off  = RJS_ABS(off);
        off_min  = min_from_time(abs_off);
        off_hour = hour_from_time(abs_off);

        snprintf(buf, sizeof(buf), "%02d:%02d:%02d GMT%s%02d%02d",
                h,
                m,
                s,
                (off < 0) ? "-" : "+",
                off_hour,
                off_min);

        r = rjs_string_from_chars(rt, rv, buf, -1);
    }

    return r;
}

/*Date.prototype.toUTCString*/
static RJS_NF(Date_prototype_toUTCString)
{
    RJS_Result r;
    RJS_Number tv;

    if ((r = this_time_value(rt, thiz, &tv)) == RJS_ERR)
        return r;

    if (isnan(tv)) {
        r = rjs_string_from_chars(rt, rv, "Invalid Date", -1);
    } else {
        int  wd, y, m, d, h, min, s;
        char buf[256];

        wd  = week_day(tv);
        y   = year_from_time(tv);
        m   = month_from_time(tv);
        d   = date_from_time(tv);
        h   = hour_from_time(tv);
        min = min_from_time(tv);
        s   = sec_from_time(tv);

        snprintf(buf, sizeof(buf), "%s, %02d %s %s%04d %02d:%02d:%02d GMT",
                week_day_strings[wd],
                d,
                month_strings[m],
                (y < 0) ? "-" : "",
                RJS_ABS(y),
                h,
                min,
                s);

        r = rjs_string_from_chars(rt, rv, buf, -1);
    }

    return r;
}

/*Date.prototype.valueOf*/
static RJS_NF(Date_prototype_valueOf)
{
    RJS_Result r;
    RJS_Number tv;

    if ((r = this_time_value(rt, thiz, &tv)) == RJS_ERR)
        return r;

    rjs_value_set_number(rt, rv, tv);
    return RJS_OK;
}

/*Date.prototype[@@toPrimitive]*/
static RJS_NF(Date_prototype_toPrimitive)
{
    RJS_Value    *hint = rjs_argument_get(rt, args, argc, 0);
    RJS_ValueType vt;

    if (!rjs_value_is_object(rt, thiz))
        return rjs_throw_type_error(rt, _("the value is not an object"));

    if (rjs_same_value(rt, hint, rjs_s_string(rt))
            || rjs_same_value(rt, hint, rjs_s_default(rt))) {
        vt = RJS_VALUE_STRING;
    } else if (rjs_same_value(rt, hint, rjs_s_number(rt))) {
        vt = RJS_VALUE_NUMBER;
    } else {
        return rjs_throw_type_error(rt, _("illegal \"@@toPrimitive\" hint"));
    }

    return rjs_ordinary_to_primitive(rt, thiz, rv, vt);
}

static const RJS_BuiltinFuncDesc
date_prototype_function_descs[] = {
    {
        "getDate",
        0,
        Date_prototype_getDate
    },
    {
        "getDay",
        0,
        Date_prototype_getDay
    },
    {
        "getFullYear",
        0,
        Date_prototype_getFullYear
    },
    {
        "getHours",
        0,
        Date_prototype_getHours
    },
    {
        "getMilliseconds",
        0,
        Date_prototype_getMilliseconds
    },
    {
        "getMinutes",
        0,
        Date_prototype_getMinutes
    },
    {
        "getMonth",
        0,
        Date_prototype_getMonth
    },
    {
        "getSeconds",
        0,
        Date_prototype_getSeconds
    },
    {
        "getTime",
        0,
        Date_prototype_getTime
    },
    {
        "getTimezoneOffset",
        0,
        Date_prototype_getTimezoneOffset
    },
    {
        "getUTCDate",
        0,
        Date_prototype_getUTCDate
    },
    {
        "getUTCDay",
        0,
        Date_prototype_getUTCDay
    },
    {
        "getUTCFullYear",
        0,
        Date_prototype_getUTCFullYear
    },
    {
        "getUTCHours",
        0,
        Date_prototype_getUTCHours
    },
    {
        "getUTCMilliseconds",
        0,
        Date_prototype_getUTCMilliseconds
    },
    {
        "getUTCMinutes",
        0,
        Date_prototype_getUTCMinutes
    },
    {
        "getUTCMonth",
        0,
        Date_prototype_getUTCMonth
    },
    {
        "getUTCSeconds",
        0,
        Date_prototype_getUTCSeconds
    },
    {
        "setDate",
        1,
        Date_prototype_setDate
    },
    {
        "setFullYear",
        3,
        Date_prototype_setFullYear
    },
    {
        "setHours",
        4,
        Date_prototype_setHours
    },
    {
        "setMilliseconds",
        1,
        Date_prototype_setMilliseconds
    },
    {
        "setMinutes",
        3,
        Date_prototype_setMinutes
    },
    {
        "setMonth",
        2,
        Date_prototype_setMonth
    },
    {
        "setSeconds",
        2,
        Date_prototype_setSeconds
    },
    {
        "setTime",
        1,
        Date_prototype_setTime
    },
    {
        "setUTCDate",
        1,
        Date_prototype_setUTCDate
    },
    {
        "setUTCFullYear",
        3,
        Date_prototype_setUTCFullYear
    },
    {
        "setUTCHours",
        4,
        Date_prototype_setUTCHours
    },
    {
        "setUTCMilliseconds",
        1,
        Date_prototype_setUTCMilliseconds
    },
    {
        "setUTCMinutes",
        3,
        Date_prototype_setUTCMinutes
    },
    {
        "setUTCMonth",
        2,
        Date_prototype_setUTCMonth
    },
    {
        "setUTCSeconds",
        2,
        Date_prototype_setUTCSeconds
    },
    {
        "toDateString",
        0,
        Date_prototype_toDateString
    },
    {
        "toISOString",
        0,
        Date_prototype_toISOString
    },
    {
        "toJSON",
        1,
        Date_prototype_toJSON
    },
    {
        "toLocaleDateString",
        0,
        Date_prototype_toDateString
    },
    {
        "toLocaleString",
        0,
        Date_prototype_toString
    },
    {
        "toLocaleTimeString",
        0,
        Date_prototype_toTimeString
    },
    {
        "toString",
        0,
        Date_prototype_toString
    },
    {
        "toTimeString",
        0,
        Date_prototype_toTimeString
    },
    {
        "toUTCString",
        0,
        Date_prototype_toUTCString
    },
    {
        "valueOf",
        0,
        Date_prototype_valueOf
    },
    {
        "@@toPrimitive",
        1,
        Date_prototype_toPrimitive
    },
    {NULL}
};

static const RJS_BuiltinObjectDesc
date_prototype_desc = {
    "Date",
    NULL,
    NULL,
    NULL,
    NULL,
    date_prototype_function_descs,
    NULL,
    NULL,
    "Date_prototype"
};