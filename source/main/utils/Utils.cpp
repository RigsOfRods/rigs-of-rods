/*
This source file is part of Rigs of Rods
Copyright 2005-2012 Pierre-Michel Ricordel
Copyright 2007-2012 Thomas Fischer

For more information, see http://www.rigsofrods.com/

Rigs of Rods is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License version 3, as
published by the Free Software Foundation.

Rigs of Rods is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Rigs of Rods.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "Utils.h"

#include "Ogre.h"
#include "rornet.h"
#include "RoRVersion.h"
#include "SHA1.h"

#ifndef WIN32
#include <iconv.h>
#endif //WIN32

using namespace Ogre;

String hexdump(void *pAddressIn, long  lSize)
{
	char szBuf[100];
	long lIndent = 1;
	long lOutLen, lIndex, lIndex2, lOutLen2;
	long lRelPos;
	struct { char *pData; unsigned long lSize; } buf;
	unsigned char *pTmp,ucTmp;
	unsigned char *pAddress = (unsigned char *)pAddressIn;

	buf.pData   = (char *)pAddress;
	buf.lSize   = lSize;

	String result = "";
	
	while (buf.lSize > 0)
	{
		pTmp     = (unsigned char *)buf.pData;
		lOutLen  = (int)buf.lSize;
		if (lOutLen > 16)
		  lOutLen = 16;

		// create a 64-character formatted output line:
		sprintf(szBuf, " >                            "
					 "                      "
					 "    %08lX", (long unsigned int)(pTmp-pAddress));
		lOutLen2 = lOutLen;

		for (lIndex = 1+lIndent, lIndex2 = 53-15+lIndent, lRelPos = 0;
		  lOutLen2;
		  lOutLen2--, lIndex += 2, lIndex2++
			)
		{
			ucTmp = *pTmp++;

			sprintf(szBuf + lIndex, "%02X ", (unsigned short)ucTmp);
			if (!isprint(ucTmp))  ucTmp = '.'; // nonprintable char
			szBuf[lIndex2] = ucTmp;

			if (!(++lRelPos & 3))     // extra blank after 4 bytes
			{  lIndex++; szBuf[lIndex+2] = ' '; }
		}

		if (!(lRelPos & 3)) lIndex--;

		szBuf[lIndex  ]   = '<';
		szBuf[lIndex+1]   = ' ';

		result += String(szBuf) + "\n";

		buf.pData   += lOutLen;
		buf.lSize   -= lOutLen;
	}
	return result;
}

UTFString tryConvertUTF(const char *buffer)
{
	try
	{
		UTFString s = UTFString(buffer);
		if (s.empty())
			s = UTFString("(UTF conversion error 1)");
		return s;

	} catch(...)
	{
		return UTFString("(UTF conversion error 2)");
	}
	//return UTFString("(UTF conversion error 3)");
}

UTFString formatBytes(double bytes)
{
	wchar_t tmp[128] = L"";
	const wchar_t *si_prefix[] = { L"B", L"KB", L"MB", L"GB", L"TB", L"EB", L"ZB", L"YB" };
	int base = 1024;
	int c = std::min((int)(log(bytes)/log((float)base)), (int)sizeof(si_prefix) - 1);
	swprintf(tmp, 128, L"%1.2f %ls", bytes / pow((float)base, c), si_prefix[c]);
	return UTFString(tmp);
}

// replace non-ASCII characters with underscores to prevent std::string problems
String getASCIIFromCharString(char *str, int maxlen)
{
	char *ptr = str;
	for (int i=0; i < maxlen; i++, ptr++)
	{
		if (*ptr == 0) break;
		if (*ptr < 32 || *ptr > 126)
		{
			*ptr = 95;
		}
	}
	str[maxlen] = 0;
	return std::string(str);
}

// replace non-ASCII characters with underscores to prevent std::string problems
String getASCIIFromOgreString(String s, int maxlen)
{
	char str[1024] = "";
	strncpy(str, s.c_str(), 1023);
	char *ptr = str;
	for (int i=0; i < maxlen; i++, ptr++)
	{
		if (*ptr == 0) break;
		if (*ptr < 32 || *ptr > 126)
		{
			*ptr = 95;
		}
	}
	str[maxlen] = 0;
	return std::string(str);
}

int getTimeStamp()
{
	return (int)time(NULL); //this will overflow in 2038
}


String getVersionString(bool multiline)
{
	char tmp[1024] = "";
	if (multiline)
	{
		sprintf(tmp, "Rigs of Rods\n"
			" version: %s\n"
			" revision: %s\n"
			" full revision: %s\n"
			" protocol version: %s\n"
			" build time: %s, %s\n"
			, ROR_VERSION_STRING, SVN_REVISION, SVN_ID, RORNET_VERSION, __DATE__, __TIME__);
	} else
	{
		sprintf(tmp, "Rigs of Rods version %s, revision: %s, protocol version: %s, build time: %s, %s", ROR_VERSION_STRING, SVN_REVISION, RORNET_VERSION, __DATE__, __TIME__);
	}

	return String(tmp);
}

bool fileExists(const char *filename)
{
	// be careful about what you use here...
	FILE *f = fopen(filename, "r");
	if (!f)
		return false;
	fclose(f);
	return true;
}

bool folderExists(const char *pathname)
{
#ifdef WIN32
	DWORD dwAttrib = GetFileAttributesA(pathname);
	return (dwAttrib != INVALID_FILE_ATTRIBUTES &&  (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
#else
	struct stat st;
	return (stat(pathname, &st) == 0);
#endif
}

int isPowerOfTwo (unsigned int x)
{
  return ((x != 0) && ((x & (~x + 1)) == x));
}

// same as: return preg_replace("/[^A-Za-z0-9-_.]/", "_", $s);
String stripNonASCII(String s)
{
	char filename[9046] = "";
	sprintf(filename, "%s", s.c_str());
	for (size_t i=0;i<s.size(); i++)
	{
		bool replace = true;
		if     (filename[i] >= 48 && filename[i] <= 57) replace = false; // 0-9
		else if (filename[i] >= 65 && filename[i] <= 90) replace = false; // A-Z
		else if (filename[i] >= 97 && filename[i] <= 122) replace = false; // a-z
		else if (filename[i] == 45 || filename[i] == 95 || filename[i] == 46) replace = false; // -_.
		if (replace)
			filename[i]='_';
	}
	return String(filename);
}
	

bool compareCaseInsensitive(std::string strFirst, std::string strSecond)
{
	// Convert both strings to upper case by transfrom() before compare.
	transform(strFirst.begin(), strFirst.end(), strFirst.begin(), toupper);
	transform(strSecond.begin(), strSecond.end(), strSecond.begin(), toupper);
	return (strFirst == strSecond);
}


AxisAlignedBox getWorldAABB(SceneNode* node)
{
	AxisAlignedBox aabb;

	// merge with attached objects
	for (int i=0; i<node->numAttachedObjects(); ++i)
	{
		MovableObject* o = node->getAttachedObject(i);
		aabb.merge(o->getWorldBoundingBox(true));
	}

	// merge with child nodes
	for (int i=0; i<node->numChildren(); ++i)
	{
		SceneNode* child = static_cast<SceneNode*>(node->getChild(i));
		aabb.merge( getWorldAABB(child) );
	}

	return aabb;
}

void fixRenderWindowIcon (RenderWindow *rw)
{
#ifndef ROR_EMBEDDED
#ifdef WIN32
	// only in non-embedded mode
	size_t hWnd = 0;
	rw->getCustomAttribute("WINDOW", &hWnd);

	char buf[MAX_PATH];
	::GetModuleFileNameA(0, (LPCH)&buf, MAX_PATH);

	HINSTANCE instance = ::GetModuleHandleA(buf);
	HICON hIcon = ::LoadIconA(instance, MAKEINTRESOURCEA(101));
	if (hIcon)
	{
		::SendMessageA((HWND)hWnd, WM_SETICON, 1, (LPARAM)hIcon);
		::SendMessageA((HWND)hWnd, WM_SETICON, 0, (LPARAM)hIcon);
	}
#endif // WIN32
#endif //ROR_EMBEDDED
}

UTFString ANSI_TO_UTF(const String source)
{
	return UTFString(ANSI_TO_WCHAR(source)); // UTF converts from wstring
}

// TODO: Make it bulletproof! <Bad Ptr> e.g kills this
std::wstring ANSI_TO_WCHAR(const String source)
{
#ifdef WIN32
	const char* srcPtr = source.c_str();
	int tmpSize = MultiByteToWideChar( CP_ACP, 0, srcPtr, -1, 0, 0 );
	WCHAR* tmpBuff = new WCHAR [ tmpSize + 1 ];
	MultiByteToWideChar( CP_ACP, 0, srcPtr, -1, tmpBuff, tmpSize );
	std::wstring ret = tmpBuff;
	delete[] tmpBuff;
	return ret;
#if 0
	// does not make much of a difference
	int retval = MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, _source.c_str(), -1, NULL, 0);
	if (!SUCCEEDED(retval))
	{
		return std::wstring(L"ERR");
	}
	WCHAR* wstr = new WCHAR [ retval + 1 ];
	if (wstr == NULL)
	{
		return std::wstring(L"ERR");
	}
	std::fill_n(wstr, retval+1, '\0' );
	retval = MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, _source.c_str(), -1, wstr, retval);
	if (!SUCCEEDED(retval))
	{
		delete[] wstr;
		return std::wstring(L"ERR");
	}
	std::wstring ret = wstr;
	delete[] wstr;
	return ret;
#endif
#else
	// TODO: GET THIS WORKING
	/*
	const char* srcPtr = source.c_str();
	iconv_t icv = iconv_open("ASCII", "UTF-8");
	if ( icv == (iconv_t) -1 )
	{
		return std::wstring(L"ERR1");
	}

	char *inpbuf    = const_cast<char *>(source.c_str());
	size_t inbytes  = source.size();
	size_t outbytes = inbytes;
	size_t nread    = 0;
	char *outbuf    = (char *)calloc((outbytes*4+1)*sizeof(char), 1);

	size_t res = iconv(icv, &inpbuf, &inbytes, &outbuf, &outbytes);
	if (res == (size_t) -1)
	{
		//free(outbuf);
		return std::wstring(L"ERR2");
	}
	iconv_close(icv);
	//free(outbuf);
	*/

	// hacky!
	std::wstring str2(source.length(), L' '); // Make room for characters
	std::copy(source.begin(), source.end(), str2.begin());
	return str2;
