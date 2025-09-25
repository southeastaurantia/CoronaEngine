#include "include/ResourceTypes.h"
#include <algorithm>
#include <cctype>
#include <string>
namespace Corona {
    std::uint64_t ResourceId::ComputeUid(const ResourceType &type, const ResourcePath &path){
        auto normType=type; std::transform(normType.begin(), normType.end(), normType.begin(), [](unsigned char c){return std::tolower(c);});
        auto normPath=path; std::replace(normPath.begin(), normPath.end(),'\\','/');
        normPath.erase(std::unique(normPath.begin(), normPath.end(),[](char a,char b){return a=='/'&&b=='/';}), normPath.end());
        if(!normPath.empty()&&normPath.back()=='/') normPath.pop_back();
        std::transform(normPath.begin(), normPath.end(), normPath.begin(), [](unsigned char c){return std::tolower(c);});
        const std::uint64_t FNV_OFFSET=1469598103934665603ull; const std::uint64_t FNV_PRIME=1099511628211ull;
        auto fnv1a64=[&](const std::string &s,std::uint64_t seed){ std::uint64_t h=seed; for(unsigned char ch: s){ h^=static_cast<std::uint64_t>(ch); h*=FNV_PRIME;} return h;};
        std::uint64_t h=FNV_OFFSET; h=fnv1a64(normType,h); h=fnv1a64(std::string("\n"),h); h=fnv1a64(normPath,h); return h; }
    ResourceId ResourceId::From(ResourceType type, ResourcePath path){ ResourceId id; id.type=std::move(type); id.path=std::move(path); id.uid=ComputeUid(id.type,id.path); return id; }
    bool ResourceId::operator==(const ResourceId &o) const noexcept { auto a= uid?uid:ComputeUid(type,path); auto b= o.uid?o.uid:ComputeUid(o.type,o.path); return a==b; }
    bool ResourceId::operator<(const ResourceId &o) const noexcept { auto a= uid?uid:ComputeUid(type,path); auto b= o.uid?o.uid:ComputeUid(o.type,o.path); return a<b; }
    std::size_t ResourceIdHash::operator()(const ResourceId &id) const noexcept { std::uint64_t u= id.uid? id.uid: ResourceId::ComputeUid(id.type,id.path); return static_cast<std::size_t>(u);}    
    std::uint64_t SubResourceId::Compute(std::uint64_t parent, SubResourceKind kind, std::uint64_t localNumeric, std::string_view localString){
        const std::uint64_t FNV_OFFSET=1469598103934665603ull; const std::uint64_t FNV_PRIME=1099511628211ull; auto h=FNV_OFFSET; auto mix64=[&](std::uint64_t v){ for(int i=0;i<8;++i){ unsigned char b= static_cast<unsigned char>(v & 0xFF); h^=b; h*=FNV_PRIME; v >>=8; }}; mix64(parent); mix64(static_cast<std::uint64_t>(kind)); mix64(localNumeric); for(unsigned char c: localString){ h^=c; h*=FNV_PRIME;} return h; }
    SubResourceId SubResourceId::FromIndex(const ResourceId &parent, SubResourceKind kind, std::uint64_t index){ SubResourceId s; s.parentUid= parent.uid?parent.uid: ResourceId::ComputeUid(parent.type,parent.path); s.kind=kind; s.uid=Compute(s.parentUid,kind,index); return s; }
    SubResourceId SubResourceId::FromKey(const ResourceId &parent, SubResourceKind kind, std::string_view key){ SubResourceId s; s.parentUid= parent.uid?parent.uid: ResourceId::ComputeUid(parent.type,parent.path); s.kind=kind; s.uid=Compute(s.parentUid,kind,0,key); return s; }
    SubResourceId SubResourceId::Compose(std::uint64_t parentUid, SubResourceKind kind, std::uint64_t localNumeric, std::string_view localString){ SubResourceId s; s.parentUid=parentUid; s.kind=kind; s.uid=Compute(parentUid,kind,localNumeric,localString); return s; }
} // namespace Corona
