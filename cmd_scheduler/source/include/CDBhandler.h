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
    using TVrecord = db_pkg::CDBsqlite::TVrecord;
    using Tstate = enum class enum_state: uint8_t { ENUM_TRIG=1, ENUM_RCV_ACK=2, ENUM_STARTED=3, ENUM_DONE=4, ENUM_FAIL=5 };
    using Ttype = enum class enum_db_type: uint8_t { ENUM_FUTURE=1, ENUM_NOW=2, ENUM_PAST=3 };
    using Tkey = enum class enum_key_type: uint16_t {
        ENUM_MSG_ID=1,
        ENUM_STATE=2,
        ENUM_PERIOD=3,
        ENUM_PERIOD_TYPE=4,
        ENUM_FIRSTWHEN=5,
        ENUM_WHEN_TEXT=6,

        ENUM_ID,
        ENUM_UUID,
        ENUM_WHO,
        ENUM_WHEN,
        ENUM_WHERE,
        ENUM_WHAT,
        ENUM_HOW,
        ENUM_PAYLOAD
    };

    // Function Pointer type.
    using TFPselect = db_pkg::CDBsqlite::TCBselect;
    using TFPcond = std::function<std::string (std::string /*key who*/, std::string /*key when*/, 
                                               std::string /*key where*/, std::string /*key what*/, 
                                               std::string /*key how*/, std::string /*key uuid*/,
                                               std::map<Tkey, std::string>& /*Option keys*/)>;
    using TFPconvert = std::function<void (Ttype /*db_type*/, Trecord& /*record*/, std::string& /*payload*/)>;

private:
    using TRecordHandler = std::function<std::string(std::shared_ptr<cmd::ICommand> /*cmd*/, std::shared_ptr<Trecord> /*record*/)>;
    using TCondHandler = std::function<std::string(TFPcond&)>;
    using TMrHandler = std::map<std::string /*table-name*/, TRecordHandler>;
    using TMcHandler = std::map<std::string /*table-name*/, TCondHandler>;

public:
    CDBhandler( void );

    ~CDBhandler( void );

    bool convert_record_to_event(Ttype db_type, Trecord& record, double when);

    std::shared_ptr<Trecord> make_base_record(std::shared_ptr<cmd::ICommand>& cmd);

    template<typename T>
    static void append(std::shared_ptr<Trecord>& record, Tkey key, T value) {
        std::string s_key = _gm_key_names_all_.find(key)->second;
        (*record)[ s_key ] = convert_string<T>(value);
    }

    // setter
    void insert_record(Ttype db_type, const char* table_name, std::shared_ptr<Trecord>& record);

    void insert_record(Ttype db_type, const char* table_name, std::shared_ptr<cmd::ICommand>& cmd);

    void remove_record(Ttype db_type, const char* table_name, std::shared_ptr<Trecord>& record);

    void remove_record(Ttype db_type, const char* table_name, const std::string uuid);

    template<typename TC, typename TU>
    void update_record(Ttype db_type, const char* table_name, 
                       Tkey cond_key, TC cond_val, 
                       Tkey update_key, TU update_val) {
        std::string scond_val = convert_string<TC>( cond_val );
        std::string supdate_val = convert_string<TU>( update_val );

        scond_val = get_value_context(cond_key, scond_val);
        supdate_val = get_value_context(update_key, supdate_val);

        update_record_raw(db_type, table_name, cond_key, scond_val, update_key, supdate_val);
    }

    // getter
    std::shared_ptr<TVrecord> get_records(Ttype db_type, const char* table_name, 
                                          TFPcond& conditioner, TFPconvert convertor, 
                                          std::shared_ptr<TVrecord> records=std::shared_ptr<TVrecord>());

    static std::shared_ptr<alias::CAlias> get_who(const Trecord& record);

    static std::string get_payload(const Trecord& record);

    static std::string get_uuid(const Trecord& record);

private:
    CDBhandler(const CDBhandler&) = delete;             // copy constructor
    CDBhandler& operator=(const CDBhandler&) = delete;  // copy operator
    CDBhandler(CDBhandler&&) = delete;                  // move constructor
    CDBhandler& operator=(CDBhandler&&) = delete;       // move operator

    static std::string get_record_data( const Trecord& record, const std::string key );

    std::shared_ptr<db_pkg::CDBsqlite>& get_db_instance( Ttype db_type );

    void regist_handlers_4_context_maker(void);

    template<typename T>
    static std::string convert_string(T value);

    std::string get_value_context(Tkey key, std::string& value);

    void update_record_raw(Ttype db_type, const char* table_name, 
                           Tkey cond_key, std::string& cond_val, 
                           Tkey update_key, std::string& update_val);

public:
    #define TABLE_EVENT     "EventBase"
    #define TABLE_PERIOD    "PeriodBase"
    static constexpr const char * DB_TABLE_EVENT = TABLE_EVENT;
    static constexpr const char * DB_TABLE_PERIOD = TABLE_PERIOD;