#endif // WIN32
}

void trimUTFString( UTFString &str, bool left, bool right)
{
	static const String delims = " \t\r";
	if (right)
		str.erase(str.find_last_not_of(delims)+1); // trim right
	if (left)
		str.erase(0, str.find_first_not_of(delims)); // trim left
}

Real Round(Real value, unsigned short ndigits /* = 0 */)
{
	Real f = 1.0f;

	while (ndigits--)
		f = f * 10.0f;

	value *= f;

	if (value >= 0.0f)
		value = std::floor(value + 0.5f);
	else
		value = std::ceil(value - 0.5f);

	value /= f;

	return value;
}

void generateHashFromDataStream(DataStreamPtr &ds, Ogre::String &hash)
{
	size_t location = ds->tell();
	// copy whole file into a buffer
	uint8_t *buf = 0;
	ds->seek(0); // from start
	// alloc buffer
	uint32_t bufSize = ds->size();
	buf = (uint8_t *)malloc(bufSize+1);
	// read into buffer
	ds->read(buf, bufSize);

	// and build the hash over it
	char hash_result[250];
	memset(hash_result, 0, 249);

	{
		RoR::CSHA1 sha1;
		sha1.UpdateHash(buf, bufSize);
		sha1.Final();
		sha1.ReportHash(hash_result, RoR::CSHA1::REPORT_HEX_SHORT);
	}

	// revert DS to previous position
	ds->seek(location);
	// release memory
	free(buf);
	buf = 0;
	
	hash = String(hash_result);
}

void generateHashFromFile(String filename, Ogre::String &hash)
{
	// no exception handling in here
	DataStreamPtr ds = ResourceGroupManager::getSingleton().openResource(filename, ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);
	generateHashFromDataStream(ds, hash);
}
