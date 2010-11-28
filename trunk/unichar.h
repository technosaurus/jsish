#ifndef __UNICHAR_H__
#define __UNICHAR_H__

typedef unsigned short unichar;

/* for declare unicode static string 
 * in unicode supported compiler, use wchar_t and L"string" may save a lot of works
 * any way, you should declare a static unicode string like this:
 * 
 * static UNISTR(5) hello = { 5, { 'h', 'e', 'l', 'l', 'o' } };
 * 
 * comment: alway declare one more byte for objkey
 */
#define UNISTR(_len) struct{int len;unichar unistr[(_len)+1];}

#define unistrlen(str) (*((int *)(((int)(str)) - sizeof(int))))

unichar *unistrdup(const unichar *str);
unichar *unistrdup_str(const char *str);
int unistrcmp(const unichar *str1, const unichar *str2);
unichar *unistrcat(const unichar *str1, const unichar *str2);
void unistrcpy(unichar *to, const unichar *from);
void unifree(unichar *d);
int unistrchr(const unichar *str, int c);
char *c_strdup(const char *buf);
void c_strfree(char *buf);
int unistrpos(unichar *str, int start, unichar *nid);
unichar *unisubstrdup(const unichar *a, int start, int len);

/* those two are very dangerous, keep your eyes on */
const unichar *tounichars(const char *str);
const char *tochars(const unichar *str);
void _uniprint(unichar *s);

#endif

