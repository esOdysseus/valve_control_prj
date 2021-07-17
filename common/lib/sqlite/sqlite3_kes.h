#ifndef _SQLITE3_DB_LIBRARY_BY_KES_H_
#define _SQLITE3_DB_LIBRARY_BY_KES_H_

#include <sys/stat.h>
#include <string>
#include <map>
#include <vector>
#include <mutex>
#include <memory>
#include <iostream>
#include <functional>

#include <sqlite3.h>

namespace db_pkg {


class IDBsqlite3 {
public:
    using Trecord = std::map<std::string /*key*/, std::string /*value*/>;
    using TCBselect = std::function<void(Trecord&)>;

protected:
    using TdbInst = sqlite3;

public:
    static void regist_all_of_db( std::vector<std::string>& db_list ) {
        if( _mtx_lock_.size() != 0 ) {
            throw std::logic_error("Already, db-locker is intialized.");
        }

        for(auto itr=db_list.begin(); itr!=db_list.end(); itr++) {
            _mtx_lock_[*itr];
        }
    }
    
    IDBsqlite3( const std::string& db_path );

    ~IDBsqlite3(void);

    void start(void);

    int query_insert( std::string context );

    int query_insert_replace( std::string context );

    int query_update( std::string context );

    int query_delete( std::string context );

    int query_select( std::string context, TCBselect func );

    std::shared_ptr<std::vector<Trecord>> query_select( std::string context );

protected:
    virtual int cb_oncommit(const std::string& src_name, int pages) {
        std::cout << "IDBsqlite3::cb_oncommit(" << src_name << ", " << pages << ") is called." << std::endl;
        return SQLITE_OK;
    }

    virtual void create_table_model( void ) {
        throw std::runtime_error("IDBsqlite3::create_table_model() is Not implemented.");
    }

    int execute_query(const std::string&& query, TCBselect* pfunc=NULL);

    int execute_query(const std::string& query, TCBselect* pfunc=NULL);

private:
    IDBsqlite3(void) = delete;

    inline bool check_file_exist( const std::string& file_path ) {
        struct stat __buffer__;
        return (stat(file_path.c_str(), &__buffer__) == 0);
    }

    void clear(void);

    void open(const std::string& db_path);

    void exit(void);

    int callback_oncommit(TdbInst* db, const char* source, int pages);

    static int callback_onselect(TCBselect* pfunc, int argc, char **argv, char **azColName);

private:
    std::string _m_db_path_;

    TdbInst* _m_inst_;

    int (*_m_cb_onselect_)(void*,int,char**,char**);

    static std::map<std::string /*db-path*/, std::mutex /*locker*/> _mtx_lock_;

    friend void regist_all_of_db( std::vector<std::string>& db_list );

};


}   // db_pkg

#endif // _SQLITE3_DB_LIBRARY_BY_KES_H_