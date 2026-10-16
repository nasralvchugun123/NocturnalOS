#include <string.h>
char *strncpy(char*d,const char*s,size_t n){ char*r=d; while(n&&(*d=*s)){d++;s++;n--;} while(n--)*d++=0; return r; }