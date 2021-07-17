#ifndef _CLASS_SQLITE_DATABASE_DEFINITION_H_
#define _CLASS_SQLITE_DATABASE_DEFINITION_H_

#include <vector>

#include <sqlite3_kes.h>

namespace db_pkg {


class CDBsqlite: public IDBsqlite3 {
public:
    CDBsqlite( const std::string db_path, const char* const* tables_model )
    : db_pkg::IDBsqlite3(db_path) {
        if( tables_model == NULL ) {
            throw std::invalid_argument("tables_model is NULL.");
        }

        for( int i=0; tables_model[i] != NULL; i++ ) {
            _m_tables_model_.push_back( std::string(tables_model[i]) );
        }

        if( _m_tables_model_.size() <= 0 ) {
            throw std::invalid_argument("tables_model is Empty.");
        }
    }

    ~CDBsqlite( void ) {
        _m_tables_model_.clear();
    }

protected:
    void create_table_model( void ) override {
        std::cout << "CDBsqlite::create_table_model() is called." << std::endl;
        for( auto itr=_m_tables_model_.begin(); itr!=_m_tables_model_.end(); itr++ ) {
            execute_query( "CREATE TABLE IF NOT EXISTS " + *itr + ";" );
        }
    }

private:
    CDBsqlite(void) = delete;
    CDBsqlite(const CDBsqlite&) = delete;             // copy constructor
    CDBsqlite& operator=(const CDBsqlite&) = delete;  // copy operator
    CDBsqlite(CDBsqlite&&) = delete;                  // move constructor
    CDBsqlite& operator=(CDBsqlite&&) = delete;       // move operator

private:
    std::vector<std::string> _m_tables_model_;

};


}   // namespace db_pkg


#endif // _CLASS_SQLITE_DATABASE_DEFINITION_H_