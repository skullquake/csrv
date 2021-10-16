#include "jsi.h"
#include "jsvalue.h"
#include "jsbuiltin.h"

#include <time.h>

#if defined(__unix__) || defined(__APPLE__)
#include <sys/time.h>
#elif defined(_WIN32)
#include <sys/timeb.h>
#endif

#define mjs_optnumber(J,I,V) (mjs_isdefined(J,I) ? mjs_tonumber(J,I) : V)

static double Now(void)
{
#if defined(__unix__) || defined(__APPLE__)
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return floor(tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0);
#elif defined(_WIN32)
	struct _timeb tv;
	_ftime(&tv);
	return tv.time * 1000.0 + tv.millitm;
#else
	return time(NULL) * 1000.0;
#endif
}

static double LocalTZA(void)
{
	static int once = 1;
	static double tza = 0;
	if (once) {
		time_t now = time(NULL);
		time_t utc = mktime(gmtime(&now));
		time_t loc = mktime(localtime(&now));
		tza = (loc - utc) * 1000;
		once = 0;
	}
	return tza;
}

static double DaylightSavingTA(double t)
{
	return 0; /* TODO */
}

/* Helpers from the ECMA 262 specification */

#define HoursPerDay		24.0
#define MinutesPerDay		(HoursPerDay * MinutesPerHour)
#define MinutesPerHour		60.0
#define SecondsPerDay		(MinutesPerDay * SecondsPerMinute)
#define SecondsPerHour		(MinutesPerHour * SecondsPerMinute)
#define SecondsPerMinute	60.0

#define msPerDay	(SecondsPerDay * msPerSecond)
#define msPerHour	(SecondsPerHour * msPerSecond)
#define msPerMinute	(SecondsPerMinute * msPerSecond)
#define msPerSecond	1000.0

static double pmod(double x, double y)
{
	x = fmod(x, y);
	if (x < 0)
		x += y;
	return x;
}

static int Day(double t)
{
	return floor(t / msPerDay);
}

static double TimeWithinDay(double t)
{
	return pmod(t, msPerDay);
}

static int DaysInYear(int y)
{
	return y % 4 == 0 && (y % 100 || (y % 400 == 0)) ? 366 : 365;
}

static int DayFromYear(int y)
{
	return 365 * (y - 1970) +
		floor((y - 1969) / 4.0) -
		floor((y - 1901) / 100.0) +
		floor((y - 1601) / 400.0);
}

static double TimeFromYear(int y)
{
	return DayFromYear(y) * msPerDay;
}

static int YearFromTime(double t)
{
	int y = floor(t / (msPerDay * 365.2425)) + 1970;
	double t2 = TimeFromYear(y);
	if (t2 > t)
		--y;
	else if (t2 + msPerDay * DaysInYear(y) <= t)
		++y;
	return y;
}

static int InLeapYear(int t)
{
	return DaysInYear(YearFromTime(t)) == 366;
}

static int DayWithinYear(double t)
{
	return Day(t) - DayFromYear(YearFromTime(t));
}

static int MonthFromTime(double t)
{
	int day = DayWithinYear(t);
	int leap = InLeapYear(t);
	if (day < 31) return 0;
	if (day < 59 + leap) return 1;
	if (day < 90 + leap) return 2;
	if (day < 120 + leap) return 3;
	if (day < 151 + leap) return 4;
	if (day < 181 + leap) return 5;
	if (day < 212 + leap) return 6;
	if (day < 243 + leap) return 7;
	if (day < 273 + leap) return 8;
	if (day < 304 + leap) return 9;
	if (day < 334 + leap) return 10;
	return 11;
}

static int DateFromTime(double t)
{
	int day = DayWithinYear(t);
	int leap = InLeapYear(t);
	switch (MonthFromTime(t)) {
	case 0: return day + 1;
	case 1: return day - 30;
	case 2: return day - 58 - leap;
	case 3: return day - 89 - leap;
	case 4: return day - 119 - leap;
	case 5: return day - 150 - leap;
	case 6: return day - 180 - leap;
	case 7: return day - 211 - leap;
	case 8: return day - 242 - leap;
	case 9: return day - 272 - leap;
	case 10: return day - 303 - leap;
	default : return day - 333 - leap;
	}
}

