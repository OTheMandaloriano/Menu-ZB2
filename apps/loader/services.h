#pragma once
#include "license.h"
#include <filesystem>
#include <string>
#include <vector>
namespace LoaderServices {
struct Process { unsigned long pid=0; bool loaded=false,probeLoaded=false,monoLoaded=false; uint64_t created=0; std::filesystem::path executable; };
struct Stored { std::string token; int64_t lastSeen=0; };
std::string Resource(int id);
std::vector<unsigned char> PublicKey();
std::filesystem::path DataDirectory();
Stored ReadState(const std::filesystem::path& root);
void SaveState(const std::filesystem::path& root,const Stored& value);
void CheckClock(const Stored& state,int64_t now);
Process FindGame();
std::string BundleVersion();
void VerifyBundle();
std::filesystem::path InstallBundle(const std::filesystem::path& root);
void LoadRuntime(const Process& game,const std::filesystem::path& runtime);
void PrepareReadiness(const Process& game,const std::filesystem::path& runtime);
bool SceneReady(const Process& game);
bool ReadAuto(const std::filesystem::path& root);
void SaveAuto(const std::filesystem::path& root,bool enabled);
std::string AdjacentLicense(const std::filesystem::path& executable);
bool ImportPackageLicense(const std::filesystem::path& executable,const std::filesystem::path& root,const std::string& device,int64_t now,const std::vector<unsigned char>& key,Stored& stored,std::string& warning);
}
