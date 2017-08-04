#ifndef PHP_PORTABLE_H
#define PHP_PORTABLE_H

#if PHP_MAJOR_VERSION < 7

struct _zend_string {
	char *val;
	int len;
	int persistent;
};
typedef struct _zend_string zend_string;

#define RETURN_NEW_STR(s)     RETURN_STRINGL(s->val,s->len,0);

static zend_always_inline zend_string *zend_string_alloc(int len, int persistent)
{
	/* aligned to 8 bytes size of buffer to hold (len + 1) characters */
	int alligned_size = (len + 1 + 7) & ~7;
	/* single alloc, so free the buf, will also free the struct */
	char *buf = safe_pemalloc(sizeof(zend_string) + alligned_size, 1, 0, persistent);
	zend_string *str = (zend_string *)(buf + alligned_size);
	str->val = buf;
	str->len = len;
	str->persistent = persistent;
	return str;
}

/* compatibility macros */
# define _RETURN_STRING(str) RETURN_STRING(str, 1)
# define _RETURN_STRINGL(str, l) RETURN_STRINGL(str, l, 1)
typedef long zend_long;
typedef int strsize_t;

#else

# define _RETURN_STRING(str) RETURN_STRING(str)
# define _RETURN_STRINGL(str, l) RETURN_STRINGL(str, l)
typedef size_t strsize_t;

#endif

#endif /* PHP_PORTABLE_H */