static int WeekDay(double t)
{
	return pmod(Day(t) + 4, 7);
}

static double LocalTime(double utc)
{
	return utc + LocalTZA() + DaylightSavingTA(utc);
}

static double UTC(double loc)
{
	return loc - LocalTZA() - DaylightSavingTA(loc - LocalTZA());
}

static int HourFromTime(double t)
{
	return pmod(floor(t / msPerHour), HoursPerDay);
}

static int MinFromTime(double t)
{
	return pmod(floor(t / msPerMinute), MinutesPerHour);
}

static int SecFromTime(double t)
{
	return pmod(floor(t / msPerSecond), SecondsPerMinute);
}

static int msFromTime(double t)
{
	return pmod(t, msPerSecond);
}

static double MakeTime(double hour, double min, double sec, double ms)
{
	return ((hour * MinutesPerHour + min) * SecondsPerMinute + sec) * msPerSecond + ms;
}

static double MakeDay(double y, double m, double date)
{
	/*
	 * The following array contains the day of year for the first day of
	 * each month, where index 0 is January, and day 0 is January 1.
	 */
	static const double firstDayOfMonth[2][12] = {
		{0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334},
		{0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335}
	};

	double yd, md;
	int im;

	y += floor(m / 12);
	m = pmod(m, 12);

	im = (int)m;
	if (im < 0 || im >= 12)
		return NAN;

	yd = floor(TimeFromYear(y) / msPerDay);
	md = firstDayOfMonth[DaysInYear(y) == 366][im];

	return yd + md + date - 1;
}

static double MakeDate(double day, double time)
{
	return day * msPerDay + time;
}

static double TimeClip(double t)
{
	if (!isfinite(t))
		return NAN;
	if (fabs(t) > 8.64e15)
		return NAN;
	return t < 0 ? -floor(-t) : floor(t);
}

static int toint(const char **sp, int w, int *v)
{
	const char *s = *sp;
	*v = 0;
	while (w--) {
		if (*s < '0' || *s > '9')
			return 0;
		*v = *v * 10 + (*s++ - '0');
	}
	*sp = s;
	return 1;
}

static double parseDateTime(const char *s)
{
	int y = 1970, m = 1, d = 1, H = 0, M = 0, S = 0, ms = 0;
	int tza = 0;
	double t;

	/* Parse ISO 8601 formatted date and time: */
	/* YYYY("-"MM("-"DD)?)?("T"HH":"mm(":"ss("."sss)?)?("Z"|[+-]HH(":"mm)?)?)? */

	if (!toint(&s, 4, &y)) return NAN;
	if (*s == '-') {
		s += 1;
		if (!toint(&s, 2, &m)) return NAN;
		if (*s == '-') {
			s += 1;
			if (!toint(&s, 2, &d)) return NAN;
		}
	}

	if (*s == 'T') {
		s += 1;
		if (!toint(&s, 2, &H)) return NAN;
		if (*s != ':') return NAN;
		s += 1;
		if (!toint(&s, 2, &M)) return NAN;
		if (*s == ':') {
			s += 1;
			if (!toint(&s, 2, &S)) return NAN;
			if (*s == '.') {
				s += 1;
				if (!toint(&s, 3, &ms)) return NAN;
			}
		}
		if (*s == 'Z') {
			s += 1;
			tza = 0;
		} else if (*s == '+' || *s == '-') {
			int tzh = 0, tzm = 0;
			int tzs = *s == '+' ? 1 : -1;
			s += 1;
			if (!toint(&s, 2, &tzh)) return NAN;
			if (*s == ':') {
				s += 1;
				if (!toint(&s, 2, &tzm)) return NAN;
			}
			if (tzh > 23 || tzm > 59) return NAN;
			tza = tzs * (tzh * msPerHour + tzm * msPerMinute);
		} else {
			tza = LocalTZA();
		}
	}

	if (*s) return NAN;

	if (m < 1 || m > 12) return NAN;
	if (d < 1 || d > 31) return NAN;
	if (H < 0 || H > 24) return NAN;
	if (M < 0 || M > 59) return NAN;
	if (S < 0 || S > 59) return NAN;
	if (ms < 0 || ms > 999) return NAN;
	if (H == 24 && (M != 0 || S != 0 || ms != 0)) return NAN;

	/* TODO: DaylightSavingTA on local times */
	t = MakeDate(MakeDay(y, m-1, d), MakeTime(H, M, S, ms));
	return t - tza;
}

