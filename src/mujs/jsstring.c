#include "jsi.h"
#include "jsvalue.h"
#include "jsbuiltin.h"
#include "utf.h"
#include "regexp.h"

static int mjs_doregexec(mjs_State *J, Reprog *prog, const char *string, Resub *sub, int eflags)
{
	int result = mjs_regexec(prog, string, sub, eflags);
	if (result < 0)
		mjs_error(J, "regexec failed");
	return result;
}

static const char *checkstring(mjs_State *J, int idx)
{
	if (!mjs_iscoercible(J, idx))
		mjs_typeerror(J, "string function called on null or undefined");
	return mjs_tostring(J, idx);
}

int mjs_runeat(mjs_State *J, const char *s, int i)
{
	Rune rune = 0;
	while (i-- >= 0) {
		rune = *(unsigned char*)s;
		if (rune < Runeself) {
			if (rune == 0)
				return 0;
			++s;
		} else
			s += chartorune(&rune, s);
	}
	return rune;
}

const char *mjs_utfidxtoptr(const char *s, int i)
{
	Rune rune;
	while (i-- > 0) {
		rune = *(unsigned char*)s;
		if (rune < Runeself) {
			if (rune == 0)
				return NULL;
			++s;
		} else
			s += chartorune(&rune, s);
	}
	return s;
}

int mjs_utfptrtoidx(const char *s, const char *p)
{
	Rune rune;
	int i = 0;
	while (s < p) {
		if (*(unsigned char *)s < Runeself)
			++s;
		else
			s += chartorune(&rune, s);
		++i;
	}
	return i;
}

static void jsB_new_String(mjs_State *J)
{
	mjs_newstring(J, mjs_gettop(J) > 1 ? mjs_tostring(J, 1) : "");
}

static void jsB_String(mjs_State *J)
{
	mjs_pushstring(J, mjs_gettop(J) > 1 ? mjs_tostring(J, 1) : "");
}

static void Sp_toString(mjs_State *J)
{
	mjs_Object *self = mjs_toobject(J, 0);
	if (self->type != JS_CSTRING) mjs_typeerror(J, "not a string");
	mjs_pushliteral(J, self->u.s.string);
}

static void Sp_valueOf(mjs_State *J)
{
	mjs_Object *self = mjs_toobject(J, 0);
	if (self->type != JS_CSTRING) mjs_typeerror(J, "not a string");
	mjs_pushliteral(J, self->u.s.string);
}

static void Sp_charAt(mjs_State *J)
{
	char buf[UTFmax + 1];
	const char *s = checkstring(J, 0);
	int pos = mjs_tointeger(J, 1);
	Rune rune = mjs_runeat(J, s, pos);
	if (rune > 0) {
		buf[runetochar(buf, &rune)] = 0;
		mjs_pushstring(J, buf);
	} else {
		mjs_pushliteral(J, "");
	}
}

static void Sp_charCodeAt(mjs_State *J)
{
	const char *s = checkstring(J, 0);
	int pos = mjs_tointeger(J, 1);
	Rune rune = mjs_runeat(J, s, pos);
	if (rune > 0)
		mjs_pushnumber(J, rune);
	else
		mjs_pushnumber(J, NAN);
}

static void Sp_concat(mjs_State *J)
{
	int i, top = mjs_gettop(J);
	int n;
	char * volatile out;
	const char *s;

	if (top == 1)
		return;

	s = checkstring(J, 0);
	n = strlen(s);
	out = mjs_malloc(J, n + 1);
	strcpy(out, s);

	if (mjs_try(J)) {
		mjs_free(J, out);
		mjs_throw(J);
	}

	for (i = 1; i < top; ++i) {
		s = mjs_tostring(J, i);
		n += strlen(s);
		out = mjs_realloc(J, out, n + 1);
		strcat(out, s);
	}

	mjs_pushstring(J, out);
	mjs_endtry(J);
	mjs_free(J, out);
}

static void Sp_indexOf(mjs_State *J)
{
	const char *haystack = checkstring(J, 0);
	const char *needle = mjs_tostring(J, 1);
	int pos = mjs_tointeger(J, 2);
	int len = strlen(needle);
	int k = 0;
	Rune rune;
	while (*haystack) {
		if (k >= pos && !strncmp(haystack, needle, len)) {
			mjs_pushnumber(J, k);
			return;
		}
		haystack += chartorune(&rune, haystack);
		++k;
	}
	mjs_pushnumber(J, -1);
}

