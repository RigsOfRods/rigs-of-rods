/*
 * File JSON.h part of the SimpleJSON Library - http://mjpa.in/json
 *
 * Copyright (C) 2010 Mike Anchor
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef _JSON_H_
#define _JSON_H_

// Win32 incompatibilities
#if defined(WIN32) && !defined(__GNUC__)
	#define wcsncasecmp _wcsnicmp
	static inline bool isnan(double x) { return x != x; }
	static inline bool isinf(double x) { return !isnan(x) && isnan(x - x); }
#endif

#include <vector>
#include <string>
#include <map>

// Linux compile fix - from quaker66
#ifdef __GNUC__
	#include <cstring>
	#include <cstdlib>
#endif

// Mac compile fixes - from quaker66
#if defined(__APPLE__) || (defined(WIN32) && defined(__GNUC__))
	#include <wctype.h>
	#include <wchar.h>
	#include <algorithm>
	
	static inline int wcsncasecmp (const wchar_t *s1, const wchar_t *s2, size_t n)
	{
		if (!wcsncmp(s1, s2, n))
			return 0;

		std::wstring ws(s1), wws(s2);
		transform(ws.begin(), ws.end(), ws.begin(), towlower);
		transform(wws.begin(), wws.end(), wws.begin(), towlower);
		return wcsncmp(ws.c_str(), wws.c_str(), n);
	}
#endif

// Custom types
class JSONValue;
typedef std::vector<JSONValue*> JSONArray;
typedef std::map<std::wstring, JSONValue*> JSONObject;

#include "JSONValue.h"

class JSON
{
	friend class JSONValue;
	
	public:
		static JSONValue* Parse(const char *data);
		static JSONValue* Parse(const wchar_t *data);
		static std::wstring Stringify(const JSONValue *value);
	protected:
		static bool SkipWhitespace(const wchar_t **data);
		static bool ExtractString(const wchar_t **data, std::wstring &str);
		static double ParseInt(const wchar_t **data);
		static double ParseDecimal(const wchar_t **data);
	private:
		JSON();
};

#endif
