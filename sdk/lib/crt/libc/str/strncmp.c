#include <string.h>
int strncmp(const char*a,const char*b,size_t n){ while(n&&*a&&*a==*b){a++;b++;n--;} if(!n)return 0; return (int)(unsigned char)*a-(int)(unsigned char)*b; }