static void Sp_lastIndexOf(mjs_State *J)
{
	const char *haystack = checkstring(J, 0);
	const char *needle = mjs_tostring(J, 1);
	int pos = mjs_isdefined(J, 2) ? mjs_tointeger(J, 2) : (int)strlen(haystack);
	int len = strlen(needle);
	int k = 0, last = -1;
	Rune rune;
	while (*haystack && k <= pos) {
		if (!strncmp(haystack, needle, len))
			last = k;
		haystack += chartorune(&rune, haystack);
		++k;
	}
	mjs_pushnumber(J, last);
}

static void Sp_localeCompare(mjs_State *J)
{
	const char *a = checkstring(J, 0);
	const char *b = mjs_tostring(J, 1);
	mjs_pushnumber(J, strcmp(a, b));
}

static void Sp_slice(mjs_State *J)
{
	const char *str = checkstring(J, 0);
	const char *ss, *ee;
	int len = utflen(str);
	int s = mjs_tointeger(J, 1);
	int e = mjs_isdefined(J, 2) ? mjs_tointeger(J, 2) : len;

	s = s < 0 ? s + len : s;
	e = e < 0 ? e + len : e;

	s = s < 0 ? 0 : s > len ? len : s;
	e = e < 0 ? 0 : e > len ? len : e;

	if (s < e) {
		ss = mjs_utfidxtoptr(str, s);
		ee = mjs_utfidxtoptr(ss, e - s);
	} else {
		ss = mjs_utfidxtoptr(str, e);
		ee = mjs_utfidxtoptr(ss, s - e);
	}

	mjs_pushlstring(J, ss, ee - ss);
}

static void Sp_substring(mjs_State *J)
{
	const char *str = checkstring(J, 0);
	const char *ss, *ee;
	int len = utflen(str);
	int s = mjs_tointeger(J, 1);
	int e = mjs_isdefined(J, 2) ? mjs_tointeger(J, 2) : len;

	s = s < 0 ? 0 : s > len ? len : s;
	e = e < 0 ? 0 : e > len ? len : e;

	if (s < e) {
		ss = mjs_utfidxtoptr(str, s);
		ee = mjs_utfidxtoptr(ss, e - s);
	} else {
		ss = mjs_utfidxtoptr(str, e);
		ee = mjs_utfidxtoptr(ss, s - e);
	}

	mjs_pushlstring(J, ss, ee - ss);
}

static void Sp_toLowerCase(mjs_State *J)
{
	const char *src = checkstring(J, 0);
	char *dst = mjs_malloc(J, UTFmax * strlen(src) + 1);
	const char *s = src;
	char *d = dst;
	Rune rune;
	while (*s) {
		s += chartorune(&rune, s);
		rune = tolowerrune(rune);
		d += runetochar(d, &rune);
	}
	*d = 0;
	if (mjs_try(J)) {
		mjs_free(J, dst);
		mjs_throw(J);
	}
	mjs_pushstring(J, dst);
	mjs_endtry(J);
	mjs_free(J, dst);
}

static void Sp_toUpperCase(mjs_State *J)
{
	const char *src = checkstring(J, 0);
	char *dst = mjs_malloc(J, UTFmax * strlen(src) + 1);
	const char *s = src;
	char *d = dst;
	Rune rune;
	while (*s) {
		s += chartorune(&rune, s);
		rune = toupperrune(rune);
		d += runetochar(d, &rune);
	}
	*d = 0;
	if (mjs_try(J)) {
		mjs_free(J, dst);
		mjs_throw(J);
	}
	mjs_pushstring(J, dst);
	mjs_endtry(J);
	mjs_free(J, dst);
}

static int istrim(int c)
{
	return c == 0x9 || c == 0xB || c == 0xC || c == 0x20 || c == 0xA0 || c == 0xFEFF ||
		c == 0xA || c == 0xD || c == 0x2028 || c == 0x2029;
}

static void Sp_trim(mjs_State *J)
{
	const char *s, *e;
	s = checkstring(J, 0);
	while (istrim(*s))
		++s;
	e = s + strlen(s);
	while (e > s && istrim(e[-1]))
		--e;
	mjs_pushlstring(J, s, e - s);
}