/* date formatting */

static char *fmtdate(char *buf, double t)
{
	int y = YearFromTime(t);
	int m = MonthFromTime(t);
	int d = DateFromTime(t);
	if (!isfinite(t))
		return "Invalid Date";
	sprintf(buf, "%04d-%02d-%02d", y, m+1, d);
	return buf;
}

static char *fmttime(char *buf, double t, double tza)
{
	int H = HourFromTime(t);
	int M = MinFromTime(t);
	int S = SecFromTime(t);
	int ms = msFromTime(t);
	int tzh = HourFromTime(fabs(tza));
	int tzm = MinFromTime(fabs(tza));
	if (!isfinite(t))
		return "Invalid Date";
	if (tza == 0)
		sprintf(buf, "%02d:%02d:%02d.%03dZ", H, M, S, ms);
	else if (tza < 0)
		sprintf(buf, "%02d:%02d:%02d.%03d-%02d:%02d", H, M, S, ms, tzh, tzm);
	else
		sprintf(buf, "%02d:%02d:%02d.%03d+%02d:%02d", H, M, S, ms, tzh, tzm);
	return buf;
}

static char *fmtdatetime(char *buf, double t, double tza)
{
	char dbuf[20], tbuf[20];
	if (!isfinite(t))
		return "Invalid Date";
	fmtdate(dbuf, t);
	fmttime(tbuf, t, tza);
	sprintf(buf, "%sT%s", dbuf, tbuf);
	return buf;
}

/* Date functions */

static double mjs_todate(mjs_State *J, int idx)
{
	mjs_Object *self = mjs_toobject(J, idx);
	if (self->type != JS_CDATE)
		mjs_typeerror(J, "not a date");
	return self->u.number;
}

static void mjs_setdate(mjs_State *J, int idx, double t)
{
	mjs_Object *self = mjs_toobject(J, idx);
	if (self->type != JS_CDATE)
		mjs_typeerror(J, "not a date");
	self->u.number = TimeClip(t);
	mjs_pushnumber(J, self->u.number);
}

static void D_parse(mjs_State *J)
{
	double t = parseDateTime(mjs_tostring(J, 1));
	mjs_pushnumber(J, t);
}

static void D_UTC(mjs_State *J)
{
	double y, m, d, H, M, S, ms, t;
	y = mjs_tonumber(J, 1);
	if (y < 100) y += 1900;
	m = mjs_tonumber(J, 2);
	d = mjs_optnumber(J, 3, 1);
	H = mjs_optnumber(J, 4, 0);
	M = mjs_optnumber(J, 5, 0);
	S = mjs_optnumber(J, 6, 0);
	ms = mjs_optnumber(J, 7, 0);
	t = MakeDate(MakeDay(y, m, d), MakeTime(H, M, S, ms));
	t = TimeClip(t);
	mjs_pushnumber(J, t);
}

static void D_now(mjs_State *J)
{
	mjs_pushnumber(J, Now());
}

static void jsB_Date(mjs_State *J)
{
	char buf[64];
	mjs_pushstring(J, fmtdatetime(buf, LocalTime(Now()), LocalTZA()));
}

