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
std::vector<std::string> Lines(const std::string& payload) {
    std::vector<std::string> fields;size_t start=0;
    for(size_t end=payload.find('\n');end!=std::string::npos;end=payload.find('\n',start)) {
        fields.push_back(payload.substr(start,end-start));start=end+1;
    }
    if(start!=payload.size())fields.clear();return fields;
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
std::string Normalize(const std::string& token) {
    if(token.size()>8192)return token;
    size_t start=token.rfind("\xef\xbb\xbf",0)==0?3:0;std::string result;
    for(size_t i=start;i<token.size();++i)if(token[i]!=' ' && token[i]!='\t' && token[i]!='\r' && token[i]!='\n')result+=token[i];
    return result;
}
Result Validate(const std::string& input,const std::string& device,int64_t now,
                const std::vector<unsigned char>& publicXY) {
    const auto token=Normalize(input);
    Result result;
    result.error=u8"Cole sua licença ou escolha Abrir arquivo.";
    if(token.empty())return result;
    result.error=u8"Isso não é uma licença. Abra o arquivo .zb2license recebido.";
    if(token.size()>4096 || (token.rfind("ZB2L1.",0)!=0 && token.rfind("ZB2L2.",0)!=0))return result;
    std::vector<std::string> parts;size_t first=0;
    for(size_t end=token.find('.');end!=std::string::npos;end=token.find('.',first)){parts.push_back(token.substr(first,end-first));first=end+1;}
    parts.push_back(token.substr(first));
    const bool delegated=parts[0]=="ZB2L2";
    if(parts.size()!=(delegated?5u:3u))return result;
    auto signer=publicXY;int64_t maxDays=3650,grantIssued=0,grantExpires=INT64_MAX;
    result.error=u8"Licença incompleta ou alterada. Importe o arquivo original.";
    if(delegated){
        auto certificateBytes=Unhex(parts[3]);std::string certificate(certificateBytes.begin(),certificateBytes.end());
        if(!Verify(certificate,Unhex(parts[4]),publicXY))return result;
        auto grant=Lines(certificate);
        if(grant.size()!=8 || grant[0]!="ZB2-ISSUER-1" || grant[1]!="Menu-ZB2" || !LowerHex(grant[2],32) ||
           grant[3].size()<4 || grant[3].size()>160 || Unhex(grant[3]).empty() || !LowerHex(grant[4],128) ||
           !Number(grant[5],grantIssued) || !Number(grant[6],grantExpires) || !Number(grant[7],maxDays) ||
           maxDays>3650 || grantExpires<=grantIssued || grantExpires-grantIssued>3650LL*86400)return result;
        if(now<grantIssued || now>=grantExpires){result.error=u8"Autorização do emissor fora da validade. Solicite uma nova licença.";return result;}
        signer=Unhex(grant[4]);
    }
    auto bytes=Unhex(parts[1]);
    std::string payload(bytes.begin(),bytes.end());
    if(!Verify(payload,Unhex(parts[2]),signer))return result;
    auto fields=Lines(payload);
    int64_t issued=0;
    if(fields.size()!=6 || fields[0]!="ZB2-LICENSE-1" || fields[1]!="Menu-ZB2" ||
       !LowerHex(fields[2],32) || !LowerHex(fields[3],64) || !Number(fields[4],issued) || !Number(fields[5],result.expires) ||
       result.expires<=issued || result.expires-issued>maxDays*86400 || issued<grantIssued || result.expires>grantExpires) return result;
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
