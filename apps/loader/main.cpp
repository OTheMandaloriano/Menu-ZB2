#include "window.h"
#include <exception>

int WINAPI wWinMain(HINSTANCE instance,HINSTANCE,PWSTR,int show) {
    HANDLE mutex=CreateMutexW(nullptr,FALSE,L"Local\\ZB2Menu.Loader.v1");
    if(!mutex)return 4;
    if(GetLastError()==ERROR_ALREADY_EXISTS){CloseHandle(mutex);MessageBoxW(nullptr,L"O loader já está aberto.",L"ZB2 Menu",MB_OK|MB_ICONINFORMATION);return 0;}
    int result=1;
    try{result=RunLoaderWindow(instance,show);}
    catch(const std::exception&){MessageBoxW(nullptr,L"Não foi possível iniciar o loader. Confira os arquivos e tente novamente.",L"ZB2 Menu",MB_OK|MB_ICONERROR);}
    CloseHandle(mutex);return result;
}