static void S_fromCharCode(mjs_State *J)
{
	int i, top = mjs_gettop(J);
	Rune c;
	char *s, *p;

	s = p = mjs_malloc(J, (top-1) * UTFmax + 1);

	if (mjs_try(J)) {
		mjs_free(J, s);
		mjs_throw(J);
	}

	for (i = 1; i < top; ++i) {
		c = mjs_touint16(J, i);
		p += runetochar(p, &c);
	}
	*p = 0;
	mjs_pushstring(J, s);

	mjs_endtry(J);
	mjs_free(J, s);
}

static void Sp_match(mjs_State *J)
{
	mjs_Regexp *re;
	const char *text;
	int len;
	const char *a, *b, *c, *e;
	Resub m;

	text = checkstring(J, 0);

	if (mjs_isregexp(J, 1))
		mjs_copy(J, 1);
	else if (mjs_isundefined(J, 1))
		mjs_newregexp(J, "", 0);
	else
		mjs_newregexp(J, mjs_tostring(J, 1), 0);

	re = mjs_toregexp(J, -1);
	if (!(re->flags & JS_REGEXP_G)) {
		mjs_RegExp_prototype_exec(J, re, text);
		return;
	}

	re->last = 0;

	mjs_newarray(J);

	len = 0;
	a = text;
	e = text + strlen(text);
	while (a <= e) {
		if (mjs_doregexec(J, re->prog, a, &m, a > text ? REG_NOTBOL : 0))
			break;

		b = m.sub[0].sp;
		c = m.sub[0].ep;

		mjs_pushlstring(J, b, c - b);
		mjs_setindex(J, -2, len++);

		a = c;
		if (c - b == 0)
			++a;
	}

	if (len == 0) {
		mjs_pop(J, 1);
		mjs_pushnull(J);
	}
}

static void Sp_search(mjs_State *J)
{
	mjs_Regexp *re;
	const char *text;
	Resub m;

	text = checkstring(J, 0);

	if (mjs_isregexp(J, 1))
		mjs_copy(J, 1);
	else if (mjs_isundefined(J, 1))
		mjs_newregexp(J, "", 0);
	else
		mjs_newregexp(J, mjs_tostring(J, 1), 0);

	re = mjs_toregexp(J, -1);

	if (!mjs_doregexec(J, re->prog, text, &m, 0))
		mjs_pushnumber(J, mjs_utfptrtoidx(text, m.sub[0].sp));
	else
		mjs_pushnumber(J, -1);
}

static void Sp_replace_regexp(mjs_State *J)
{
	mjs_Regexp *re;
	const char *source, *s, *r;
	mjs_Buffer *sb = NULL;
	int n, x;
	Resub m;

	source = checkstring(J, 0);
	re = mjs_toregexp(J, 1);

	if (mjs_doregexec(J, re->prog, source, &m, 0)) {
		mjs_copy(J, 0);
		return;
	}

	re->last = 0;

loop:
	s = m.sub[0].sp;
	n = m.sub[0].ep - m.sub[0].sp;

	if (mjs_iscallable(J, 2)) {
		mjs_copy(J, 2);
		mjs_pushundefined(J);
		for (x = 0; m.sub[x].sp; ++x) /* arg 0..x: substring and subexps that matched */
			mjs_pushlstring(J, m.sub[x].sp, m.sub[x].ep - m.sub[x].sp);
		mjs_pushnumber(J, s - source); /* arg x+2: offset within search string */
		mjs_copy(J, 0); /* arg x+3: search string */
		mjs_call(J, 2 + x);
		r = mjs_tostring(J, -1);
		mjs_putm(J, &sb, source, s);
		mjs_puts(J, &sb, r);
		mjs_pop(J, 1);
	} else {
		r = mjs_tostring(J, 2);
		mjs_putm(J, &sb, source, s);
		while (*r) {
			if (*r == '$') {
				switch (*(++r)) {
				case 0: --r; /* end of string; back up */
				/* fallthrough */
				case '$': mjs_putc(J, &sb, '$'); break;
				case '`': mjs_putm(J, &sb, source, s); break;
				case '\'': mjs_puts(J, &sb, s + n); break;
				case '&':
					mjs_putm(J, &sb, s, s + n);
					break;
				case '0': case '1': case '2': case '3': case '4':
				case '5': case '6': case '7': case '8': case '9':
					x = *r - '0';
					if (r[1] >= '0' && r[1] <= '9')
						x = x * 10 + *(++r) - '0';
					if (x > 0 && x < m.nsub) {
						mjs_putm(J, &sb, m.sub[x].sp, m.sub[x].ep);
					} else {
						mjs_putc(J, &sb, '$');
						if (x > 10) {
							mjs_putc(J, &sb, '0' + x / 10);
							mjs_putc(J, &sb, '0' + x % 10);
						} else {
							mjs_putc(J, &sb, '0' + x);
						}
					}
					break;
				default:
					mjs_putc(J, &sb, '$');
					mjs_putc(J, &sb, *r);
					break;
				}
				++r;
			} else {
				mjs_putc(J, &sb, *r++);
			}
		}
	}

	if (re->flags & JS_REGEXP_G) {
		source = m.sub[0].ep;
		if (n == 0) {
			if (*source)
				mjs_putc(J, &sb, *source++);
			else
				goto end;
		}
		if (!mjs_doregexec(J, re->prog, source, &m, REG_NOTBOL))
			goto loop;
	}

end:
	mjs_puts(J, &sb, s + n);
	mjs_putc(J, &sb, 0);

	if (mjs_try(J)) {
		mjs_free(J, sb);
		mjs_throw(J);
	}
	mjs_pushstring(J, sb ? sb->s : "");
	mjs_endtry(J);
	mjs_free(J, sb);
}

