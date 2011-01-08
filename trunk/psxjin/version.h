
#define PCSXRR_VERSION 14

#define PCSXRR_NAME "psxjin"

#ifdef WIN32
#include "svnrev.h"
#else
#ifdef SVN_REV
#define SVN_REV_STR SVN_REV
#else
#define SVN_REV_STR ""
#endif
#endif

#define PCSXRR_FEATURE_STRING ""

#ifdef _DEBUG
#define PCSXRR_SUBVERSION_STRING " debug"
#elif defined(PUBLIC_RELEASE)
#define PCSXRR_SUBVERSION_STRING ""
#else
#define PCSXRR_SUBVERSION_STRING " svn" SVN_REV_STR
#endif

#if defined(_MSC_VER)
#define PCSXRR_COMPILER ""
#define PCSXRR_COMPILER_DETAIL " msvc " _Py_STRINGIZE(_MSC_VER)
#define _Py_STRINGIZE(X) _Py_STRINGIZE1((X))
#define _Py_STRINGIZE1(X) _Py_STRINGIZE2 ## X
#define _Py_STRINGIZE2(X) #X
//re: http://72.14.203.104/search?q=cache:HG-okth5NGkJ:mail.python.org/pipermail/python-checkins/2002-November/030704.html+_msc_ver+compiler+version+string&hl=en&gl=us&ct=clnk&cd=5
#else
// TODO: make for others compilers
#define PCSXRR_COMPILER ""
#define PCSXRR_COMPILER_DETAIL ""
#endif

#define PCSXRR_VERSION_STRING "v0.1.4" PCSXRR_SUBVERSION_STRING PCSXRR_FEATURE_STRING PCSXRR_COMPILER
#define PCSXRR_NAME_AND_VERSION PCSXRR_NAME " " PCSXRR_VERSION_STRING
