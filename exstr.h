#ifndef __EXSTR_H__
# include <string.h>
# define __EXSTR_H__

# define _strcomb(dst, src, LIT) strcat(strcpy(dst, src), LIT"\0")
# define _strcatl(dst, src, LIT) strcat(strcat(dst, src), LIT"\0")
# define _strrepl(dst, s, LIT) _strnrepl(dst, s, LIT, sizeof(LIT))
# define _strnrepl(dst, s, LIT, l) \
	for(int i = 0; i < l; i++) {\
	    dst[i + s] =  (LIT)[i];\
	}; (dst[s + l] = '\0')

#endif
