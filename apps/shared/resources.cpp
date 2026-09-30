#include "resources.h"
#include <Windows.h>
#include <stdexcept>
std::string AppResources::Read(int id) {
    HMODULE module=GetModuleHandleW(nullptr);
    HRSRC resource=FindResourceW(module,MAKEINTRESOURCEW(id),RT_RCDATA);
    if(!resource)throw std::runtime_error("Recurso interno ausente. Obtenha novamente o aplicativo.");
    DWORD size=SizeofResource(module,resource);const void* data=LockResource(LoadResource(module,resource));
    if(!data || size==0 || size>64*1024*1024)throw std::runtime_error("Recurso interno incorreto.");
    return std::string(static_cast<const char*>(data),size);
}