static void Sp_replace_string(mjs_State *J)
{
	const char *source, *needle, *s, *r;
	mjs_Buffer *sb = NULL;
	int n;

	source = checkstring(J, 0);
	needle = mjs_tostring(J, 1);

	s = strstr(source, needle);
	if (!s) {
		mjs_copy(J, 0);
		return;
	}
	n = strlen(needle);

	if (mjs_iscallable(J, 2)) {
		mjs_copy(J, 2);
		mjs_pushundefined(J);
		mjs_pushlstring(J, s, n); /* arg 1: substring that matched */
		mjs_pushnumber(J, s - source); /* arg 2: offset within search string */
		mjs_copy(J, 0); /* arg 3: search string */
		mjs_call(J, 3);
		r = mjs_tostring(J, -1);
		mjs_putm(J, &sb, source, s);
		mjs_puts(J, &sb, r);
		mjs_puts(J, &sb, s + n);
		mjs_putc(J, &sb, 0);
		mjs_pop(J, 1);
	} else {
		r = mjs_tostring(J, 2);
		mjs_putm(J, &sb, source, s);
		while (*r) {
			if (*r == '$') {
				switch (*(++r)) {
				case 0: --r; /* end of string; back up */
				/* fallthrough */
				case '$': mjs_putc(J, &sb, '$'); break;
				case '&': mjs_putm(J, &sb, s, s + n); break;
				case '`': mjs_putm(J, &sb, source, s); break;
				case '\'': mjs_puts(J, &sb, s + n); break;
				default: mjs_putc(J, &sb, '$'); mjs_putc(J, &sb, *r); break;
				}
				++r;
			} else {
				mjs_putc(J, &sb, *r++);
			}
		}
		mjs_puts(J, &sb, s + n);
		mjs_putc(J, &sb, 0);
	}

	if (mjs_try(J)) {
		mjs_free(J, sb);
		mjs_throw(J);
	}
	mjs_pushstring(J, sb ? sb->s : "");
	mjs_endtry(J);
	mjs_free(J, sb);
}

static void Sp_replace(mjs_State *J)
{
	if (mjs_isregexp(J, 1))
		Sp_replace_regexp(J);
	else
		Sp_replace_string(J);
}

