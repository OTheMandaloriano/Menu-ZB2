#pragma once
#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>
using HWND=void*;using HMODULE=void*;using UINT=unsigned;using LONG=long;using DWORD=unsigned long;
constexpr int MAX_PATH=260,TRUE=1,FALSE=0;
constexpr DWORD INVALID_FILE_ATTRIBUTES=~DWORD(0);
constexpr int MOVEFILE_REPLACE_EXISTING=1,MOVEFILE_WRITE_THROUGH=8,MAPVK_VK_TO_VSC=0;
constexpr int VK_LBUTTON=1,VK_RBUTTON=2,VK_MBUTTON=4,VK_XBUTTON1=5,VK_XBUTTON2=6,VK_ESCAPE=27,VK_INSERT=45,VK_DELETE=46,VK_HOME=36,VK_END=35,VK_PRIOR=33,VK_NEXT=34,VK_LEFT=37,VK_UP=38,VK_RIGHT=39,VK_DOWN=40,VK_NUMLOCK=144;
constexpr size_t _TRUNCATE=~size_t(0);
namespace TestPlatform { inline bool failCopy=false,failMove=false;inline std::string directory;inline std::string Path(const char* value){std::string path=value;std::replace(path.begin(),path.end(),'\\','/');return path;} }
inline int CreateDirectoryA(const char* path,void*){std::error_code error;std::filesystem::create_directories(TestPlatform::Path(path),error);return !error;}
inline DWORD GetFileAttributesA(const char* path){std::error_code error;return std::filesystem::exists(TestPlatform::Path(path),error)?0:INVALID_FILE_ATTRIBUTES;}
inline int CopyFileA(const char* from,const char* to,int){if(TestPlatform::failCopy)return FALSE;std::error_code error;return std::filesystem::copy_file(TestPlatform::Path(from),TestPlatform::Path(to),std::filesystem::copy_options::none,error)&&!error;}
inline int MoveFileExA(const char* from,const char* to,int){if(TestPlatform::failMove)return FALSE;std::error_code error;std::filesystem::rename(TestPlatform::Path(from),TestPlatform::Path(to),error);return !error;}
inline UINT MapVirtualKeyA(UINT value,int){return value;}
inline int GetKeyNameTextA(LONG,char*,int){return 0;}
inline short GetAsyncKeyState(int){return 0;}
inline int fopen_s(FILE** stream,const char* path,const char* mode){*stream=std::fopen(TestPlatform::Path(path).c_str(),mode);return *stream?0:1;}
inline int strncpy_s(char* dest,size_t capacity,const char* source,size_t count){size_t size=std::min(capacity-1,std::min(std::strlen(source),count));std::memcpy(dest,source,size);dest[size]=0;return 0;}
template<size_t N> int strncpy_s(char(&dest)[N],const char* source,size_t count){return strncpy_s(dest,N,source,count);}
inline int _snprintf_s(char* dest,size_t capacity,size_t,const char* format,...){va_list args;va_start(args,format);int result=std::vsnprintf(dest,capacity,format,args);va_end(args);return result;}
template<size_t N,typename... Args> int _snprintf_s(char(&dest)[N],size_t,const char* format,Args... args){return std::snprintf(dest,N,format,args...);}
