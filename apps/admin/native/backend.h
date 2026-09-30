#pragma once
#include "model.h"
#include <filesystem>
namespace Admin {
std::filesystem::path DataRoot();
class Backend {
public:
    explicit Backend(std::filesystem::path root):root_(std::move(root)){}
    Snapshot Call(const Request& request);
private:
    std::filesystem::path root_;
    std::filesystem::path Executable();
};
}
