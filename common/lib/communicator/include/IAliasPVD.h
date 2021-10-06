/***
 * CConfigAliases.h
 * Copyright [2019-] 
 * Written by EunSeok Kim <es.odysseus@gmail.com>
 * 
 * This file is part of the Common-Communicator framework.
 */
#ifndef _C_ALIASES_PROVIDER_INTERFACE_H_
#define _C_ALIASES_PROVIDER_INTERFACE_H_

#include <string>
#include <memory>

#include <Enum_common.h>

namespace cf_alias {


    /***
     * Alias & Provider Interface.
     */
    class IAlias {
    public:
        IAlias(const char* pvd_id, enum_c::AliasType type_, std::shared_ptr<IAlias> parent_=std::shared_ptr<IAlias>());

        IAlias(const char* app_path, const char* pvd_id, enum_c::AliasType type_);

        virtual ~IAlias(void);

        // getter
        std::string name( void );

        std::string path( void );

        std::string path_parent( void );

        // setter
        void update( std::string& parent, std::string& name );

        enum_c::AliasType alias_type( void );

        static std::string make_full_path(std::string &app_path, std::string &pvd_path);

        static std::string make_full_path(std::string &&app_path, std::string &&pvd_path);

    private:
        // setter
        void set_path_parent( IAlias& parent_ );

        void set_path_parent( std::string app_path );

    private:
        std::string _m_name_;       // Current name.
        std::string _m_path_;       // Path of naming from resource to APP.
        enum_c::AliasType _m_type_;

    };


    class IAliasPVD : public IAlias {
    public:
        // for svc-pvd/XXX/provider-type
        static constexpr const char* UDP = "udp";
        static constexpr const char* UDP_UDS = "udp_uds";
        static constexpr const char* TCP = "tcp";
        static constexpr const char* TCP_UDS = "tcp_uds";
        static constexpr const char* VSOMEIP = "vsomeip";

    public:
        IAliasPVD(const char* alias_, const char* pvd_type_, std::shared_ptr<IAlias> parent_);

        IAliasPVD(const char* app_path, const char* pvd_path, const char* pvd_type_);

        virtual ~IAliasPVD(void);

        // getter
        enum_c::ProviderType type( void );

        template <typename PVD_TYPE>
        PVD_TYPE* get( void ) {
            return dynamic_cast<PVD_TYPE*>(this);
        }

        template <typename PVD_TYPE>
        static std::shared_ptr<PVD_TYPE> convert( std::shared_ptr<IAliasPVD> target ) {
            return std::dynamic_pointer_cast<PVD_TYPE>( target );
        }

        static std::string convert(enum_c::ProviderType pvd_type);

    private:
        enum_c::ProviderType convert(std::string type_);

    private:
       enum_c::ProviderType _m_type_;

    };


}   // cf_alias

#endif // _C_ALIASES_PROVIDER_INTERFACE_H_
