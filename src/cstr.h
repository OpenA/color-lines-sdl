#ifndef _CSTR_H_
# include <string.h>
# define _CSTR_H_

typedef const char* cstr_t;

# define _strcomb(  dst, src, _T)     strcat( strcpy(dst, src   ), _T)
# define _strcatl(  dst, src, _T)     strcat( strcat(dst, src   ), _T)
# define _strncomb( dst, src, _T, l)  strcat(strncpy(dst, src, l), _T)

# define cstr_rep_lit(_T, dst, si) cstr_rep_n(_T, sizeof(_T), dst, si)
# define cstr_cpy_lit(_T, dst, mx) cstr_cpy_a(_T, (signed)mx, dst)

static inline int cstr_rep_n(
	cstr_t src, const int repNum, char dst[], int sidx
) {
	for (int i = 0; i < repNum; i++, sidx++)
	   dst[sidx] = src[i];
	return sidx;
}

static inline int cstr_cpy_a(
	cstr_t src, const int sizeMax, char dst[]
) {
	signed idx = 0;
	while (idx < sizeMax && src[idx] != '\0')
	   dst[idx] = src[idx], idx++;
	return idx;
}

#endif
