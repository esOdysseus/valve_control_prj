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
    /* DB related variables. */
    db_pkg::CDBsqlite _m_db_future_;
    db_pkg::CDBsqlite _m_db_now_;
    db_pkg::CDBsqlite _m_db_past_;

    static constexpr const char * DB_NAME_FUTURE = "db_future.db";
    static constexpr const char * DB_NAME_NOW = "db_now.db";
    static constexpr const char * DB_NAME_PAST = "db_past.db";
    
    #define TABLE_EVENT     "EventBase"
    #define TABLE_PERIOD    "PeriodBase"
    #define TMODEL_EVENT    \
           "id          INTEGER     PRIMARY KEY     AUTOINCREMENT       NOT NULL,   \
            uuid        TEXT        UNIQUE                              NOT NULL,   \
            who         TEXT                                            NOT NULL,   \
            when        TEXT                                            NOT NULL,   \
            when-utc    INTEGER                                         NOT NULL,   \
            where       TEXT                                            NOT NULL,   \
            what        TEXT                                            NOT NULL,   \
            how         TEXT                                            NOT NULL,   \
            cmd         TEXT                                            NOT NULL"
    #define TMODEL_PERIOD   \
           "id          INTEGER     PRIMARY KEY     AUTOINCREMENT       NOT NULL,   \
            uuid        TEXT        UNIQUE                              NOT NULL,   \
            who         TEXT                                            NOT NULL,   \
            period-type TEXT                                            NOT NULL,   \
            period      INTEGER                                         NOT NULL,   \
            firsttime   TEXT                                            NOT NULL,   \
            next-utc    INTEGER                                         NOT NULL,   \
            where       TEXT                                            NOT NULL,   \
            what        TEXT                                            NOT NULL,   \
            how         TEXT                                            NOT NULL,   \
            cmd         TEXT                                            NOT NULL"

    static constexpr const char * TABLE_MODEL_FUTURE[] = {
        /***
         * who      : alias of peer.    Ex) APP-name/PVD-name
         * when     : Data + Time       Ex) 2021-05-13 13:50:52
         * when-utc : UTC time
         * where    : position of Dynamic-who.
         * what     : target            Ex) valve-01
         * how      : operation         Ex) open
         * cmd      : json-data
         * uuid     : who@when@where@what@how for uniqueness as ID.
         ***/
        TABLE_EVENT "(" \
            TMODEL_EVENT    \
        ")",
        /***
         * who          : alias of peer.                            Ex) APP-name/PVD-name
         * period-type  : type for value of period column.          Valid-Values)   HOUR, DAY, MONTH, YEAR
         * period       : duration for period-execution.            [Unit: day]
         * firsttime    : Data + Time [first-time to execute]       Ex) 2021-05-13 13:50:52
         * next-utc     : UTC time [latest-time to execute]
         * where        : position of Dynamic-who.
         * what         : target                                    Ex) valve-01
         * how          : operation                                 Ex) open
         * cmd          : json-data
         * uuid         : who@firsttime@where@what@how for uniqueness as ID.
         ***/
        TABLE_PERIOD "(" \
            TMODEL_PERIOD \
        ")",
        NULL
    };

    static constexpr const char * TABLE_MODEL_NOW[] = {
        /***
         * who      : alias of peer.    Ex) APP-name/PVD-name
         * when     : Data + Time       Ex) 2021-05-13 13:50:52
         * when-utc : UTC time
         * where    : position of Dynamic-who.
         * what     : target            Ex) valve-01
         * how      : operation         Ex) open
         * cmd      : json-data
         * state    : state of CMD operation    Valid-Values) TRIGGERED, RCV-ACK, STARTED, DONE, FAIL
         * msg-id   : ID of req-msg that is sent.
         * uuid     : who@firsttime@where@what@how for uniqueness as ID.
         ***/
        TABLE_EVENT "(" \
            TMODEL_EVENT ","    \
            "msg-id      INTEGER                                         NOT NULL,"   \
            "state       TEXT                                            NOT NULL"    \
        ")",
        NULL
    };

    static constexpr const char * TABLE_MODEL_PAST[] = {
        /***
         * who      : alias of peer.    Ex) APP-name/PVD-name
         * when     : Data + Time       Ex) 2021-05-13 13:50:52
         * when-utc : UTC time
         * where    : position of Dynamic-who.
         * what     : target            Ex) valve-01
         * how      : operation         Ex) open
         * cmd      : json-data
         * state    : state of CMD operation    Valid-Values) TRIGGERED, RCV-ACK, STARTED, DONE, FAIL
         * msg-id   : ID of req-msg that is sent.
         * uuid     : who@firsttime@where@what@how for uniqueness as ID.
         ***/
        TABLE_EVENT "(" \
            TMODEL_EVENT ","    \
            "msg-id      INTEGER                                         NOT NULL,"   \
            "state       TEXT                                            NOT NULL"    \
        ")",
        NULL
    };

};


}   // namespace db


#endif // _CLASS_DATABASE_HANDLER_H_