#include <logger.h>
#include <CSource.h>

template void CSource::init(std::shared_ptr<struct sockaddr_in> addr, const char* alias, enum_c::ProviderType provider_type);
template void CSource::init(std::shared_ptr<int> addr, const char* alias, enum_c::ProviderType provider_type);
template std::shared_ptr<struct sockaddr_in> CSource::get_address(void);
template std::shared_ptr<int> CSource::get_address(void);

/***********************************************************
 * Definition member-function of CSource Class.
 * */
CSource::CSource(void) {
    addr_type = EADDR_TYPE::E_PVDT_NOT_DEFINE;
    alias.clear();
}

CSource::~CSource(void){
    try{
        if(addr_type != EADDR_TYPE::E_PVDT_NOT_DEFINE) {
            address.reset();
        }
        addr_type = EADDR_TYPE::E_PVDT_NOT_DEFINE;
        alias.clear();
    }
    catch(const std::exception &e) {
        LOGERR("%s", e.what());
    }
}

template <typename ADDR_TYPE> 
void CSource::init(std::shared_ptr<ADDR_TYPE> addr, 
                                const char* alias, 
                                enum_c::ProviderType provider_type) {
    try {
        std::shared_ptr<void> dumy = std::static_pointer_cast<void>(addr);
        this->address = move(dumy);
        this->addr_type = provider_type;
        this->alias = alias;
    }
    catch(const std::exception &e) {
        LOGERR("%s", e.what());
    }
}

template <typename ADDR_TYPE> 
std::shared_ptr<ADDR_TYPE> CSource::get_address(void) {
    try{
        return std::static_pointer_cast<ADDR_TYPE>(address);
    }catch(const std::exception &e) {
        LOGERR("%s", e.what());
    }
}

std::string CSource::get_alias(void) {
    return alias;
}

CSource::EADDR_TYPE CSource::get_addr_type(void) {
    return addr_type;
}

