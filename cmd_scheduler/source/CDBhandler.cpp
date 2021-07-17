#include <CDBhandler.h>

#include <logger.h>

namespace db {

constexpr const char * CDBhandler::DB_NAME_FUTURE;
constexpr const char * CDBhandler::DB_NAME_NOW;
constexpr const char * CDBhandler::DB_NAME_PAST;

constexpr const char * CDBhandler::TABLE_MODEL_FUTURE[];
constexpr const char * CDBhandler::TABLE_MODEL_NOW[];
constexpr const char * CDBhandler::TABLE_MODEL_PAST[];


/*********************************
 * Definition of Public Function.
 */
CDBhandler::CDBhandler( void )
: _m_db_future_(DB_NAME_FUTURE, TABLE_MODEL_FUTURE),
  _m_db_now_(DB_NAME_NOW, TABLE_MODEL_NOW),
  _m_db_past_(DB_NAME_PAST, TABLE_MODEL_PAST) {
    try {
        std::vector<std::string> db_list;

        db_list.push_back(DB_NAME_FUTURE);
        db_list.push_back(DB_NAME_NOW);
        db_list.push_back(DB_NAME_PAST);
        db_pkg::IDBsqlite3::regist_all_of_db( db_list );

        ;   // TODO
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
    }
}

CDBhandler::~CDBhandler( void ) {
    ;   // TODO
}

bool CDBhandler::init( void ) {
    try {
        ;   // TODO

        _m_db_future_.start();
        _m_db_now_.start();
        _m_db_past_.start();
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
}

/*********************************
 * Definition of Private Function.
 */
void CDBhandler::clear( void ) {
    ;   // TODO
}



}   // db