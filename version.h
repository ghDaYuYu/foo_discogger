﻿#pragma once
#define COMPONENT_YEAR "2021"
#define COMPONENT_VERSION_MAJOR 1
#define COMPONENT_VERSION_MINOR 0
#define COMPONENT_VERSION_PATCH 6
#define COMPONENT_VERSION_SUB_PATCH 0

#define MAKE_STRING(text) #text
#define MAKE_COMPONENT_VERSION(major,minor,patch) MAKE_STRING(major) "." MAKE_STRING(minor) "." MAKE_STRING(patch)
#define MAKE_DLL_VERSION(major,minor,patch,subpatch) MAKE_STRING(major) "." MAKE_STRING(minor) "." MAKE_STRING(patch) "." MAKE_STRING(subpatch)

//"0.1.2"
#define FOO_DISCOGGER_VERSION MAKE_COMPONENT_VERSION(COMPONENT_VERSION_MAJOR,COMPONENT_VERSION_MINOR,COMPONENT_VERSION_PATCH)
//0.1.2.3 & "0.1.2.3"
#define DLL_VERSION_NUMERIC COMPONENT_VERSION_MAJOR, COMPONENT_VERSION_MINOR, COMPONENT_VERSION_PATCH, COMPONENT_VERSION_SUB_PATCH
#define DLL_VERSION_STRING MAKE_DLL_VERSION(COMPONENT_VERSION_MAJOR,COMPONENT_VERSION_MINOR,COMPONENT_VERSION_PATCH,COMPONENT_VERSION_SUB_PATCH)

//#define DC_DB