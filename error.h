#ifndef __ERROR_H__
#define __ERROR_H__

#define die(format,args...) \
	do { fprintf(stderr, "[Fatal] "format, ##args); exit(1); }while(0)

#define warn(format,args...) \
	do { fprintf(stderr, "[Warning:%s:%d] "format, __FILE__, __LINE__, ##args); }while(0)

#define info(format,args...) \
	do { fprintf(stderr, "[Info:%s:%d] "format, __FILE__, __LINE__, ##args); }while(0)

#define bug(format,args...) \
	do { fprintf(stderr, "[Bug:%s:%d] "format"\nplease contact wenxichang@163.com\n", \
		__FILE__, __LINE__, ##args); exit(1); }while(0)

#define todo(format,args...) \
	do { fprintf(stderr, "[TODO:%s:%d] "format"\nunfinish version, sorry\n", \
		__FILE__, __LINE__, ##args); exit(1); }while(0)
		
#endif
