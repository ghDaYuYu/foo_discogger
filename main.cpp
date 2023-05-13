#include "stdafx.h"

// Declaration of your component's version information
// Since foobar2000 v1.0 having at least one of these in your DLL is mandatory to let the troubleshooter tell different versions of your component apart.
// Note that it is possible to declare multiple components within one DLL, but it's strongly recommended to keep only one declaration per DLL.
// As for 1.1, the version numbers are used by the component update finder to find updates; for that to work, you must have ONLY ONE declaration per DLL.
// If there are multiple declarations, the component is assumed to be outdated and a version number of "0" is assumed, to overwrite the component
// with whatever is currently on the site assuming that it comes with proper version numbers.

DECLARE_COMPONENT_VERSION(
// component name
"Discogger",
// component version
FOO_DISCOGGER_VERSION,
"A foobar 2000 plugin to access Discogs database (https://www.discogs.com)\n\n"
"Author: dayuyu\n"
"Version: "FOO_DISCOGGER_VERSION"\n"
"Compiled: "__DATE__ "\n"
"fb2k SDK: "PLUGIN_FB2K_SDK"\n"
"Website: https://github.com/ghDaYuYu/foo_discogs\n"
"\nfoo_discogger is a fork of foo_discogs v2.23 repository by zoomorph (https://bitbucket.org/zoomorph/foo_discogs)\n\n"
"Acknowledgements:\n"
"Thanks to Michael Pujos (aka bubbleguuum) for starting the foo_discogs project (up to version 1.32) and "
"zoomorph for continuing development (up to version 2.23).\n\n"
"foo_discogger uses the following open source libraries (thanks to their respective authors):\n\n"
"jansson - JSON Parser: http://www.digip.org/jansson/\n"
"liboauthcpp - OAuth library: https://github.com/sirikata/liboauthcpp\n\n"
"This plugin uses Discogs' API but is not affiliated with, sponsored or endorsed by Discogs.'Discogs' is a trademark of Zink Media, LLC.\n");

// This will prevent users from renaming your component around (important for proper troubleshooter behaviors) or loading multiple instances of it.
VALIDATE_COMPONENT_FILENAME("foo_discogger.dll");
