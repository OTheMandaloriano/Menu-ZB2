#pragma once
#include <string>
namespace ItemSearch {
inline std::string Fold(const char* text) {
    std::string out;
    for(const unsigned char* p=reinterpret_cast<const unsigned char*>(text);*p;++p){
        unsigned char c=*p;
        if(c==0xc3 && p[1]){unsigned char next=*++p;c=0;
            if((next>=0x80 && next<=0x85)||(next>=0xa0 && next<=0xa5))c='a';
            else if(next==0x87 || next==0xa7)c='c';
            else if((next>=0x88 && next<=0x8b)||(next>=0xa8 && next<=0xab))c='e';
            else if((next>=0x8c && next<=0x8f)||(next>=0xac && next<=0xaf))c='i';
            else if(next==0x91 || next==0xb1)c='n';
            else if((next>=0x92 && next<=0x96)||(next>=0xb2 && next<=0xb6))c='o';
            else if((next>=0x99 && next<=0x9c)||(next>=0xb9 && next<=0xbc))c='u';
        }
        if(c>='A' && c<='Z')c=static_cast<unsigned char>(c-'A'+'a');
        if((c>='a' && c<='z')||(c>='0' && c<='9'))out+=static_cast<char>(c);
    }
    return out;
}
inline bool Matches(const char* name,const char* alias,const char* query){const auto needle=Fold(query);return Fold(name).find(needle)!=std::string::npos || Fold(alias).find(needle)!=std::string::npos;}
}
