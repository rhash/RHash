/* platform.h - platform-specific includes and macro */
#ifndef PLATFORM_H
#define PLATFORM_H

/* if compiler is MS VC++ */
#if _MSC_VER > 1000
#define inline __inline

/* disable warnings: The POSIX name for this item is deprecated. Use the ISO C++ conformant name. */
#pragma warning(disable : 4996)

#else /* _MSC_VER > 1000 */

#include <unistd.h>

#endif /* _MSC_VER > 1000 */

#endif /* PLATFORM_H */
