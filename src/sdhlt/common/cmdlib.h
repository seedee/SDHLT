#ifndef CMDLIB_H__
#define CMDLIB_H__

#if _MSC_VER >= 1000
#pragma once
#endif

//#define MODIFICATIONS_STRING "Submit detailed bug reports to (zoner@gearboxsoftware.com)\n"
//#define MODIFICATIONS_STRING "Submit detailed bug reports to (merlinis@bigpond.net.au)\n"
//#define MODIFICATIONS_STRING "Submit detailed bug reports to (amckern@yahoo.com)\n"
//#define MODIFICATIONS_STRING "Submit detailed bug reports to (vluzacn@163.com)\n" //--vluzacn
#define MODIFICATIONS_STRING "Submit detailed bug reports to (github.com/seedee/SDHLT/issues)\n"

#ifdef _DEBUG
#define ZHLT_VERSIONSTRING "v3.4 dbg"
#else
#define ZHLT_VERSIONSTRING "v3.4"
#endif

#define HACK_VERSIONSTRING "VL34" //--vluzacn

#define SDHLT_VERSIONSTRING "v1.2.0"

#if !defined (SDHLCSG) && !defined (SDHLBSP) && !defined (SDHLVIS) && !defined (SDHLRAD) && !defined (SDRIPENT) //seedee
#error "You must define one of these in the settings of each project: SDHLCSG, SDHLBSP, SDHLVIS, SDHLRAD, SDRIPENT. The most likely cause is that you didn't load the project from the .sln file."
#endif
#if !defined (VERSION_32BIT) && !defined (VERSION_64BIT) && !defined (VERSION_LINUX) && !defined (VERSION_OTHER) //--vluzacn
#error "You must define one of these in the settings of each project: VERSION_32BIT, VERSION_64BIT, VERSION_LINUX, VERSION_OTHER. The most likely cause is that you didn't load the project from the .sln file."
#endif

#ifdef VERSION_32BIT
#define PLATFORM_VERSIONSTRING "32-bit"
#define PLATFORM_CAN_CALC_EXTENT
#endif
#ifdef VERSION_64BIT
#define PLATFORM_VERSIONSTRING "64-bit"
#define PLATFORM_CAN_CALC_EXTENT
#endif
#ifdef VERSION_LINUX
#define PLATFORM_VERSIONSTRING "linux"
#define PLATFORM_CAN_CALC_EXTENT
#endif
#ifdef VERSION_OTHER
#define PLATFORM_VERSIONSTRING "???"
#endif

//=====================================================================
// AJM: Different features of the tools can be undefined here
//      these are not officially beta tested, but seem to work okay

// ZHLT_* features are spread across more than one tool. Hence, changing
//      one of these settings probably means recompiling the whole set

	#ifdef SYSTEM_WIN32
#define RIPENT_PAUSE //--vluzacn
	#endif

// tool specific settings below only mean a recompile of the tool affected

	#ifdef SYSTEM_WIN32
#define HLCSG_GAMETEXTMESSAGE_UTF8 //--vluzacn
	#endif

//=====================================================================

#if _MSC_VER <1400
#define strcpy_s strcpy //--vluzacn
#define sprintf_s sprintf //--vluzacn
#endif
#if _MSC_VER >= 1400
#pragma warning(disable: 4996)
#endif

#ifdef __MINGW32__
#include <io.h>
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if 0 //--vluzacn
// AJM: gnu compiler fix
#ifdef __GNUC__
#define _alloca __builtin_alloca
#define alloca __builtin_alloca
#endif
#endif

#include "win32fix.h"
#include "mathtypes.h"

#ifdef SYSTEM_WIN32
#pragma warning(disable: 4127)                      // conditional expression is constant
#pragma warning(disable: 4115)                      // named type definition in parentheses
#pragma warning(disable: 4244)                      // conversion from 'type' to type', possible loss of data
// AJM
#pragma warning(disable: 4786)                      // identifier was truncated to '255' characters in the browser information
#pragma warning(disable: 4305)                      // truncation from 'const double' to 'float'
#pragma warning(disable: 4800)                     // forcing value to bool 'true' or 'false' (performance warning)
#endif


#ifdef STDC_HEADERS
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>
#include <stdarg.h>
#include <limits.h>
#endif

#include <stdint.h> //--vluzacn

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef ZHLT_NETVIS
#include "c2cpp.h"
#endif

#ifdef SYSTEM_WIN32
#define SYSTEM_SLASH_CHAR  '\\'
#define SYSTEM_SLASH_STR   "\\"
#endif
#ifdef SYSTEM_POSIX
#define SYSTEM_SLASH_CHAR  '/'
#define SYSTEM_SLASH_STR   "/"
#endif

// the dec offsetof macro doesn't work very well...
#define myoffsetof(type,identifier) ((size_t)&((type*)0)->identifier)
#define sizeofElement(type,identifier) (sizeof((type*)0)->identifier)

#ifdef SYSTEM_POSIX
extern char*    strupr(char* string);
extern char*    strlwr(char* string);
#endif
extern const char* stristr(const char* const string, const char* const substring);
extern bool CDECL FORMAT_PRINTF(3,4) safe_snprintf(char* const dest, const size_t count, const char* const args, ...);
extern bool     safe_strncpy(char* const dest, const char* const src, const size_t count);
extern bool     safe_strncat(char* const dest, const char* const src, const size_t count);
extern bool     TerminatedString(const char* buffer, const int size);

extern char*    FlipSlashes(char* string);

extern double   I_FloatTime();

extern int      CheckParm(char* check);

extern void     DefaultExtension(char* path, const char* extension);
extern void     DefaultPath(char* path, char* basepath);
extern void     StripFilename(char* path);
extern void     StripExtension(char* path);

extern void     ExtractFile(const char* const path, char* dest);
extern void     ExtractFilePath(const char* const path, char* dest);
extern void     ExtractFileBase(const char* const path, char* dest);
extern void     ExtractFileExtension(const char* const path, char* dest);

extern short    BigShort(short l);
extern short    LittleShort(short l);
extern int      BigLong(int l);
extern int      LittleLong(int l);
extern float    BigFloat(float l);
extern float    LittleFloat(float l);

#endif //CMDLIB_H__
