#pragma once
#include <string>
#include <vector>
#include <cstdint>
namespace Admin {
struct LicenseRow {std::string id,customer,device,issuer,token,expiry,days,status;int64_t expires=0;};
struct GrantRow {std::string id,name,expiry,days,token;};
struct Snapshot {
    bool busy=true,initialized=false,hasKey=false,error=false;
    unsigned revision=0;int maxDays=0,total=0,matched=0;
    std::string role="pending",name,stationId,device,user,problem,grantExpiry,notice;
    std::string output,filename,issuedId,requestName,requestId,requestText;
    std::vector<LicenseRow> licenses;std::vector<GrantRow> grants;
};
struct Request {std::string command;std::vector<std::string> args;};
std::string EncodeRequest(const Request& request);
Snapshot DecodeResponse(const std::string& response);
}
