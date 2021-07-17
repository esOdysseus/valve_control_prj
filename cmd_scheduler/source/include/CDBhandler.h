#ifndef _CLASS_DATABASE_HANDLER_H_
#define _CLASS_DATABASE_HANDLER_H_

#include <memory>
#include <string>

#include <CDBsqlite.h>

namespace db {


class CDBhandler {
public:
    CDBhandler( void );

    ~CDBhandler( void );

    bool init( void );

private:
    CDBhandler(const CDBhandler&) = delete;             // copy constructor
    CDBhandler& operator=(const CDBhandler&) = delete;  // copy operator
    CDBhandler(CDBhandler&&) = delete;                  // move constructor
    CDBhandler& operator=(CDBhandler&&) = delete;       // move operator

    void clear( void );

private:
    db_pkg::CDBsqlite _m_db_future_;
    db_pkg::CDBsqlite _m_db_now_;
    db_pkg::CDBsqlite _m_db_past_;

    static constexpr const char * DB_NAME_FUTURE = "db_future.db";
    static constexpr const char * DB_NAME_NOW = "db_now.db";
    static constexpr const char * DB_NAME_PAST = "db_past.db";

    static constexpr const char * TABLE_MODEL_FUTURE[] = {
        "EventBase( \
            id          INTEGER     PRIMARY KEY     AUTOINCREMENT       NOT NULL,   \
            timestamp   TEXT        UNIQUE                              NOT NULL,   \
            cmd         TEXT                                            NOT NULL    \
        )",
        "PeriodBase( \
            id          INTEGER     PRIMARY KEY     AUTOINCREMENT       NOT NULL,   \
            timestamp   TEXT        UNIQUE                              NOT NULL,   \
            cmd         TEXT                                            NOT NULL    \
        )",
        NULL
    };
    static constexpr const char * TABLE_MODEL_NOW[] = {
        "EventBase( \
            id          INTEGER     PRIMARY KEY     AUTOINCREMENT       NOT NULL,   \
            timestamp   TEXT        UNIQUE                              NOT NULL,   \
            cmd         TEXT                                            NOT NULL,   \
            state       TEXT                                            NOT NULL    \
        )",
        NULL
    };
    static constexpr const char * TABLE_MODEL_PAST[] = {
        "EventBase( \
            id          INTEGER     PRIMARY KEY     AUTOINCREMENT       NOT NULL,   \
            timestamp   TEXT        UNIQUE                              NOT NULL,   \
            cmd         TEXT                                            NOT NULL,   \
            state       TEXT                                            NOT NULL    \
        )",
        NULL
    };

};


}   // namespace db


#endif // _CLASS_DATABASE_HANDLER_H_