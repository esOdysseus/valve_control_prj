#ifndef _CLASS_DATABASE_HANDLER_H_
#define _CLASS_DATABASE_HANDLER_H_

#include <memory>
#include <string>
#include <functional>

#include <ICommand.h>
#include <CDBsqlite.h>
#include <Common.h>

namespace db {


class CDBhandler {
public:
    using Trecord = db_pkg::CDBsqlite::Trecord;
    using TVrecord = std::vector<std::shared_ptr<Trecord>>;
    using Ttype = enum class enum_db_type: uint8_t { ENUM_FUTURE=1, ENUM_NOW=2, ENUM_PAST=3 };

    // Function Pointer type.
    using TFPselect = db_pkg::CDBsqlite::TCBselect;
    using TFPcond = std::function<std::string (std::string /*key who*/, std::string /*key when*/, 
                                               std::string /*key where*/, std::string /*key what*/, 
                                               std::string /*key how*/, std::string /*key uuid*/)>;
    using TFPconvert = std::function<void (Ttype /*db_type*/, Trecord& /*record*/)>;

public:
    CDBhandler( void );

    ~CDBhandler( void );

    void insert_record(Ttype db_type, const char* table_name, std::shared_ptr<cmd::ICommand>& cmd);
    
    bool convert_record_to_event(Ttype db_type, Trecord& record);

    // getter
    void get_records(Ttype db_type, const char* table_name, 
                     TFPcond& conditioner, TFPconvert convertor, TVrecord& records);

    static std::shared_ptr<alias::CAlias> get_who(const Trecord& record);

    static std::string get_payload(const Trecord& record);

private:
    CDBhandler(const CDBhandler&) = delete;             // copy constructor
    CDBhandler& operator=(const CDBhandler&) = delete;  // copy operator
    CDBhandler(CDBhandler&&) = delete;                  // move constructor
    CDBhandler& operator=(CDBhandler&&) = delete;       // move operator

    void clear( void );

public:
    #define TABLE_EVENT     "EventBase"
    #define TABLE_PERIOD    "PeriodBase"
    static constexpr const char * DB_TABLE_EVENT = TABLE_EVENT;
    static constexpr const char * DB_TABLE_PERIOD = TABLE_PERIOD;

private:
    /* DB related variables. */
    db_pkg::CDBsqlite _m_db_future_;
    db_pkg::CDBsqlite _m_db_now_;
    db_pkg::CDBsqlite _m_db_past_;
    
    static constexpr const char * DB_NAME_FUTURE = "db_future.db";
    static constexpr const char * DB_NAME_NOW = "db_now.db";
    static constexpr const char * DB_NAME_PAST = "db_past.db";

    #define TMODEL_EVENT    \
           "id          INTEGER     PRIMARY KEY     AUTOINCREMENT       NOT NULL,   \
            uuid        TEXT        UNIQUE                              NOT NULL,   \
            who         TEXT                                            NOT NULL,   \
            when        TEXT                                            NOT NULL,   \
            when-utc    REAL                                            NOT NULL,   \
            where       TEXT                                            NOT NULL,   \
            what        TEXT                                            NOT NULL,   \
            how         TEXT                                            NOT NULL,   \
            payload     TEXT                                            NOT NULL"
    #define TMODEL_PERIOD   \
           "id          INTEGER     PRIMARY KEY     AUTOINCREMENT       NOT NULL,   \
            uuid        TEXT        UNIQUE                              NOT NULL,   \
            who         TEXT                                            NOT NULL,   \
            period-type TEXT                                            NOT NULL,   \
            period      INTEGER                                         NOT NULL,   \
            firsttime   TEXT                                            NOT NULL,   \
            next-utc    REAL                                            NOT NULL,   \
            where       TEXT                                            NOT NULL,   \
            what        TEXT                                            NOT NULL,   \
            how         TEXT                                            NOT NULL,   \
            payload     TEXT                                            NOT NULL"

    static constexpr const char * TABLE_MODEL_FUTURE[] = {
        /***
         * who      : alias of peer.    Ex) APP-name/PVD-name
         * when     : Data + Time       Ex) 2021-05-13 13:50:52
         * when-utc : UTC time
         * where    : position of Dynamic-who.
         * what     : target            Ex) valve-01
         * how      : operation         Ex) open
         * payload  : json-data
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
         * payload      : json-data
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
         * payload  : json-data
         * state    : state of CMD operation    Valid-Values) TRIGGERED, RCV-ACK, STARTED, DONE, FAIL
         * msg-id   : ID of req-msg that is sent.
         * uuid     : who@when@where@what@how for uniqueness as ID.
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
         * payload  : json-data
         * state    : state of CMD operation    Valid-Values) TRIGGERED, RCV-ACK, STARTED, DONE, FAIL
         * msg-id   : ID of req-msg that is sent.
         * uuid     : who@when@where@what@how for uniqueness as ID.
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