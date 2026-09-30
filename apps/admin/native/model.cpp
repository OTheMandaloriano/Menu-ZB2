#include "model.h"
#include "../../loader/license.h"
#include <sstream>
#include <stdexcept>
#include <charconv>
namespace Admin {
std::string EncodeRequest(const Request& request){
    std::string result=request.command;
    for(const auto& item:request.args){result+='\t';result+=License::Hex(item.data(),item.size());}
    if(result.size()>131072)throw std::runtime_error("Pedido muito grande.");return result;
}
namespace {
int Number(const std::string& input){int value=0;auto r=std::from_chars(input.data(),input.data()+input.size(),value);if(r.ec!=std::errc()||r.ptr!=input.data()+input.size()||value<0)throw std::runtime_error("Resposta numerica incorreta.");return value;}
}
Snapshot DecodeResponse(const std::string& response){
    if(response.size()>4*1024*1024)throw std::runtime_error("Resposta muito grande.");
    Snapshot model;model.busy=false;bool ok=false,state=false;std::istringstream stream(response);std::string row;
    while(std::getline(stream,row)){
        std::istringstream columns(row);std::string type,field;std::getline(columns,type,'\t');std::vector<std::string> fields;
        while(std::getline(columns,field,'\t')){auto bytes=License::Unhex(field);if(!field.empty()&&bytes.empty())throw std::runtime_error("Resposta incorreta.");fields.emplace_back(bytes.begin(),bytes.end());}
        if(!row.empty()&&row.back()=='\t')fields.emplace_back();
        if(type=="ERR"&&fields.size()==1)throw std::runtime_error(fields[0]);
        if(type=="OK"&&fields.size()==1){ok=true;model.notice=fields[0];}
        else if(type=="S"&&fields.size()==8){model.role=fields[0];model.name=fields[1];model.stationId=fields[2];model.maxDays=Number(fields[3]);model.problem=fields[4];model.grantExpiry=fields[5];model.user=fields[6];model.hasKey=fields[7]=="1";state=true;}
        else if(type=="N"&&fields.size()==2){model.total=Number(fields[0]);model.matched=Number(fields[1]);}
        else if(type=="L"&&(fields.size()==8||fields.size()==9)){
            model.licenses.push_back({fields[0],fields[1],fields[2],fields[3],fields[4],fields[5],fields[6],fields[7]});
            if(fields.size()==9){auto& value=model.licenses.back().expires;auto parsed=std::from_chars(fields[8].data(),fields[8].data()+fields[8].size(),value);if(parsed.ec!=std::errc()||parsed.ptr!=fields[8].data()+fields[8].size()||value<=0)throw std::runtime_error("Data de validade incorreta.");}
        }
        else if(type=="G"&&fields.size()==5)model.grants.push_back({fields[0],fields[1],fields[2],fields[3],fields[4]});
        else if(type=="X"&&fields.size()==2){model.output=fields[0];model.filename=fields[1];}
        else if(type=="R"&&fields.size()==3){model.requestName=fields[0];model.requestId=fields[1];model.requestText=fields[2];}
        else throw std::runtime_error("Resposta do servico incompatível.");
    }
    if(!ok||!state)throw std::runtime_error("Resposta incompleta do servico.");model.initialized=true;return model;
}
}
