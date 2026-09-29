#pragma once
#include <cstdint>
#include <string>
#include <vector>
namespace License {
struct Result {
    bool valid = false;
    std::string id;
    std::string error;
    int64_t expires = 0;
};
std::vector<unsigned char> Unhex(const std::string& input);
std::string Hex(const void* data, size_t size);
std::string Sha256(const void* data, size_t size);
bool Verify(const std::string& payload, const std::vector<unsigned char>& signature,
            const std::vector<unsigned char>& publicXY);
Result Validate(const std::string& token, const std::string& device, int64_t now,
                const std::vector<unsigned char>& publicXY);
std::string DeviceId();
int64_t Now();
}