private:
    /* DB related variables. */
    std::map<Ttype, std::shared_ptr<db_pkg::CDBsqlite>> _mm_db_;

    std::map<Ttype, TMrHandler>  _mm_make_context_4ins_;

    std::map<Ttype, TMcHandler> _mm_make_context_4con_;

    static const std::map<Tkey, std::string> _gm_key_names_all_;

    static const std::map<Ttype, std::map<Tkey, std::string>> _gm_key_names_;
    
    // define DB-path
    static constexpr const char * DB_NAME_FUTURE = "db_future.db";
    static constexpr const char * DB_NAME_NOW = "db_now.db";
    static constexpr const char * DB_NAME_PAST = "db_past.db";

    // define KEY
    #define KEY_ID          "id"
    #define KEY_UUID        "uuid"
    #define KEY_WHO         "who"
    #define KEY_WHEN_TEXT   "when_text"
    #define KEY_WHEN        "when_utc"
    #define KEY_FIRSTWHEN   "when_first"
    #define KEY_PERIOD_TYPE "period_type"
    #define KEY_PERIOD      "period"
    #define KEY_WHERE       "where_pos"
    #define KEY_WHAT        "what"
    #define KEY_HOW         "how"
    #define KEY_PAYLOAD     "payload"
    #define KEY_MSG_ID      "msg_id"
    #define KEY_STATE       "state"

    // define Table-Model
    #define TMODEL_EVENT    \
           KEY_ID        " INTEGER     PRIMARY KEY     AUTOINCREMENT       NOT NULL,"   \
           KEY_UUID      " TEXT        UNIQUE                              NOT NULL,"   \
           KEY_WHO       " TEXT                                            NOT NULL,"   \
           KEY_WHEN_TEXT " TEXT                                            NOT NULL,"   \
           KEY_WHEN      " REAL                                            NOT NULL,"   \
           KEY_WHERE     " TEXT                                            NOT NULL,"   \
           KEY_WHAT      " TEXT                                            NOT NULL,"   \
           KEY_HOW       " TEXT                                            NOT NULL,"   \
           KEY_PAYLOAD   " TEXT                                            NOT NULL"

    #define TMODEL_PERIOD   \
           KEY_ID           " INTEGER     PRIMARY KEY     AUTOINCREMENT       NOT NULL,"   \
           KEY_UUID         " TEXT        UNIQUE                              NOT NULL,"   \
           KEY_WHO          " TEXT                                            NOT NULL,"   \
           KEY_PERIOD_TYPE  " TEXT                                            NOT NULL,"   \
           KEY_PERIOD       " INTEGER                                         NOT NULL,"   \
           KEY_FIRSTWHEN    " TEXT                                            NOT NULL,"   \
           KEY_WHEN         " REAL                                            NOT NULL,"   \
           KEY_WHERE        " TEXT                                            NOT NULL,"   \
           KEY_WHAT         " TEXT                                            NOT NULL,"   \
           KEY_HOW          " TEXT                                            NOT NULL,"   \
           KEY_PAYLOAD      " TEXT                                            NOT NULL"

    static constexpr const char * TABLE_MODEL_FUTURE[] = {
        /***
         * who      : alias of peer.    Ex) APP-name/PVD-name
         * when-text: Data + Time       Ex) 2021-05-13 13:50:52
         * when-utc : UTC time
         * where    : position of Dynamic-who.
         * what     : target            Ex) valve-01
         * how      : operation         Ex) open
         * payload  : json-data
         * uuid     : who@when-text@where@what@how for uniqueness as ID.
         ***/
        TABLE_EVENT "(" \
            TMODEL_EVENT    \
        ")",
        /***
         * who          : alias of peer.                            Ex) APP-name/PVD-name
         * period-type  : type for value of period column.          Valid-Values)   HOUR, DAY, MONTH, YEAR
         * period       : duration for period-execution.            [Unit: day]
         * when-first   : Data + Time [first-time to execute]       Ex) 2021-05-13 13:50:52
         * when-utc     : UTC time [next-time to execute]
         * where        : position of Dynamic-who.
         * what         : target                                    Ex) valve-01
         * how          : operation                                 Ex) open
         * payload      : json-data
         * uuid         : who@when-first@where@what@how for uniqueness as ID.
         ***/
        TABLE_PERIOD "(" \
            TMODEL_PERIOD \
        ")",
        NULL
    };

    static constexpr const char * TABLE_MODEL_NOW[] = {
        /***
         * who      : alias of peer.    Ex) APP-name/PVD-name
         * when-text: Data + Time       Ex) 2021-05-13 13:50:52
         * when-utc : UTC time
         * where    : position of Dynamic-who.
         * what     : target            Ex) valve-01
         * how      : operation         Ex) open
         * payload  : json-data
         * state    : state of CMD operation    Valid-Values) TRIGGERED, RCV-ACK, STARTED, DONE, FAIL
         * msg-id   : ID of req-msg that is sent.
         * uuid     : who@when-text@where@what@how for uniqueness as ID.
         ***/
        TABLE_EVENT "(" \
            TMODEL_EVENT ","    \
            KEY_MSG_ID   " INTEGER                                         NOT NULL,"   \
            KEY_STATE    " TEXT                                            NOT NULL"    \
        ")",
        NULL
    };

    static constexpr const char * TABLE_MODEL_PAST[] = {
        /***
         * who      : alias of peer.    Ex) APP-name/PVD-name
         * when-text: Data + Time       Ex) 2021-05-13 13:50:52
         * when-utc : UTC time
         * where    : position of Dynamic-who.
         * what     : target            Ex) valve-01
         * how      : operation         Ex) open
         * payload  : json-data
         * state    : state of CMD operation    Valid-Values) TRIGGERED, RCV-ACK, STARTED, DONE, FAIL
         * msg-id   : ID of req-msg that is sent.
         * uuid     : who@when-text@where@what@how for uniqueness as ID.
         ***/
        TABLE_EVENT "(" \
            TMODEL_EVENT ","    \
            KEY_MSG_ID   " INTEGER                                         NOT NULL,"   \
            KEY_STATE    " TEXT                                            NOT NULL"    \
        ")",
        NULL
    };

};


}   // namespace db


#endif // _CLASS_DATABASE_HANDLER_H_