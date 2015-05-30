----
INFO
----

My site: http://www.team5150.com/~andrew/
Project site: http://code.google.com/p/high-performance-cplusplus-profiler/

License: MIT

------------
WHAT IS THIS
------------

A high performance multi-threaded C++ profiler, currently for x86 (32bit)
under Windows and Linux.


----------
QUICKSTART
----------

Add Profiler.cpp to your project, and #include "Profiler.h" to any file
you wish to profile.

(OPTIONAL)
Call Profiler::detect( argc, argv ) under any OS or 
Profiler::detect( commandLine ) under Win32 non-console  apps to autodetect 
program name and startup parameters (only used for html dumps).

Use PROFILE_SCOPED() at the start of any function you wish to profile.

Use PROFILE_THREAD_SCOPED() at the entry point of any thread you wish to
profile. The main thread is instantiated automatically.

Use Profiler::reset(); to reset the profile stats at any time

Use Profiler::dump(); to generate an ASCII report of the current profile

Use Profiler::dumphtml(); to generate an HTML report of the current profile
to the current directory.


---------------
Minimal Example
---------------

#include <stdlib.h>
#include "Profiler.h"

int test() {
	PROFILE_SCOPED()
	
	int i = 1;
	while ( i < 1000 )
		i *= ( rand() & 7 ) + 1;
	return i;
}

int main( int argc, const char *argv[] ) {
	Profiler::detect( argc, argv );
	for ( int i = 0; i < 100; i++ )
		test();
	Profiler::dump();
	return 0;
}
