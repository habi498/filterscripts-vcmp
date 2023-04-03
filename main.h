#ifndef MAIN_H
#define MAIN_H

#define MAX_FILTER_SCRIPTS 16
#define SAFE_DELETE(p)	{ if (p) { delete (p); (p) = NULL; } }
#include "PluginAPI.h"
#include "SQImports.h"
#include "stdio.h"
#include "filterscripts.h"
#include "ReadCFG.h"
#include "string.h"
#include "hook.h"
#endif

