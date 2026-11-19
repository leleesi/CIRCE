#pragma once
inline bool columnIs(const char* p,const char* expected){
    if((unsigned char)p[0]==0xef && (unsigned char)p[1]==0xbb && (unsigned char)p[2]==0xbf)p+=3;
    while(*p==' '||*p=='\t')++p;
    if(*p=='"')++p;
    while(*expected){
        char c=*p++;if(c>='A'&&c<='Z')c+='a'-'A';
        if(c!=*expected++)return false;
    }
    if(*p=='"')++p;
    while(*p==' '||*p=='\t')++p;
    return !*p;
}
