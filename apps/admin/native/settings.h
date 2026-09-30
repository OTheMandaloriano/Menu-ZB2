#pragma once
#include <filesystem>
#include <vector>
#include <string>
#include <cstdint>
namespace Admin {
struct Settings {bool reducedMotion=false;};
struct CacheEntry {std::filesystem::path path;std::string hash;uint64_t bytes=0;};
struct CachePlan {std::vector<CacheEntry> entries;uint64_t bytes=0;};
struct CacheResult {int removed=0,kept=0;uint64_t bytes=0;};
CachePlan InspectCache(const std::filesystem::path& root,const std::string& currentHash);
CacheResult CleanCache(const std::filesystem::path& root,const CachePlan& reviewed,const std::string& currentHash);
Settings ReadSettings(const std::filesystem::path& root);
void SaveSettings(const std::filesystem::path& root,const Settings& settings);
void WriteDiagnostics(const std::filesystem::path& path,bool owner,int licenseCount,int teamCount);
}
