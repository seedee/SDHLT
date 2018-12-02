#include "cmdlib.h"
#include "messages.h"
#include "win32fix.h"
#include "log.h"
#include "hlassert.h"
#include "mathlib.h"
#include "scriplib.h"
#include "winding.h"
#include "threads.h"
#include "bspfile.h"
#include "blockmem.h"
#include "filelib.h"
#ifdef ZHLT_PARAMFILE
#include "cmdlinecfg.h"
#endif

#define DEFAULT_PARSE false
#ifdef RIPENT_TEXTURE
#define DEFAULT_TEXTUREPARSE false
#endif
#define DEFAULT_CHART false
#define DEFAULT_INFO true
#ifdef ZHLT_64BIT_FIX
#define DEFAULT_WRITEEXTENTFILE false
#endif
#ifdef ZHLT_EMBEDLIGHTMAP
#ifdef RIPENT_TEXTURE
#define DEFAULT_DELETEEMBEDDEDLIGHTMAPS false
#endif
#endif

