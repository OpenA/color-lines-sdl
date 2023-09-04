#ifndef _SYS_H_
# include <stdio.h>
# include "cstr.h"
# define _SYS_H_

# ifdef _WIN32
	#include <windows.h>
	#include <shlobj.h>
	#define SYS_PATH_S '\\'
	#define SYS_PATH_L MAX_PATH
	#define IS_PATH_NOT_ABSOLUTE(p) \
		(p->path[1] != ':' || p->path[2] != OS_PATH_S || p->path[3] != OS_PATH_S)
# else
	#ifdef __APPLE__
		#include <mach-o/dyld.h>
	#endif
	#include <pwd.h>
	#include <limits.h>
	#include <unistd.h>
	#include <sys/stat.h>
	#include <sys/types.h>
	#define SYS_PATH_S '/'
	#define SYS_PATH_L 1024
	#define IS_PATH_NOT_ABSOLUTE(p) (p->path[0] != OS_PATH_S)
# endif

typedef struct {
	unsigned int len;
	char path[SYS_PATH_L];
} path_t;

static inline int SysAcessConfigPath(path_t *cfg, cstr_t appname) {

	size_t sz = 0;

# ifdef _WIN32
	if (S_OK == SHGetFolderPath(
		NULL, CSIDL_FLAG_CREATE | CSIDL_LOCAL_APPDATA,
		NULL, 0, (LPTSTR)cfg->path))) {

		PathAppend((LPTSTR)cfg->path), TEXT(appname));

		if (SHCreateDirectory(NULL, (LPTSTR)cfg->path)) == ERROR_SUCCESS)
			sz = strlen(strcat(cfg->path, "\\"));
	}
# else
	struct passwd *pw = getpwuid(getuid());

	if (pw) {
		_strcomb(cfg->path, pw->pw_dir, "/.config/");
		_strcatl(cfg->path, appname, "/");

		sz = strlen(cfg->path);
		if (  mkdir(cfg->path, 0777) == 0 )
			/* sucess */;
	}
#endif
	if ( sz > 0 )
		cfg->len = sz;
	return sz > 0;
}

static inline int SysGetExecPath(path_t *exe) {

	ssize_t i, sz = 0;

# if   defined(_WIN32)
	sz = GetModuleFileName(NULL, exe->path, SYS_PATH_L);
# elif defined(__APPLE__)
	_NSGetExecutablePath(exe->path, &sz);
# elif defined(__FreeBSD__)
	int mib[4] = {CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, -1};
	sysctl(mib, 4, exe->path, &sz, NULL, 0);
# else

# if defined(__linux__)
#  define PROC_LNK "/proc/self/exe"
# elif defined(__NetBSD__)
#  define PROC_LNK "/proc/curproc/exe"
# else // other unix
#  define PROC_LNK p_lnk
	char p_lnk[127];
	snprintf( p_lnk, 127, "/proc/%d/exe", getpid() );
# endif
	sz = readlink(PROC_LNK, exe->path, SYS_PATH_L);
#  undef PROC_LNK

# endif

	for (i = sz - 1; i >= 0 && exe->path[i] != SYS_PATH_S; i--)
		exe->path[i] = '\0', sz--;
	if ( sz > 0 )
		exe->len = sz;
	return sz > 0;
}

#endif // _SYS_H_
