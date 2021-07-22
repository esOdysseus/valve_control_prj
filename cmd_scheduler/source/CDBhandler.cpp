#include <CDBhandler.h>

#include <logger.h>

namespace db {

constexpr const char * CDBhandler::DB_TABLE_EVENT;
constexpr const char * CDBhandler::DB_TABLE_PERIOD;

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
        // Register DBs
        db_list.push_back(DB_NAME_FUTURE);
        db_list.push_back(DB_NAME_NOW);
        db_list.push_back(DB_NAME_PAST);
        db_pkg::CDBsqlite::regist_all_of_db( db_list );

        // Start DBs
        _m_db_future_.start();
        _m_db_now_.start();
        _m_db_past_.start();
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
    }
}

CDBhandler::~CDBhandler( void ) {
    ;   // TODO
}

void CDBhandler::insert_record(Ttype db_type, const char* table_name, std::shared_ptr<cmd::ICommand>& cmd) {
    try {
        if( table_name == NULL ) {
            throw std::invalid_argument("Table Name is NULL.");
        }

        if( cmd.get() == NULL ) {
            throw std::invalid_argument("Command is NULL");
        }

        ;   // TODO
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
}



bool CDBhandler::convert_record_to_event(Ttype db_type, Trecord& record) {
    bool res = false;

    ;   // TODO
    return res;
}

void CDBhandler::get_records(Ttype db_type, const char* table_name, 
                             TFPcond& conditioner, TFPconvert convertor, TVrecord& records) {
    try {
        if( table_name == NULL ) {
            throw std::invalid_argument("Table Name is NULL.");
        }

        if( conditioner == nullptr ) {
            throw std::invalid_argument("conditioner function-pointer is NULL");
        }

        ;   // TODO
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
}

std::shared_ptr<alias::CAlias> CDBhandler::get_who(const Trecord& record) {
    std::shared_ptr<alias::CAlias> who;

    ;   // TODO
    return who;
}

std::string CDBhandler::get_payload(const Trecord& record) {
    std::string payload;

    ;   // TODO
    return payload;
}


/*********************************
 * Definition of Private Function.
 */
void CDBhandler::clear( void ) {
    ;   // TODO
}



}   // db