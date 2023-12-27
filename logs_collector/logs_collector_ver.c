#include <stdio.h>
#include <string.h>

#define MAJOR_VER		1
#define MINOR_VER		0
#define MICRO_VER		0

char *get_version_string(void)
{
	static char version[32] = { 0 };

	if (strlen(version))
		return version;

#ifdef _E_VERSION_
	snprintf(version, sizeof(version), "%s", _E_VERSION_);
#else /* _E_VERSION_ */
	snprintf(version, sizeof(version), "%d.%d.%dL", MAJOR_VER, MINOR_VER,
								MICRO_VER);
#endif /* _E_VERSION_ */

	return version;
}
