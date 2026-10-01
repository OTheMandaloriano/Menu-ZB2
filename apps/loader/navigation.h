#pragma once
#include <vector>
enum class LoaderPage {Library,Game,Access,Help,Credits,Diagnostics};
struct LoaderNavigation {
    LoaderPage page=LoaderPage::Library;
    std::vector<LoaderPage> history;
    void Go(LoaderPage target){
        if(page==target)return;
        if(target==LoaderPage::Library)history.clear();
        else{if(history.size()==16)history.erase(history.begin());history.push_back(page);}
        page=target;
    }
    void Back(){if(history.empty())page=LoaderPage::Library;else{page=history.back();history.pop_back();}}
};
