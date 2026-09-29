#include "license.h"
#include <Windows.h>
#include <bcrypt.h>
#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <cstring>
#include <stdexcept>
namespace License {
namespace {
struct Algorithm {
    BCRYPT_ALG_HANDLE value = nullptr;
    explicit Algorithm(LPCWSTR name) {
        if (BCryptOpenAlgorithmProvider(&value, name, nullptr, 0) < 0)
            throw std::runtime_error("Windows cryptography unavailable");
    }
    ~Algorithm() { if (value) BCryptCloseAlgorithmProvider(value, 0); }
};
bool LowerHex(const std::string& value, size_t length) {
    return value.size() == length && std::all_of(value.begin(), value.end(), [](char c) {
        return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
    });
}
bool Number(const std::string& value, int64_t& output) {
    if (value.empty() || value.size() > 12 || value[0] == '0') return false;
    const auto parsed = std::from_chars(value.data(), value.data()+value.size(), output);
    return parsed.ec == std::errc() && parsed.ptr == value.data()+value.size() && output > 0;
}
}
std::vector<unsigned char> Unhex(const std::string& input) {
    if (input.size()%2 || !LowerHex(input,input.size())) return {};
    const auto digit=[](char c) { return c<='9' ? c-'0' : c-'a'+10; };
    std::vector<unsigned char> output(input.size()/2);
    for(size_t i=0;i<output.size();++i) output[i]=static_cast<unsigned char>((digit(input[i*2])<<4)|digit(input[i*2+1]));
    return output;
}
std::string Hex(const void* data,size_t size) {
    static constexpr char digits[]="0123456789abcdef";
    auto bytes=static_cast<const unsigned char*>(data);
    std::string result(size*2,'0');
    for(size_t i=0;i<size;++i) { result[2*i]=digits[bytes[i]>>4];result[2*i+1]=digits[bytes[i]&15]; }
    return result;
}
std::string Sha256(const void* data,size_t size) {
    if(size>64*1024*1024) throw std::runtime_error("Hash input exceeds limit");
    Algorithm algorithm(BCRYPT_SHA256_ALGORITHM);
    std::array<unsigned char,32> hash{};
    if(BCryptHash(algorithm.value,nullptr,0,static_cast<PUCHAR>(const_cast<void*>(data)),
                  static_cast<ULONG>(size),hash.data(),static_cast<ULONG>(hash.size()))<0)
        throw std::runtime_error("SHA256 failed");
    return Hex(hash.data(),hash.size());
}
bool Verify(const std::string& payload,const std::vector<unsigned char>& signature,
            const std::vector<unsigned char>& publicXY) {
    if(signature.size()!=64 || publicXY.size()!=64 || payload.size()>16384) return false;
    Algorithm algorithm(BCRYPT_ECDSA_P256_ALGORITHM);
    std::array<unsigned char,sizeof(BCRYPT_ECCKEY_BLOB)+64> blob{};
    BCRYPT_ECCKEY_BLOB header{BCRYPT_ECDSA_PUBLIC_P256_MAGIC,32};
    memcpy(blob.data(),&header,sizeof(header));memcpy(blob.data()+sizeof(header),publicXY.data(),64);
    BCRYPT_KEY_HANDLE key=nullptr;
    if(BCryptImportKeyPair(algorithm.value,nullptr,BCRYPT_ECCPUBLIC_BLOB,&key,blob.data(),static_cast<ULONG>(blob.size()),0)<0) return false;
    auto hash=Unhex(Sha256(payload.data(),payload.size()));
    const auto status=BCryptVerifySignature(key,nullptr,hash.data(),static_cast<ULONG>(hash.size()),
        const_cast<PUCHAR>(signature.data()),static_cast<ULONG>(signature.size()),0);
    BCryptDestroyKey(key);
    return status>=0;
}
Result Validate(const std::string& token,const std::string& device,int64_t now,
                const std::vector<unsigned char>& publicXY) {
    Result result;
    result.error=u8"Licença inválida. Confira a chave recebida.";
    if(token.size()>2048 || token.rfind("ZB2L1.",0)!=0) return result;
    const auto dot=token.find('.',6);
    if(dot==std::string::npos) return result;
    auto bytes=Unhex(token.substr(6,dot-6));
    std::string payload(bytes.begin(),bytes.end());
    if(!Verify(payload,Unhex(token.substr(dot+1)),publicXY)) return result;
    std::vector<std::string> fields;
    size_t start=0;
    for(size_t end=payload.find('\n');end!=std::string::npos;end=payload.find('\n',start)) {
        fields.push_back(payload.substr(start,end-start));start=end+1;
    }
    int64_t issued=0;
    if(start!=payload.size() || fields.size()!=6 || fields[0]!="ZB2-LICENSE-1" || fields[1]!="Menu-ZB2" ||
       !LowerHex(fields[2],32) || !LowerHex(fields[3],64) || !Number(fields[4],issued) || !Number(fields[5],result.expires) ||
       result.expires<=issued || result.expires-issued>3650LL*86400) return result;
    if(fields[3]!=device) { result.error=u8"Licença de outro computador. Solicite uma nova.";return result; }
    if(now<issued) { result.error=u8"Licença ainda não válida. Confira a data do PC.";return result; }
    if(now>=result.expires) { result.error=u8"Licença expirada. Solicite a renovação.";return result; }
    result.valid=true;result.id=fields[2];result.error.clear();return result;
}
std::string DeviceId() {
    wchar_t guid[128]{};DWORD size=sizeof(guid);
    if(RegGetValueW(HKEY_LOCAL_MACHINE,L"SOFTWARE\\Microsoft\\Cryptography",L"MachineGuid",
       RRF_RT_REG_SZ|RRF_SUBKEY_WOW6464KEY,nullptr,guid,&size)!=ERROR_SUCCESS || size<=sizeof(wchar_t))
        throw std::runtime_error("Cannot read Windows device identifier");
    std::string value="Menu-ZB2/device/v1:";
    for(wchar_t ch:guid) { if(!ch)break;if(ch>127)throw std::runtime_error("Invalid device identifier");value+=static_cast<char>(ch); }
    return Sha256(value.data(),value.size());
}
int64_t Now() { return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count(); }
}
