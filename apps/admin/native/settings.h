#pragma once
#include <filesystem>
namespace Admin {
struct Settings {bool reducedMotion=false;};
Settings ReadSettings(const std::filesystem::path& root);
void SaveSettings(const std::filesystem::path& root,const Settings& settings);
void WriteDiagnostics(const std::filesystem::path& path,bool owner,int licenseCount,int teamCount);
}