static void Sp_split_regexp(mjs_State *J)
{
	mjs_Regexp *re;
	const char *text;
	int limit, len, k;
	const char *p, *a, *b, *c, *e;
	Resub m;

	text = checkstring(J, 0);
	re = mjs_toregexp(J, 1);
	limit = mjs_isdefined(J, 2) ? mjs_tointeger(J, 2) : 1 << 30;

	mjs_newarray(J);
	len = 0;

	e = text + strlen(text);

	/* splitting the empty string */
	if (e == text) {
		if (mjs_doregexec(J, re->prog, text, &m, 0)) {
			if (len == limit) return;
			mjs_pushliteral(J, "");
			mjs_setindex(J, -2, 0);
		}
		return;
	}

	p = a = text;
	while (a < e) {
		if (mjs_doregexec(J, re->prog, a, &m, a > text ? REG_NOTBOL : 0))
			break; /* no match */

		b = m.sub[0].sp;
		c = m.sub[0].ep;

		/* empty string at end of last match */
		if (b == p) {
			++a;
			continue;
		}

		if (len == limit) return;
		mjs_pushlstring(J, p, b - p);
		mjs_setindex(J, -2, len++);

		for (k = 1; k < m.nsub; ++k) {
			if (len == limit) return;
			mjs_pushlstring(J, m.sub[k].sp, m.sub[k].ep - m.sub[k].sp);
			mjs_setindex(J, -2, len++);
		}

		a = p = c;
	}

	if (len == limit) return;
	mjs_pushstring(J, p);
	mjs_setindex(J, -2, len);
}

static void Sp_split_string(mjs_State *J)
{
	const char *str = checkstring(J, 0);
	const char *sep = mjs_tostring(J, 1);
	int limit = mjs_isdefined(J, 2) ? mjs_tointeger(J, 2) : 1 << 30;
	int i, n;

	mjs_newarray(J);

	n = strlen(sep);

	/* empty string */
	if (n == 0) {
		Rune rune;
		for (i = 0; *str && i < limit; ++i) {
			n = chartorune(&rune, str);
			mjs_pushlstring(J, str, n);
			mjs_setindex(J, -2, i);
			str += n;
		}
		return;
	}

	for (i = 0; str && i < limit; ++i) {
		const char *s = strstr(str, sep);
		if (s) {
			mjs_pushlstring(J, str, s-str);
			mjs_setindex(J, -2, i);
			str = s + n;
		} else {
			mjs_pushstring(J, str);
			mjs_setindex(J, -2, i);
			str = NULL;
		}
	}
}

static void Sp_split(mjs_State *J)
{
	if (mjs_isundefined(J, 1)) {
		mjs_newarray(J);
		mjs_copy(J, 0);
		mjs_setindex(J, -2, 0);
	} else if (mjs_isregexp(J, 1)) {
		Sp_split_regexp(J);
	} else {
		Sp_split_string(J);
	}
}

void jsB_initstring(mjs_State *J)
{
	J->String_prototype->u.s.string = "";
	J->String_prototype->u.s.length = 0;

	mjs_pushobject(J, J->String_prototype);
	{
		jsB_propf(J, "String.prototype.toString", Sp_toString, 0);
		jsB_propf(J, "String.prototype.valueOf", Sp_valueOf, 0);
		jsB_propf(J, "String.prototype.charAt", Sp_charAt, 1);
		jsB_propf(J, "String.prototype.charCodeAt", Sp_charCodeAt, 1);
		jsB_propf(J, "String.prototype.concat", Sp_concat, 0); /* 1 */
		jsB_propf(J, "String.prototype.indexOf", Sp_indexOf, 1);
		jsB_propf(J, "String.prototype.lastIndexOf", Sp_lastIndexOf, 1);
		jsB_propf(J, "String.prototype.localeCompare", Sp_localeCompare, 1);
		jsB_propf(J, "String.prototype.match", Sp_match, 1);
		jsB_propf(J, "String.prototype.replace", Sp_replace, 2);
		jsB_propf(J, "String.prototype.search", Sp_search, 1);
		jsB_propf(J, "String.prototype.slice", Sp_slice, 2);
		jsB_propf(J, "String.prototype.split", Sp_split, 2);
		jsB_propf(J, "String.prototype.substring", Sp_substring, 2);
		jsB_propf(J, "String.prototype.toLowerCase", Sp_toLowerCase, 0);
		jsB_propf(J, "String.prototype.toLocaleLowerCase", Sp_toLowerCase, 0);
		jsB_propf(J, "String.prototype.toUpperCase", Sp_toUpperCase, 0);
		jsB_propf(J, "String.prototype.toLocaleUpperCase", Sp_toUpperCase, 0);

		/* ES5 */
		jsB_propf(J, "String.prototype.trim", Sp_trim, 0);
	}
	mjs_newcconstructor(J, jsB_String, jsB_new_String, "String", 0); /* 1 */
	{
		jsB_propf(J, "String.fromCharCode", S_fromCharCode, 0); /* 1 */
	}
	mjs_defglobal(J, "String", JS_DONTENUM);
}
