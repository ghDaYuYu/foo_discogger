﻿#pragma once
#define COMPONENT_NAME "foo_discogger"
#define COMPONENT_YEAR "2023"
#define COMPONENT_VERSION_MAJOR 1
#define COMPONENT_VERSION_MINOR 0
#define COMPONENT_VERSION_PATCH 19
#define COMPONENT_VERSION_SUB_PATCH 0

//#define FIX_VER

#define MAKE_STRING(text) #text
#define MAKE_COMPONENT_VERSION(major,minor,patch) MAKE_STRING(major) "." MAKE_STRING(minor) "." MAKE_STRING(patch)
#define MAKE_DLL_VERSION(major,minor,patch,subpatch) MAKE_STRING(major) "." MAKE_STRING(minor) "." MAKE_STRING(patch) "." MAKE_STRING(subpatch)
#define MAKE_API_SDK_VERSION(sdk_ver, sdk_target) MAKE_STRING(sdk_ver) " " MAKE_STRING(sdk_target)

//1.0.18 or 1.0.18.1
#ifdef FIX_VER
#define FOO_DISCOGGER_VERSION MAKE_DLL_VERSION(COMPONENT_VERSION_MAJOR,COMPONENT_VERSION_MINOR,COMPONENT_VERSION_PATCH,COMPONENT_VERSION_SUB_PATCH)
#else
#define FOO_DISCOGGER_VERSION MAKE_COMPONENT_VERSION(COMPONENT_VERSION_MAJOR,COMPONENT_VERSION_MINOR,COMPONENT_VERSION_PATCH)
#endif

//0.1.2.3 & "0.1.2.3"
#define DLL_VERSION_NUMERIC COMPONENT_VERSION_MAJOR, COMPONENT_VERSION_MINOR, COMPONENT_VERSION_PATCH, COMPONENT_VERSION_SUB_PATCH
#define DLL_VERSION_STRING MAKE_DLL_VERSION(COMPONENT_VERSION_MAJOR,COMPONENT_VERSION_MINOR,COMPONENT_VERSION_PATCH,COMPONENT_VERSION_SUB_PATCH)

//fb2k version
#define PLUGIN_FB2K_SDK MAKE_API_SDK_VERSION(FOOBAR2000_SDK_VERSION, FOOBAR2000_TARGET_VERSION)