static void jsB_new_Date(mjs_State *J)
{
	int top = mjs_gettop(J);
	mjs_Object *obj;
	double t;

	if (top == 1)
		t = Now();
	else if (top == 2) {
		mjs_toprimitive(J, 1, JS_HNONE);
		if (mjs_isstring(J, 1))
			t = parseDateTime(mjs_tostring(J, 1));
		else
			t = TimeClip(mjs_tonumber(J, 1));
	} else {
		double y, m, d, H, M, S, ms;
		y = mjs_tonumber(J, 1);
		if (y < 100) y += 1900;
		m = mjs_tonumber(J, 2);
		d = mjs_optnumber(J, 3, 1);
		H = mjs_optnumber(J, 4, 0);
		M = mjs_optnumber(J, 5, 0);
		S = mjs_optnumber(J, 6, 0);
		ms = mjs_optnumber(J, 7, 0);
		t = MakeDate(MakeDay(y, m, d), MakeTime(H, M, S, ms));
		t = TimeClip(UTC(t));
	}

	obj = jsV_newobject(J, JS_CDATE, J->Date_prototype);
	obj->u.number = t;

	mjs_pushobject(J, obj);
}

static void Dp_valueOf(mjs_State *J)
{
	double t = mjs_todate(J, 0);
	mjs_pushnumber(J, t);
}

static void Dp_toString(mjs_State *J)
{
	char buf[64];
	double t = mjs_todate(J, 0);
	mjs_pushstring(J, fmtdatetime(buf, LocalTime(t), LocalTZA()));
}

static void Dp_toDateString(mjs_State *J)
{
	char buf[64];
	double t = mjs_todate(J, 0);
	mjs_pushstring(J, fmtdate(buf, LocalTime(t)));
}

static void Dp_toTimeString(mjs_State *J)
{
	char buf[64];
	double t = mjs_todate(J, 0);
	mjs_pushstring(J, fmttime(buf, LocalTime(t), LocalTZA()));
}

static void Dp_toUTCString(mjs_State *J)
{
	char buf[64];
	double t = mjs_todate(J, 0);
	mjs_pushstring(J, fmtdatetime(buf, t, 0));
}

static void Dp_toISOString(mjs_State *J)
{
	char buf[64];
	double t = mjs_todate(J, 0);
	if (!isfinite(t))
		mjs_rangeerror(J, "invalid date");
	mjs_pushstring(J, fmtdatetime(buf, t, 0));
}

static void Dp_getFullYear(mjs_State *J)
{
	double t = mjs_todate(J, 0);
	mjs_pushnumber(J, YearFromTime(LocalTime(t)));
}

static void Dp_getMonth(mjs_State *J)
{
	double t = mjs_todate(J, 0);
	mjs_pushnumber(J, MonthFromTime(LocalTime(t)));
}

static void Dp_getDate(mjs_State *J)
{
	double t = mjs_todate(J, 0);
	mjs_pushnumber(J, DateFromTime(LocalTime(t)));
}

static void Dp_getDay(mjs_State *J)
{
	double t = mjs_todate(J, 0);
	mjs_pushnumber(J, WeekDay(LocalTime(t)));
}

static void Dp_getHours(mjs_State *J)
{
	double t = mjs_todate(J, 0);
	mjs_pushnumber(J, HourFromTime(LocalTime(t)));
}

static void Dp_getMinutes(mjs_State *J)
{
	double t = mjs_todate(J, 0);
	mjs_pushnumber(J, MinFromTime(LocalTime(t)));
}

static void Dp_getSeconds(mjs_State *J)
{
	double t = mjs_todate(J, 0);
	mjs_pushnumber(J, SecFromTime(LocalTime(t)));
}

static void Dp_getMilliseconds(mjs_State *J)
{
	double t = mjs_todate(J, 0);
	mjs_pushnumber(J, msFromTime(LocalTime(t)));
}

static void Dp_getUTCFullYear(mjs_State *J)
{
	double t = mjs_todate(J, 0);
	mjs_pushnumber(J, YearFromTime(t));
}

static void Dp_getUTCMonth(mjs_State *J)
{
	double t = mjs_todate(J, 0);
	mjs_pushnumber(J, MonthFromTime(t));
}

static void Dp_getUTCDate(mjs_State *J)
{
	double t = mjs_todate(J, 0);
	mjs_pushnumber(J, DateFromTime(t));
}

static void Dp_getUTCDay(mjs_State *J)
{
	double t = mjs_todate(J, 0);
	mjs_pushnumber(J, WeekDay(t));
}

