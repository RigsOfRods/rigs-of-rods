/* 
 * Copyright (C) 2004-2009 Georgy Yunaev gyunaev@ulduzsoft.com
 *
 * This library is free software; you can redistribute it and/or modify it 
 * under the terms of the GNU Lesser General Public License as published by 
 * the Free Software Foundation; either version 2 of the License, or (at your 
 * option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT 
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public 
 * License for more details.
 */

#include "RoRPrerequisites.h"

#if !defined (_WIN32)
	#include "config_linux.h"
	#include <stdio.h>
	#include <stdarg.h>
	#include <unistd.h>
	#include <string.h>
	#include <stdlib.h>
	#include <sys/stat.h>
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netdb.h>
	#include <arpa/inet.h>	
	#include <netinet/in.h>
	#include <fcntl.h>
	#include <errno.h>
	#include <ctype.h>
	#include <time.h>
#else
	#include <winsock2.h>
	#include <ws2tcpip.h>
	#include <windows.h>
	#include <time.h>
	#include <stdio.h>
	#include <stdarg.h>
	#include <string.h>
	#include <stdlib.h>
	#include <sys/stat.h>

	#define inline
	#define snprintf			_snprintf
	#define vsnprintf			_vsnprintf
#endif // _WIN32


#if defined (ENABLE_THREADS)

#include <mutex>

typedef std::mutex port_mutex_t

static inline int libirc_mutex_init (port_mutex_t * mutex)
{
	return 0;
}


static inline void libirc_mutex_destroy (port_mutex_t * mutex)
{
}

static inline void libirc_mutex_lock (port_mutex_t * mutex)
{
	mutex.lock();
}


static inline void libirc_mutex_unlock (port_mutex_t * mutex)
{
	mutex.unlock();
}

#else

	typedef void *	port_mutex_t;

	static inline int libirc_mutex_init (port_mutex_t * mutex) { return 0; }
	static inline void libirc_mutex_destroy (port_mutex_t * mutex) {}
	static inline void libirc_mutex_lock (port_mutex_t * mutex) {}
	static inline void libirc_mutex_unlock (port_mutex_t * mutex) {}

#endif // ENABLE_THREADS


/*
 * Stub for WIN32 dll to initialize winsock API
 */
#if defined (_USRDLL)
BOOL WINAPI DllMain (HINSTANCE hinstDll, DWORD fdwReason, LPVOID lpvReserved)
{
	WORD wVersionRequested = MAKEWORD (1, 1);
    WSADATA wsaData;

	switch(fdwReason)
	{
		case DLL_PROCESS_ATTACH:
			if ( WSAStartup (wVersionRequested, &wsaData) != 0 )
				return FALSE;

			DisableThreadLibraryCalls (hinstDll);
			break;

		case DLL_PROCESS_DETACH:
			WSACleanup();
			break;
	}

	return TRUE;
}
#endif
