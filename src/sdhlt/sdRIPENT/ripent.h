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
#include "cmdlinecfg.h"

#define DEFAULT_PARSE false
#define DEFAULT_TEXTUREPARSE false
#define DEFAULT_CHART true //seedee
#define DEFAULT_INFO true
#define DEFAULT_WRITEEXTENTFILE false
#define DEFAULT_DELETEEMBEDDEDLIGHTMAPS false