static void Dp_getUTCHours(mjs_State *J)
{
	double t = mjs_todate(J, 0);
	mjs_pushnumber(J, HourFromTime(t));
}

static void Dp_getUTCMinutes(mjs_State *J)
{
	double t = mjs_todate(J, 0);
	mjs_pushnumber(J, MinFromTime(t));
}

static void Dp_getUTCSeconds(mjs_State *J)
{
	double t = mjs_todate(J, 0);
	mjs_pushnumber(J, SecFromTime(t));
}

static void Dp_getUTCMilliseconds(mjs_State *J)
{
	double t = mjs_todate(J, 0);
	mjs_pushnumber(J, msFromTime(t));
}

static void Dp_getTimezoneOffset(mjs_State *J)
{
	double t = mjs_todate(J, 0);
	mjs_pushnumber(J, (t - LocalTime(t)) / msPerMinute);
}

static void Dp_setTime(mjs_State *J)
{
	mjs_setdate(J, 0, mjs_tonumber(J, 1));
}

static void Dp_setMilliseconds(mjs_State *J)
{
	double t = LocalTime(mjs_todate(J, 0));
	double h = HourFromTime(t);
	double m = MinFromTime(t);
	double s = SecFromTime(t);
	double ms = mjs_tonumber(J, 1);
	mjs_setdate(J, 0, UTC(MakeDate(Day(t), MakeTime(h, m, s, ms))));
}

static void Dp_setSeconds(mjs_State *J)
{
	double t = LocalTime(mjs_todate(J, 0));
	double h = HourFromTime(t);
	double m = MinFromTime(t);
	double s = mjs_tonumber(J, 1);
	double ms = mjs_optnumber(J, 2, msFromTime(t));
	mjs_setdate(J, 0, UTC(MakeDate(Day(t), MakeTime(h, m, s, ms))));
}

static void Dp_setMinutes(mjs_State *J)
{
	double t = LocalTime(mjs_todate(J, 0));
	double h = HourFromTime(t);
	double m = mjs_tonumber(J, 1);
	double s = mjs_optnumber(J, 2, SecFromTime(t));
	double ms = mjs_optnumber(J, 3, msFromTime(t));
	mjs_setdate(J, 0, UTC(MakeDate(Day(t), MakeTime(h, m, s, ms))));
}

static void Dp_setHours(mjs_State *J)
{
	double t = LocalTime(mjs_todate(J, 0));
	double h = mjs_tonumber(J, 1);
	double m = mjs_optnumber(J, 2, MinFromTime(t));
	double s = mjs_optnumber(J, 3, SecFromTime(t));
	double ms = mjs_optnumber(J, 4, msFromTime(t));
	mjs_setdate(J, 0, UTC(MakeDate(Day(t), MakeTime(h, m, s, ms))));
}

static void Dp_setDate(mjs_State *J)
{
	double t = LocalTime(mjs_todate(J, 0));
	double y = YearFromTime(t);
	double m = MonthFromTime(t);
	double d = mjs_tonumber(J, 1);
	mjs_setdate(J, 0, UTC(MakeDate(MakeDay(y, m, d), TimeWithinDay(t))));
}

static void Dp_setMonth(mjs_State *J)
{
	double t = LocalTime(mjs_todate(J, 0));
	double y = YearFromTime(t);
	double m = mjs_tonumber(J, 1);
	double d = mjs_optnumber(J, 2, DateFromTime(t));
	mjs_setdate(J, 0, UTC(MakeDate(MakeDay(y, m, d), TimeWithinDay(t))));
}

static void Dp_setFullYear(mjs_State *J)
{
	double t = LocalTime(mjs_todate(J, 0));
	double y = mjs_tonumber(J, 1);
	double m = mjs_optnumber(J, 2, MonthFromTime(t));
	double d = mjs_optnumber(J, 3, DateFromTime(t));
	mjs_setdate(J, 0, UTC(MakeDate(MakeDay(y, m, d), TimeWithinDay(t))));
}

