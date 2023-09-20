#ifndef _CSTR_H_
# include <string.h>
# define _CSTR_H_

typedef const char* cstr_t;

# define _strcomb(  dst, src, _T)     strcat( strcpy(dst, src   ), _T"\0")
# define _strcatl(  dst, src, _T)     strcat( strcat(dst, src   ), _T"\0")
# define _strncomb( dst, src, _T, l)  strcat(strncpy(dst, src, l), _T"\0")
# define _strrepl(  dst, s,   _T)    _strnrepl(dst, s, _T, sizeof(_T))

static inline cstr_t _strnrepl(
	char  *dst, unsigned int s,
	cstr_t src, unsigned int l
) {
	for(int i = 0; i < l; i++) {
	    dst[i + s] =  src[i];
	}; (dst[s + l] = '\0');
	return dst;
}

#endif
