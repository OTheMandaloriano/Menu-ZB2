#include "controller.h"
#include "ui.h"
#include "settings.h"
#include "../../shared/widgets.h"
#include "../../shared/graphics.h"
#include "../../shared/theme.h"
#include "../../loader/license.h"
#include "../../../imgui/imgui_impl_win32.h"
#include "../../../imgui/imgui_impl_dx11.h"
#include <Windows.h>
#include <windowsx.h>
#include <commdlg.h>
#include <shellapi.h>
#include <dwmapi.h>
#include <fstream>
#include <memory>
namespace {
AppGraphics graphics;UINT resizeWidth=0,resizeHeight=0;bool dpiChanged=false;float nextScale=1;
struct Handle{HANDLE value=nullptr;~Handle(){if(value)CloseHandle(value);}};
std::filesystem::path Pick(HWND window,bool save,const std::string& suggested,const wchar_t* filter){
    wchar_t buffer[32768]{};
    if(!suggested.empty())MultiByteToWideChar(CP_UTF8,0,suggested.c_str(),-1,buffer,32768);
    OPENFILENAMEW dialog{};dialog.lStructSize=sizeof(dialog);dialog.hwndOwner=window;dialog.lpstrFilter=filter;dialog.lpstrFile=buffer;dialog.nMaxFile=32768;
    const auto packages=Admin::DataRoot().parent_path()/L"Pacotes";
    if(save){std::filesystem::create_directories(packages);dialog.lpstrInitialDir=packages.c_str();}
    dialog.Flags=OFN_NOCHANGEDIR|OFN_PATHMUSTEXIST|(save?OFN_OVERWRITEPROMPT:OFN_FILEMUSTEXIST);
    return (save?GetSaveFileNameW(&dialog):GetOpenFileNameW(&dialog))?std::filesystem::path(buffer):std::filesystem::path();
}
void Save(const std::filesystem::path& path,const std::string& content){
    auto temporary=path;temporary+=L".tmp-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64());
    {std::ofstream file(temporary,std::ios::binary|std::ios::trunc);if(!file)throw std::runtime_error("Nao foi possivel salvar o arquivo.");file<<content<<'\n';file.flush();if(!file)throw std::runtime_error("Falha ao gravar o arquivo completo.");}
    if(!MoveFileExW(temporary.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)){DeleteFileW(temporary.c_str());throw std::runtime_error("Nao foi possivel concluir o arquivo.");}
}
}
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND,UINT,WPARAM,LPARAM);
LRESULT CALLBACK AdminWindowProc(HWND window,UINT message,WPARAM w,LPARAM l){
    if(ImGui_ImplWin32_WndProcHandler(window,message,w,l))return 1;
    switch(message){
    case WM_SIZE:if(w!=SIZE_MINIMIZED){resizeWidth=LOWORD(l);resizeHeight=HIWORD(l);}return 0;
    case WM_DPICHANGED:{nextScale=HIWORD(w)/96.f;dpiChanged=true;auto rect=reinterpret_cast<RECT*>(l);SetWindowPos(window,nullptr,rect->left,rect->top,rect->right-rect->left,rect->bottom-rect->top,SWP_NOZORDER|SWP_NOACTIVATE);return 0;}
    case WM_NCHITTEST:{POINT p{GET_X_LPARAM(l),GET_Y_LPARAM(l)};ScreenToClient(window,&p);if(p.y>=0&&p.y<64*UiTheme::uiScale&&p.x<736*UiTheme::uiScale)return HTCAPTION;break;}
    case WM_DESTROY:PostQuitMessage(0);return 0;
    }
    return DefWindowProcW(window,message,w,l);
}
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE,PWSTR,int show){
    try{
        SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        auto root=Admin::DataRoot();bool showSettings=false;int argc=0;LPWSTR* argv=CommandLineToArgvW(GetCommandLineW(),&argc);
        for(int i=1;i<argc;++i){if(std::wstring(argv[i])==L"--data"&&i+1<argc)root=argv[++i];else if(std::wstring(argv[i])==L"--settings")showSettings=true;}LocalFree(argv);
        auto canonical=std::filesystem::absolute(root).wstring();CharUpperBuffW(canonical.data(),static_cast<DWORD>(canonical.size()));
        auto utf8=std::filesystem::path(canonical).u8string();auto identity=License::Sha256(utf8.data(),utf8.size());
        std::wstring name=L"Local\\ZB2Admin."+std::wstring(identity.begin(),identity.end());Handle mutex;mutex.value=CreateMutexW(nullptr,FALSE,name.c_str());
        const bool duplicate=mutex.value&&GetLastError()==ERROR_ALREADY_EXISTS;
        if(duplicate){HWND existing=FindWindowW(L"ZB2AdminNative",nullptr);if(existing){ShowWindow(existing,SW_RESTORE);SetForegroundWindow(existing);return 0;}}
        WNDCLASSW wc{};wc.lpfnWndProc=AdminWindowProc;wc.hInstance=instance;wc.lpszClassName=L"ZB2AdminNative";wc.hCursor=LoadCursor(nullptr,IDC_ARROW);wc.hIcon=LoadIconW(instance,MAKEINTRESOURCEW(1));if(!RegisterClassW(&wc))return 1;
        HWND window=CreateWindowW(wc.lpszClassName,L"ZB2 Admin",WS_POPUP|WS_SYSMENU|WS_MINIMIZEBOX,CW_USEDEFAULT,CW_USEDEFAULT,Admin::Width,Admin::Height,nullptr,nullptr,instance,nullptr);
        if(!window||!graphics.Initialize(window))return 2;
        ImGui::CreateContext();auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.ConfigFlags|=ImGuiConfigFlags_NavEnableKeyboard;
        nextScale=GetDpiForWindow(window)/96.f;ConfigureUiTheme(nextScale);
        RECT area{};SystemParametersInfoW(SPI_GETWORKAREA,0,&area,0);
        const int width=static_cast<int>(Admin::Width*nextScale),height=static_cast<int>(Admin::Height*nextScale);
        SetWindowPos(window,nullptr,area.left+(area.right-area.left-width)/2,area.top+(area.bottom-area.top-height)/2,width,height,SWP_NOZORDER|SWP_NOACTIVATE);
        const DWORD corners=2;DwmSetWindowAttribute(window,33,&corners,sizeof(corners));
        if(!ImGui_ImplWin32_Init(window)||!ImGui_ImplDX11_Init(graphics.Device(),graphics.Context()))return 3;
        std::unique_ptr<Admin::Controller> controller;if(!duplicate&&mutex.value)controller=std::make_unique<Admin::Controller>(root);
        Admin::UiState state;if(!controller){state.data.busy=false;state.message="Feche a outra versao do Admin e abra esta novamente.";}
        wchar_t self[32768]{};GetModuleFileNameW(nullptr,self,32768);const auto applicationDirectory=std::filesystem::path(self).parent_path();
        state.dataPath=root.u8string();state.applicationPath=applicationDirectory.u8string();
        try{state.reducedMotion=Admin::ReadSettings(root).reducedMotion;Ui::SetReducedMotion(state.reducedMotion);}catch(const std::exception& error){state.message=error.what();}
        if(showSettings){state.page=Admin::Page::Settings;state.started=true;}
        ShowWindow(window,show);bool done=false;
        while(!done){
            MSG message;while(PeekMessageW(&message,nullptr,0,0,PM_REMOVE)){TranslateMessage(&message);DispatchMessageW(&message);if(message.message==WM_QUIT)done=true;}
            if(done)break;if(IsIconic(window)){Sleep(30);continue;}
            if(resizeWidth&&resizeHeight){if(!graphics.Resize(resizeWidth,resizeHeight))break;resizeWidth=resizeHeight=0;}
            if(dpiChanged){ImGui_ImplDX11_InvalidateDeviceObjects();ConfigureUiTheme(nextScale);dpiChanged=false;}
            if(controller)state.data=controller->Get();
            ImGui_ImplDX11_NewFrame();ImGui_ImplWin32_NewFrame();ImGui::NewFrame();Admin::Draw(state);ImGui::Render();
            if(state.send){
                state.send=false;
                bool teamPackage=state.action.command=="team_package"||state.action.command=="authorize_package";
                bool package=state.action.command=="issue_package"||state.action.command=="client_package"||teamPackage;
                if(package){auto path=Pick(window,true,teamPackage?"Equipe-pronta.zip":"Cliente-pronto.zip",L"Pacote pronto ZIP\0*.zip\0\0");
                    if(!path.empty()){if(path.extension()!=L".zip")path+=L".zip";state.action.args.push_back(path.u8string());if(controller)controller->Submit(state.action);}
                }else if(controller)controller->Submit(state.action);
            }
            if(state.copy){ImGui::SetClipboardText(state.copyText.c_str());state.copy=false;}
            if(state.saveSettings){state.saveSettings=false;try{Admin::SaveSettings(root,{state.reducedMotion});state.message="Preferência salva.";}catch(const std::exception& error){state.message=error.what();}}
            if(state.openFolder){
                auto path=state.openFolder==1?root:state.openFolder==2?root.parent_path()/L"Pacotes":state.openFolder==3?applicationDirectory:root.parent_path()/L"logs";
                state.openFolder=0;try{std::filesystem::create_directories(path);if(reinterpret_cast<INT_PTR>(ShellExecuteW(window,L"open",path.c_str(),nullptr,nullptr,SW_SHOWNORMAL))<=32)throw std::runtime_error("Não foi possível abrir a pasta.");}catch(const std::exception& error){state.message=error.what();}
            }
            if(state.exportDiagnostics){state.exportDiagnostics=false;auto path=Pick(window,true,"ZB2-diagnostico.txt",L"Diagnostico de suporte\0*.txt\0\0");if(!path.empty())try{Admin::WriteDiagnostics(path,state.data.role=="owner",state.data.total,static_cast<int>(state.data.grants.size()));state.message="Diagnóstico salvo sem dados sensíveis.";}catch(const std::exception& error){state.message=error.what();}}
            if(state.pick!=Admin::Picker::None&&controller){
                const auto action=state.pick;state.pick=Admin::Picker::None;
                const auto filter=action==Admin::Picker::Request?L"Solicitacao da equipe\0*.zb2station\0\0":action==Admin::Picker::Authorization?L"Autorizacao da estacao\0*.zb2issuer\0\0":L"Chave do proprietario\0*.dpapi\0\0";
                auto path=Pick(window,false,"",filter);
                if(!path.empty()){state.requestPage=state.page;state.pendingFeedback=true;controller->Submit({action==Admin::Picker::Request?"inspect_request":action==Admin::Picker::Authorization?"import_authorization":"import_owner",action==Admin::Picker::Owner?std::vector<std::string>{path.u8string(),state.name}:std::vector<std::string>{path.u8string()}});}
            }
            if(state.save){state.save=false;auto extension=std::filesystem::path(state.exportName).extension().string();
                const wchar_t* filter=extension==".zb2station"?L"Solicitacao da equipe\0*.zb2station\0\0":extension==".zb2issuer"?L"Autorizacao da estacao\0*.zb2issuer\0\0":L"Licenca do cliente\0*.zb2license\0\0";
                auto path=Pick(window,true,state.exportName,filter);if(!path.empty())try{Save(path,state.exportText);state.message="Arquivo salvo. Confira o destino em Ajuda.";}catch(const std::exception& error){state.message=error.what();}
            }
            if(state.close){if(controller)controller->Stop();PostMessageW(window,WM_CLOSE,0,0);state.close=false;}
            if(state.minimize){ShowWindow(window,SW_MINIMIZE);state.minimize=false;}
            if(!graphics.Present(ImGui::GetDrawData()))break;
        }
        if(controller)controller->Stop();ImGui_ImplDX11_Shutdown();ImGui_ImplWin32_Shutdown();ImGui::DestroyContext();graphics.Shutdown();if(IsWindow(window))DestroyWindow(window);UnregisterClassW(wc.lpszClassName,instance);return 0;
    }catch(const std::exception&){return 1;}
}
