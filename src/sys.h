#ifndef _SYS_H_
# include <stdio.h>
# include "config.h"
# include "cstr.h"
# define _SYS_H_

# ifdef _WIN32
	#include <windows.h>
	#include <shlobj.h>
	#define SYS_PATH_S "\\"
	#define SYS_PATH_C '\\'
	#define SYS_PATH_L MAX_PATH
	#define SYS_CONF_DIR CL_NAME"\\"
	#define IS_PATH_NOT_ABSOLUTE(p) \
		(p->path[1] != ':' || p->path[2] != SYS_PATH_C || p->path[3] != SYS_PATH_C)
# else
	#ifdef __APPLE__
		#include <mach-o/dyld.h>
	#endif
	#include <pwd.h>
	#include <limits.h>
	#include <unistd.h>
	#include <sys/stat.h>
	#include <sys/types.h>
	#define SYS_PATH_S "/"
	#define SYS_PATH_C '/'
	#define SYS_PATH_L 1024
	#define SYS_CONF_DIR ".config/"CL_NAME"/"
	#define IS_PATH_NOT_ABSOLUTE(p) (p->path[0] != SYS_PATH_C)
# endif

typedef struct {
	unsigned short carret;
	char path[SYS_PATH_L];
} path_t;

# define sys_make_dir_path(p, _T)  sys_extend_path(p, _T SYS_PATH_S, 1)
# define sys_make_file_path(p, _T) sys_extend_path(p, _T, 0)

static inline cstr_t sys_extend_path(path_t *dir, cstr_t ext, const char move_carret)
{
	int i = 0, k = dir->carret;
	while (k < SYS_PATH_L && (dir->path[k] = ext[i]) != '\0')
		i++, k++;
	if (move_carret)
		dir->carret = k;
	return dir->path;
}

static inline cstr_t sys_cuts_path(path_t *dir, int ridx, const char move_carret)
{
	int k = ridx - 1;
	while (k >= 0 && dir->path[k] != SYS_PATH_C)
		dir->path[k--] = '\0', ridx--;
	if (move_carret)
		dir->carret = ridx;
	return dir->path;
}

typedef enum {
	SUCESS_fail = 0,
	SUCESS_ok   = 1
} SUCESS;

static inline SUCESS SysExtractArgPath(path_t *exe, const char* restrict argv0, const char autoslash)
{
	char c;
	int msz, i = 0,
		sli, s = 0;
	do {
		if((c = argv0[i]) == SYS_PATH_C)
			s = i;
		exe->path[i] = c;
	} while (c != '\0' && ++i < SYS_PATH_L);

	msz = i + 1,
	sli = s + 1;

	if (sli == i) {
		msz = i;
	} else if (msz < SYS_PATH_L && autoslash) {
		exe->path[i] = SYS_PATH_C;
	} else {
		exe->path[(msz = sli)] = '\0';
	}
	return (exe->carret = msz) > 0 ? SUCESS_ok : SUCESS_fail;
}

static inline SUCESS SysAcessConfigPath(path_t *cfg)
{
# ifdef _WIN32
	if (S_OK == SHGetFolderPath(
		NULL, CSIDL_FLAG_CREATE | CSIDL_LOCAL_APPDATA,
		NULL, 0, (LPTSTR)cfg->path))
# else
	struct passwd *pw = getpwuid(getuid());

	if (pw && SysExtractArgPath(cfg, pw->pw_dir, 1))
#endif
	{
		(void)sys_extend_path(cfg, SYS_CONF_DIR, 1);
# ifdef _WIN32
		if (SHCreateDirectory(NULL, (LPTSTR)cfg->path) == ERROR_SUCCESS)
# else
		if (  mkdir(cfg->path, 0777) == 0 )
			/* sucess */;
# endif
			return SUCESS_ok;
	}
	return SUCESS_fail;
}

static inline SUCESS SysGetExecPath(path_t *exe) {

	ssize_t sz = 0;

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
	if (sys_cuts_path(exe, sz, 1)[0] == SYS_PATH_C)
		return SUCESS_ok;
	return SUCESS_fail;
}

#endif // _SYS_H_