static void Dp_setUTCMilliseconds(mjs_State *J)
{
	double t = mjs_todate(J, 0);
	double h = HourFromTime(t);
	double m = MinFromTime(t);
	double s = SecFromTime(t);
	double ms = mjs_tonumber(J, 1);
	mjs_setdate(J, 0, MakeDate(Day(t), MakeTime(h, m, s, ms)));
}

static void Dp_setUTCSeconds(mjs_State *J)
{
	double t = mjs_todate(J, 0);
	double h = HourFromTime(t);
	double m = MinFromTime(t);
	double s = mjs_tonumber(J, 1);
	double ms = mjs_optnumber(J, 2, msFromTime(t));
	mjs_setdate(J, 0, MakeDate(Day(t), MakeTime(h, m, s, ms)));
}

static void Dp_setUTCMinutes(mjs_State *J)
{
	double t = mjs_todate(J, 0);
	double h = HourFromTime(t);
	double m = mjs_tonumber(J, 1);
	double s = mjs_optnumber(J, 2, SecFromTime(t));
	double ms = mjs_optnumber(J, 3, msFromTime(t));
	mjs_setdate(J, 0, MakeDate(Day(t), MakeTime(h, m, s, ms)));
}

static void Dp_setUTCHours(mjs_State *J)
{
	double t = mjs_todate(J, 0);
	double h = mjs_tonumber(J, 1);
	double m = mjs_optnumber(J, 2, HourFromTime(t));
	double s = mjs_optnumber(J, 3, SecFromTime(t));
	double ms = mjs_optnumber(J, 4, msFromTime(t));
	mjs_setdate(J, 0, MakeDate(Day(t), MakeTime(h, m, s, ms)));
}

static void Dp_setUTCDate(mjs_State *J)
{
	double t = mjs_todate(J, 0);
	double y = YearFromTime(t);
	double m = MonthFromTime(t);
	double d = mjs_tonumber(J, 1);
	mjs_setdate(J, 0, MakeDate(MakeDay(y, m, d), TimeWithinDay(t)));
}

static void Dp_setUTCMonth(mjs_State *J)
{
	double t = mjs_todate(J, 0);
	double y = YearFromTime(t);
	double m = mjs_tonumber(J, 1);
	double d = mjs_optnumber(J, 2, DateFromTime(t));
	mjs_setdate(J, 0, MakeDate(MakeDay(y, m, d), TimeWithinDay(t)));
}

static void Dp_setUTCFullYear(mjs_State *J)
{
	double t = mjs_todate(J, 0);
	double y = mjs_tonumber(J, 1);
	double m = mjs_optnumber(J, 2, MonthFromTime(t));
	double d = mjs_optnumber(J, 3, DateFromTime(t));
	mjs_setdate(J, 0, MakeDate(MakeDay(y, m, d), TimeWithinDay(t)));
}

static void Dp_toJSON(mjs_State *J)
{
	mjs_copy(J, 0);
	mjs_toprimitive(J, -1, JS_HNUMBER);
	if (mjs_isnumber(J, -1) && !isfinite(mjs_tonumber(J, -1))) {
		mjs_pushnull(J);
		return;
	}
	mjs_pop(J, 1);

	mjs_getproperty(J, 0, "toISOString");
	if (!mjs_iscallable(J, -1))
		mjs_typeerror(J, "this.toISOString is not a function");
	mjs_copy(J, 0);
	mjs_call(J, 0);
}

