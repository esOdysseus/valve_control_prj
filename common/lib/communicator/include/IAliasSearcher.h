/***
 * CAliasSearcher.h
 * Copyright [2019-] 
 * Written by EunSeok Kim <es.odysseus@gmail.com>
 * 
 * This file is part of the Common-Communicator framework.
 */
#ifndef _ALIAS_SEARCHER_INTERFACE_H_
#define _ALIAS_SEARCHER_INTERFACE_H_

#include <map>
#include <list>
#include <memory>
#include <string>

#include <IAliasPVD.h>


namespace alias {


    class IAliasSearcher {
    public:
        using TPvdList = std::list<std::shared_ptr<cf_alias::IAliasPVD>>;

    public:
        static std::shared_ptr<IAliasSearcher> get_instance( const std::string& alias_file_path );

        virtual ~IAliasSearcher(void) = default;

        /// Search my-provider list that is created by my application.
        virtual std::map<std::string, TPvdList> get_mypvds( const std::string& my_app ) = 0;

        /// Search peer-provider that is connected with my-app.
        virtual std::shared_ptr<cf_alias::IAliasPVD> get_peer_provider( const std::string& peer_app, const std::string& peer_pvd ) = 0;

        /// In multiple provider in a my-apps, search my-provider that is connected with wanted peer.
        virtual TPvdList get_mypvds_sendable(const std::string& peer_app, const std::string& peer_pvd ) = 0;

    protected:
        IAliasSearcher(void) = default;

    private:
        IAliasSearcher( const IAliasSearcher& inst ) = delete;
        IAliasSearcher( IAliasSearcher&& inst ) = delete;

    };


}

#endif // _ALIAS_SEARCHER_INTERFACE_H_