void jsB_initdate(mjs_State *J)
{
	J->Date_prototype->u.number = 0;

	mjs_pushobject(J, J->Date_prototype);
	{
		jsB_propf(J, "Date.prototype.valueOf", Dp_valueOf, 0);
		jsB_propf(J, "Date.prototype.toString", Dp_toString, 0);
		jsB_propf(J, "Date.prototype.toDateString", Dp_toDateString, 0);
		jsB_propf(J, "Date.prototype.toTimeString", Dp_toTimeString, 0);
		jsB_propf(J, "Date.prototype.toLocaleString", Dp_toString, 0);
		jsB_propf(J, "Date.prototype.toLocaleDateString", Dp_toDateString, 0);
		jsB_propf(J, "Date.prototype.toLocaleTimeString", Dp_toTimeString, 0);
		jsB_propf(J, "Date.prototype.toUTCString", Dp_toUTCString, 0);

		jsB_propf(J, "Date.prototype.getTime", Dp_valueOf, 0);
		jsB_propf(J, "Date.prototype.getFullYear", Dp_getFullYear, 0);
		jsB_propf(J, "Date.prototype.getUTCFullYear", Dp_getUTCFullYear, 0);
		jsB_propf(J, "Date.prototype.getMonth", Dp_getMonth, 0);
		jsB_propf(J, "Date.prototype.getUTCMonth", Dp_getUTCMonth, 0);
		jsB_propf(J, "Date.prototype.getDate", Dp_getDate, 0);
		jsB_propf(J, "Date.prototype.getUTCDate", Dp_getUTCDate, 0);
		jsB_propf(J, "Date.prototype.getDay", Dp_getDay, 0);
		jsB_propf(J, "Date.prototype.getUTCDay", Dp_getUTCDay, 0);
		jsB_propf(J, "Date.prototype.getHours", Dp_getHours, 0);
		jsB_propf(J, "Date.prototype.getUTCHours", Dp_getUTCHours, 0);
		jsB_propf(J, "Date.prototype.getMinutes", Dp_getMinutes, 0);
		jsB_propf(J, "Date.prototype.getUTCMinutes", Dp_getUTCMinutes, 0);
		jsB_propf(J, "Date.prototype.getSeconds", Dp_getSeconds, 0);
		jsB_propf(J, "Date.prototype.getUTCSeconds", Dp_getUTCSeconds, 0);
		jsB_propf(J, "Date.prototype.getMilliseconds", Dp_getMilliseconds, 0);
		jsB_propf(J, "Date.prototype.getUTCMilliseconds", Dp_getUTCMilliseconds, 0);
		jsB_propf(J, "Date.prototype.getTimezoneOffset", Dp_getTimezoneOffset, 0);

		jsB_propf(J, "Date.prototype.setTime", Dp_setTime, 1);
		jsB_propf(J, "Date.prototype.setMilliseconds", Dp_setMilliseconds, 1);
		jsB_propf(J, "Date.prototype.setUTCMilliseconds", Dp_setUTCMilliseconds, 1);
		jsB_propf(J, "Date.prototype.setSeconds", Dp_setSeconds, 2);
		jsB_propf(J, "Date.prototype.setUTCSeconds", Dp_setUTCSeconds, 2);
		jsB_propf(J, "Date.prototype.setMinutes", Dp_setMinutes, 3);
		jsB_propf(J, "Date.prototype.setUTCMinutes", Dp_setUTCMinutes, 3);
		jsB_propf(J, "Date.prototype.setHours", Dp_setHours, 4);
		jsB_propf(J, "Date.prototype.setUTCHours", Dp_setUTCHours, 4);
		jsB_propf(J, "Date.prototype.setDate", Dp_setDate, 1);
		jsB_propf(J, "Date.prototype.setUTCDate", Dp_setUTCDate, 1);
		jsB_propf(J, "Date.prototype.setMonth", Dp_setMonth, 2);
		jsB_propf(J, "Date.prototype.setUTCMonth", Dp_setUTCMonth, 2);
		jsB_propf(J, "Date.prototype.setFullYear", Dp_setFullYear, 3);
		jsB_propf(J, "Date.prototype.setUTCFullYear", Dp_setUTCFullYear, 3);

		/* ES5 */
		jsB_propf(J, "Date.prototype.toISOString", Dp_toISOString, 0);
		jsB_propf(J, "Date.prototype.toJSON", Dp_toJSON, 1);
	}
	mjs_newcconstructor(J, jsB_Date, jsB_new_Date, "Date", 0); /* 1 */
	{
		jsB_propf(J, "Date.parse", D_parse, 1);
		jsB_propf(J, "Date.UTC", D_UTC, 7);

		/* ES5 */
		jsB_propf(J, "Date.now", D_now, 0);
	}
	mjs_defglobal(J, "Date", JS_DONTENUM);
}
