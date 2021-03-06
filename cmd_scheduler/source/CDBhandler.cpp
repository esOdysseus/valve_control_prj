#include <CDBhandler.h>
#include <Principle6.h>
#include <time_kes.h>

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


// define KEYs
#define KEYS_FUTURE_EVENT   KEY_UUID","KEY_WHO","KEY_WHEN_TEXT","KEY_WHEN","   \
                            KEY_WHERE","KEY_WHAT","KEY_HOW","KEY_PAYLOAD
#define KEYS_FUTURE_PERIOD  KEY_UUID","KEY_WHO","KEY_PERIOD_TYPE","    \
                            KEY_PERIOD","KEY_FIRSTWHEN","KEY_WHEN","KEY_WHERE","    \
                            KEY_WHAT","KEY_HOW","KEY_PAYLOAD
#define KEYS_NOW_EVENT      KEYS_FUTURE_EVENT","KEY_MSG_ID","KEY_STATE
#define KEYS_PAST_EVENT     KEYS_FUTURE_EVENT","KEY_MSG_ID","KEY_STATE

const std::map<CDBhandler::Tkey, std::string> CDBhandler::_gm_key_names_all_ = {
    {CDBhandler::Tkey::ENUM_ID, KEY_ID},
    {CDBhandler::Tkey::ENUM_UUID, KEY_UUID},
    {CDBhandler::Tkey::ENUM_WHO, KEY_WHO},
    {CDBhandler::Tkey::ENUM_WHEN_TEXT, KEY_WHEN_TEXT},
    {CDBhandler::Tkey::ENUM_WHEN, KEY_WHEN},
    {CDBhandler::Tkey::ENUM_WHERE, KEY_WHERE},
    {CDBhandler::Tkey::ENUM_WHAT, KEY_WHAT},
    {CDBhandler::Tkey::ENUM_HOW, KEY_HOW},
    {CDBhandler::Tkey::ENUM_PAYLOAD, KEY_PAYLOAD},

    {CDBhandler::Tkey::ENUM_FIRSTWHEN, KEY_FIRSTWHEN},
    {CDBhandler::Tkey::ENUM_MSG_ID, KEY_MSG_ID},
    {CDBhandler::Tkey::ENUM_PERIOD, KEY_PERIOD},
    {CDBhandler::Tkey::ENUM_PERIOD_TYPE, KEY_PERIOD_TYPE},
    {CDBhandler::Tkey::ENUM_STATE, KEY_STATE}
};

const std::map<CDBhandler::Ttype, std::map<CDBhandler::Tkey, std::string>> CDBhandler::_gm_key_names_ = {
    {CDBhandler::Ttype::ENUM_FUTURE, {
                                        {CDBhandler::Tkey::ENUM_ID, KEY_ID},
                                        {CDBhandler::Tkey::ENUM_UUID, KEY_UUID},
                                        {CDBhandler::Tkey::ENUM_WHO, KEY_WHO},
                                        {CDBhandler::Tkey::ENUM_WHEN_TEXT, KEY_WHEN_TEXT},
                                        {CDBhandler::Tkey::ENUM_WHEN, KEY_WHEN},
                                        {CDBhandler::Tkey::ENUM_WHERE, KEY_WHERE},
                                        {CDBhandler::Tkey::ENUM_WHAT, KEY_WHAT},
                                        {CDBhandler::Tkey::ENUM_HOW, KEY_HOW},
                                        {CDBhandler::Tkey::ENUM_PAYLOAD, KEY_PAYLOAD}
                                    }
    },
    {CDBhandler::Ttype::ENUM_NOW, std::map<CDBhandler::Tkey, std::string>()},
    {CDBhandler::Ttype::ENUM_PAST, std::map<CDBhandler::Tkey, std::string>()}
};


/*********************************
 * Definition of Public Function.
 */
CDBhandler::CDBhandler( void ) {
    try {
        std::vector<std::string> db_list;

        // Create DBs
        _mm_db_[Ttype::ENUM_FUTURE] = std::make_shared<db_pkg::CDBsqlite>(DB_NAME_FUTURE, TABLE_MODEL_FUTURE);
        _mm_db_[Ttype::ENUM_NOW] = std::make_shared<db_pkg::CDBsqlite>(DB_NAME_NOW, TABLE_MODEL_NOW);
        _mm_db_[Ttype::ENUM_PAST] = std::make_shared<db_pkg::CDBsqlite>(DB_NAME_PAST, TABLE_MODEL_PAST);
        if( _mm_db_[Ttype::ENUM_FUTURE].get() == NULL || _mm_db_[Ttype::ENUM_NOW].get() == NULL || _mm_db_[Ttype::ENUM_PAST].get() == NULL ) {
            throw std::runtime_error("CDBsqlite memory-allocation is failed.");
        }

        // Register DBs
        db_list.push_back(DB_NAME_FUTURE);
        db_list.push_back(DB_NAME_NOW);
        db_list.push_back(DB_NAME_PAST);
        db_pkg::CDBsqlite::regist_all_of_db( db_list );

        // Register DB-record handler
        regist_handlers_4_context_maker();

        // Start DBs
        for( auto itr=_mm_db_.begin(); itr!=_mm_db_.end(); itr++ ) {
            itr->second->start();
        }
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
}

CDBhandler::~CDBhandler( void ) { 
    _mm_db_.clear();
    _mm_make_context_4ins_.clear();
    _mm_make_context_4con_.clear();
}

bool CDBhandler::convert_record_to_event(Ttype db_type, Trecord& record, double when) {
    bool res = false;

    try {
        std::string uuid;
        Trecord new_record;
        if( db_type != Ttype::ENUM_FUTURE ) {
            std::string err = "NOT Supported DataBase Type. (" + std::to_string(static_cast<uint8_t>(db_type)) + ")";
            throw std::out_of_range(err);
        }

        if( record.size() == 0 ) {
            throw std::invalid_argument("record have not any keys. It's empty.");
        }

        if( when <= 0.0 ) {
            std::string err = "'when' is invalid-value. (" + std::to_string(when) + ")";
            throw std::invalid_argument(err);
        }
    
        auto itr = _gm_key_names_.find(db_type);
        if( itr == _gm_key_names_.end() || itr->second.size() <= 0 ) {
            std::string err = "Not exist key_names in mapper of " + std::to_string(static_cast<uint8_t>(db_type));
            throw std::logic_error(err);
        }

        // extract key,value only for eventbase table from record to new_record.
        uuid = get_uuid(record);
        record[KEY_WHEN] = std::to_string(when);
        record[KEY_WHEN_TEXT] = time_pkg::CTime::print<double>(when, "%Y-%m-%d %T");
        record[KEY_UUID] = uuid + "@" + record[KEY_WHEN_TEXT];

        for( auto eitr=itr->second.begin(); eitr!=itr->second.end(); eitr++ ) {
            auto kitr = record.find( eitr->second );
            if( kitr == record.end() ) {
                std::string err = "key(" + eitr->second + ") have to exist in record. But it's not exist.";
                throw std::logic_error(err);
            }

            new_record[eitr->second] = kitr->second;
        }

        // update record by new_record.
        record.clear();
        for(auto eitr=new_record.begin(); eitr!=new_record.end(); eitr++) {
            record[eitr->first] = eitr->second;
        }
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
    return res;
}

std::shared_ptr<CDBhandler::Trecord> CDBhandler::make_base_record(std::shared_ptr<cmd::ICommand>& cmd) {
    std::shared_ptr<CDBhandler::Trecord> record;

    auto lamda_get_what = [&](void) -> std::string {
        std::string type = cmd->what().get_type();
        if( type == ::principle::CWhat::TYPE_VALVE ) {
            return type + " " + std::to_string( cmd->what().valve_which() );
        } else if( type == ::principle::CWhat::TYPE_DB ) {
            return type + " " + ::principle::type_convert( cmd->what().db_type() );
        } else {
            std::string err = "Not Supported What-Type=" + type;
            throw std::logic_error(err);
        }
    };

    auto lamda_get_how = [&](void) -> std::string {
        std::string type = cmd->how().get_type();
        if( type == ::principle::CHow::TYPE_VALVE ) {
            return type + " " + ::principle::type_convert( cmd->how().valve_method_pre() );
        } else if( type == ::principle::CHow::TYPE_DB ) {
            return type + " " + ::principle::type_convert( cmd->how().db_method() );
        } else {
            std::string err = "Not Supported How-Type=" + type;
            throw std::logic_error(err);
        }
    };

    try {
        if( cmd.get() == NULL ) {
            throw std::invalid_argument("cmd is NULL. please check it.");
        }

        record = std::make_shared<Trecord>();
        if( record.get() == NULL ) {
            throw std::runtime_error("record memory-allocation is failed.");
        }

        // make base record. (It's depend on EventTable.)
        (*record)[KEY_WHO] = cmd->who().get_app() + "/" + cmd->who().get_pvd();
        (*record)[KEY_WHEN] = std::to_string(cmd->when().get_start_time());
        (*record)[KEY_WHEN_TEXT] = cmd->when().get_date() + " " + cmd->when().get_time();
        (*record)[KEY_WHERE] = cmd->where().get_type();
        (*record)[KEY_WHAT] = lamda_get_what();
        (*record)[KEY_HOW] = lamda_get_how();
        (*record)[KEY_PAYLOAD] = cmd->get_payload();
        (*record)[KEY_UUID] = (*record)[KEY_WHO] + "@" + (*record)[KEY_WHEN_TEXT] + "@" + (*record)[KEY_WHERE] + "@" + (*record)[KEY_WHAT] + "@" + (*record)[KEY_HOW];
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }

    return record;
}

/// Setter
void CDBhandler::insert_record(Ttype db_type, const char* table_name, std::shared_ptr<Trecord>& record) {
    try {
        std::string table;
        std::string context;

        if( table_name == NULL ) {
            throw std::invalid_argument("Table Name is NULL.");
        }

        if( record.get() == NULL ) {
            throw std::invalid_argument("Record is NULL");
        }

        table = std::string(table_name);
        auto& db_inst = get_db_instance(db_type);
        auto itr = _mm_make_context_4ins_.find(db_type);
        if( itr == _mm_make_context_4ins_.end() ) {
            std::string err = "db_type(" + std::to_string(static_cast<uint8_t>(db_type)) + ") is not exist in _mm_make_context_4ins_";
            throw std::logic_error(err);
        }

        auto itr_maker = itr->second.find(table);
        if( itr_maker == itr->second.end() ) {
            std::string err = "table(" + table + ") is not exist in _mm_make_context_4ins_";
            throw std::logic_error(err);
        }

        std::shared_ptr<cmd::ICommand> dumy;
        context = itr_maker->second(dumy, record);
        db_inst->query_insert(table + context);
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
}

void CDBhandler::insert_record(Ttype db_type, const char* table_name, std::shared_ptr<cmd::ICommand>& cmd) {
    try {
        std::string table;
        std::string context;

        if( table_name == NULL ) {
            throw std::invalid_argument("Table Name is NULL.");
        }

        if( cmd.get() == NULL ) {
            throw std::invalid_argument("Command is NULL");
        }

        table = std::string(table_name);
        auto& db_inst = get_db_instance(db_type);
        auto itr = _mm_make_context_4ins_.find(db_type);
        if( itr == _mm_make_context_4ins_.end() ) {
            std::string err = "db_type(" + std::to_string(static_cast<uint8_t>(db_type)) + ") is not exist in _mm_make_context_4ins_";
            throw std::logic_error(err);
        }

        auto itr_maker = itr->second.find(table);
        if( itr_maker == itr->second.end() ) {
            std::string err = "table(" + table + ") is not exist in _mm_make_context_4ins_";
            throw std::logic_error(err);
        }

        std::shared_ptr<Trecord> dumy;
        context = itr_maker->second(cmd, dumy);
        db_inst->query_insert(table + context);
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
}

void CDBhandler::remove_record(Ttype db_type, const char* table_name, std::shared_ptr<Trecord>& record) {
    try {
        std::string uuid;
        auto itr = record->find(KEY_UUID);
        if( itr == record->end() ) {
            throw std::invalid_argument("Not Exist UUID key in record.");
        }

        uuid = itr->second;
        remove_record(db_type, table_name, uuid);
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
}

void CDBhandler::remove_record(Ttype db_type, const char* table_name, const std::string uuid) {
    try {
        std::string table;
        std::string context;

        if( table_name == NULL ) {
            throw std::invalid_argument("Table Name is NULL.");
        }

        if( uuid.empty() == true ) {
            throw std::invalid_argument("uuid is NULL");
        }

        table = std::string(table_name);
        auto& db_inst = get_db_instance(db_type);
        context = table + " WHERE " KEY_UUID " = '" + uuid + "'";

        // If exist context in DB, then remove it.
        if( db_inst->query_delete( context ) != SQLITE_OK ) {
            std::string err = "query_delete is failed. (DELETE FROM " + context + ")";
            throw std::runtime_error(err);
        }
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
}

/// Getter
std::shared_ptr<CDBhandler::TVrecord> CDBhandler::get_records(Ttype db_type, const char* table_name, 
                                                              TFPcond& conditioner, TFPconvert convertor, 
                                                              std::shared_ptr<TVrecord> records) {
    try {
        std::string table;
        std::string context;

        if( table_name == NULL ) {
            throw std::invalid_argument("Table Name is NULL.");
        }

        if( conditioner == nullptr ) {
            throw std::invalid_argument("conditioner function-pointer is NULL");
        }

        table = std::string(table_name);
        auto& db_inst = get_db_instance(db_type);
        auto itr = _mm_make_context_4con_.find(db_type);
        if( itr == _mm_make_context_4con_.end() ) {
            std::string err = "db_type(" + std::to_string(static_cast<uint8_t>(db_type)) + ") is not exist in _mm_make_context_4con_";
            throw std::logic_error(err);
        }

        auto itr_maker = itr->second.find(table);
        if( itr_maker == itr->second.end() ) {
            std::string err = "table(" + table + ") is not exist in _mm_make_context_4con_";
            throw std::logic_error(err);
        }

        if( records.get() == NULL ) {
            records = std::make_shared<TVrecord>();
            if( records.get() == NULL ) {
                throw std::runtime_error("records Vector memory-allocation is failed.");
            }
        }

        db_pkg::IDBsqlite3::TCBselect lamda_func = [&records, &convertor, &db_type](std::shared_ptr<Trecord>& record) -> void {
            if( convertor != nullptr ) {
                auto itr_ele = record->find(std::string(KEY_PAYLOAD));
                if( itr_ele == record->end() ) {
                    throw std::logic_error("NOT Exist KEY_PAYLOAD in record.");
                }

                convertor(db_type, *record, itr_ele->second);
            }

            LOGD("push record to record-vector.");
            records->push_back(record);
        };

        context = itr_maker->second(conditioner);
        if( db_inst->query_select( "* FROM " + table + " WHERE " + context, lamda_func ) != SQLITE_OK ) {
            std::string err = "query_select is failed: SELECT * FROM " + table + " WHERE " + context;
            throw std::logic_error(err);
        }
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
    
    return records;
}

std::shared_ptr<alias::CAlias> CDBhandler::get_who(const Trecord& record) {
    std::shared_ptr<alias::CAlias> who;

    try {
        std::string who_str = get_record_data(record, KEY_WHO);
        who = std::make_shared<alias::CAlias>(who_str);
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
    return who;
}

std::string CDBhandler::get_payload(const Trecord& record) {
    try {
        return get_record_data(record, KEY_PAYLOAD);
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
}

std::string CDBhandler::get_uuid(const Trecord& record) {
    try {
        return get_record_data(record, KEY_UUID);
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
}


/*********************************
 * Definition of Private Function.
 */
std::string CDBhandler::get_record_data( const Trecord& record, const std::string key ) {
    try {
        auto itr = record.find( key );
        if( itr == record.end() ) {
            std::string err = key + " is not exist in record.";
            throw std::logic_error(err);
        }

        return itr->second;
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
}

std::shared_ptr<db_pkg::CDBsqlite>& CDBhandler::get_db_instance( Ttype db_type ) {
    try {
        auto itr = _mm_db_.find(db_type);
        if( itr == _mm_db_.end() ) {
            std::string err = "db_type(" + std::to_string(static_cast<uint8_t>(db_type)) + ") is not Created DB type.";
            throw std::invalid_argument(err);
        }

        return itr->second;
    }
    catch ( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
}

void CDBhandler::regist_handlers_4_context_maker(void) {
    try {
        /* Regist Insert-Parameter-Context maker */
        auto lamda_future_event = [this](std::shared_ptr<cmd::ICommand> cmd, std::shared_ptr<Trecord> record) -> std::string {
            std::string context = "(" KEYS_FUTURE_EVENT ") VALUES(";
            LOGD("lamda_future_event");
            try {
                if( cmd.get() != NULL ) {
                    record = make_base_record(cmd);
                }

                if( record.get() == NULL ) {
                    throw std::logic_error("lamda_future_event: We need record parameter.");
                }

                // check record Key-List.
                if( record->find(KEY_UUID) == record->end() 
                    || record->find(KEY_WHO) == record->end() 
                    || record->find(KEY_WHEN_TEXT) == record->end() 
                    || record->find(KEY_WHEN) == record->end() 
                    || record->find(KEY_WHERE) == record->end() 
                    || record->find(KEY_WHAT) == record->end() 
                    || record->find(KEY_HOW) == record->end() 
                    || record->find(KEY_PAYLOAD) == record->end() 
                    ) {
                    throw std::logic_error("lamda_future_event: There is not exist MANDATORY KEY in record.");
                }

                context = context + "'" + (*record)[KEY_UUID] + "',"  \
                                  + "'" + (*record)[KEY_WHO] + "',"  \
                                  + "'" + (*record)[KEY_WHEN_TEXT] + "',"  \
                                  + (*record)[KEY_WHEN] + ","  \
                                  + "'" + (*record)[KEY_WHERE] + "',"  \
                                  + "'" + (*record)[KEY_WHAT] + "',"  \
                                  + "'" + (*record)[KEY_HOW] + "',"  \
                                  + "'" + (*record)[KEY_PAYLOAD] + "'"  \
                                  + ")";
            }
            catch( const std::exception& e ) {
                LOGERR("%s", e.what());
                throw e;
            }
            return context;
        };

        auto lamda_future_period = [this](std::shared_ptr<cmd::ICommand> cmd, std::shared_ptr<Trecord> record) -> std::string {
            std::string context = "(" KEYS_FUTURE_PERIOD ") VALUES(";
            LOGD("lamda_future_period");
            try {
                if( cmd.get() != NULL ) {
                    uint32_t period;
                    record = make_base_record(cmd);

                    std::string period_type = cmd->when().get_type();
                    if( period_type != principle::CWhen::TYPE_ROUTINE_DAY && period_type != principle::CWhen::TYPE_ROUTINE_WEEK ) {
                        throw std::logic_error("lamda_future_period: This CMD have not Period-type WHEN.");
                    }

                    period = cmd->when().get_period();
                    if( period_type == principle::CWhen::TYPE_ROUTINE_WEEK ) {
                        period *= 7;
                    }

                    (*record)[KEY_PERIOD_TYPE] = period_type;
                    (*record)[KEY_PERIOD] = std::to_string(period);
                }

                if( record.get() == NULL ) {
                    throw std::logic_error("lamda_future_period: We need record parameter.");
                }

                // check record Key-List.
                if( record->find(KEY_UUID) == record->end() 
                    || record->find(KEY_WHO) == record->end() 
                    || record->find(KEY_PERIOD_TYPE) == record->end() 
                    || record->find(KEY_PERIOD) == record->end() 
                    || record->find(KEY_WHEN_TEXT) == record->end() 
                    || record->find(KEY_WHEN) == record->end() 
                    || record->find(KEY_WHERE) == record->end() 
                    || record->find(KEY_WHAT) == record->end() 
                    || record->find(KEY_HOW) == record->end() 
                    || record->find(KEY_PAYLOAD) == record->end() 
                    ) {
                    throw std::logic_error("lamda_future_period: There is not exist MANDATORY KEY in record.");
                }

                context = context + "'" + (*record)[KEY_UUID] + "',"  \
                                  + "'" + (*record)[KEY_WHO] + "',"  \
                                  + "'" + (*record)[KEY_PERIOD_TYPE] + "',"  \
                                  + (*record)[KEY_PERIOD] + ","  \
                                  + "'" + (*record)[KEY_WHEN_TEXT] + "',"  \
                                  + (*record)[KEY_WHEN] + ","  \
                                  + "'" + (*record)[KEY_WHERE] + "',"  \
                                  + "'" + (*record)[KEY_WHAT] + "',"  \
                                  + "'" + (*record)[KEY_HOW] + "',"  \
                                  + "'" + (*record)[KEY_PAYLOAD] + "'"  \
                                  + ")";
            }
            catch( const std::exception& e ) {
                LOGERR("%s", e.what());
                throw e;
            }
            return context;
        };

        auto lamda_now_event = [this](std::shared_ptr<cmd::ICommand> cmd, std::shared_ptr<Trecord> record) -> std::string {
            std::string context = "(" KEYS_NOW_EVENT ") VALUES(";
            LOGD("lamda_now_event");
            try {
                if( record.get() == NULL ) {
                    throw std::logic_error("lamda_now_event: We need record parameter.");
                }

                if( cmd.get() != NULL ) {
                    throw std::logic_error("lamda_now_event: Not support CMD-parameter case.");
                }

                // check record Key-List.
                if( record->find(KEY_UUID) == record->end() 
                    || record->find(KEY_WHO) == record->end() 
                    || record->find(KEY_WHEN_TEXT) == record->end() 
                    || record->find(KEY_WHEN) == record->end() 
                    || record->find(KEY_WHERE) == record->end() 
                    || record->find(KEY_WHAT) == record->end() 
                    || record->find(KEY_HOW) == record->end() 
                    || record->find(KEY_PAYLOAD) == record->end() 
                    || record->find(KEY_MSG_ID) == record->end() 
                    || record->find(KEY_STATE) == record->end() 
                    ) {
                    throw std::logic_error("lamda_now_event: There is not exist MANDATORY KEY in record.");
                }

                context = context + "'" + (*record)[KEY_UUID] + "',"  \
                                  + "'" + (*record)[KEY_WHO] + "',"  \
                                  + "'" + (*record)[KEY_WHEN_TEXT] + "',"  \
                                  + (*record)[KEY_WHEN] + ","  \
                                  + "'" + (*record)[KEY_WHERE] + "',"  \
                                  + "'" + (*record)[KEY_WHAT] + "',"  \
                                  + "'" + (*record)[KEY_HOW] + "',"  \
                                  + "'" + (*record)[KEY_PAYLOAD] + "',"  \
                                  + (*record)[KEY_MSG_ID] + ","  \
                                  + "'" + (*record)[KEY_STATE] + "'"  \
                                  + ")";
            }
            catch( const std::exception& e ) {
                LOGERR("%s", e.what());
                throw e;
            }
            return context;
        };

        auto lamda_past_event = [this](std::shared_ptr<cmd::ICommand> cmd, std::shared_ptr<Trecord> record) -> std::string {
            std::string context = "(" KEYS_PAST_EVENT ") VALUES(";
            LOGD("lamda_past_event");
            try {
                if( record.get() == NULL ) {
                    throw std::logic_error("lamda_past_event: We need record parameter.");
                }

                if( cmd.get() != NULL ) {
                    throw std::logic_error("lamda_past_event: Not support CMD-parameter case.");
                }

                // check record Key-List.
                if( record->find(KEY_UUID) == record->end() 
                    || record->find(KEY_WHO) == record->end() 
                    || record->find(KEY_WHEN_TEXT) == record->end() 
                    || record->find(KEY_WHEN) == record->end() 
                    || record->find(KEY_WHERE) == record->end() 
                    || record->find(KEY_WHAT) == record->end() 
                    || record->find(KEY_HOW) == record->end() 
                    || record->find(KEY_PAYLOAD) == record->end() 
                    || record->find(KEY_MSG_ID) == record->end() 
                    || record->find(KEY_STATE) == record->end() 
                    ) {
                    throw std::logic_error("lamda_past_event: There is not exist MANDATORY KEY in record.");
                }

                context = context + "'" + (*record)[KEY_UUID] + "',"  \
                                  + "'" + (*record)[KEY_WHO] + "',"  \
                                  + "'" + (*record)[KEY_WHEN_TEXT] + "',"  \
                                  + (*record)[KEY_WHEN] + ","  \
                                  + "'" + (*record)[KEY_WHERE] + "',"  \
                                  + "'" + (*record)[KEY_WHAT] + "',"  \
                                  + "'" + (*record)[KEY_HOW] + "',"  \
                                  + "'" + (*record)[KEY_PAYLOAD] + "',"  \
                                  + (*record)[KEY_MSG_ID] + ","  \
                                  + "'" + (*record)[KEY_STATE] + "'"  \
                                  + ")";
            }
            catch( const std::exception& e ) {
                LOGERR("%s", e.what());
                throw e;
            }
            return context;
        };

        _mm_make_context_4ins_[Ttype::ENUM_FUTURE][std::string(DB_TABLE_EVENT)] = lamda_future_event;
        _mm_make_context_4ins_[Ttype::ENUM_FUTURE][std::string(DB_TABLE_PERIOD)] = lamda_future_period;
        _mm_make_context_4ins_[Ttype::ENUM_NOW][std::string(DB_TABLE_EVENT)] = lamda_now_event;
        _mm_make_context_4ins_[Ttype::ENUM_PAST][std::string(DB_TABLE_EVENT)] = lamda_past_event;


        /* Regist Condition-Context maker */
        TCondHandler lamda_future_event_cond = [this](TFPcond& handler) -> std::string {
            std::map<Tkey, std::string> kopt;
            kopt[Tkey::ENUM_WHEN_TEXT] = KEY_WHEN_TEXT;
            return handler(KEY_WHO, KEY_WHEN, KEY_WHERE, KEY_WHAT, KEY_HOW, KEY_UUID, kopt);
        };

        TCondHandler lamda_future_period_cond = [this](TFPcond& handler) -> std::string {
            std::map<Tkey, std::string> kopt;
            kopt[Tkey::ENUM_PERIOD_TYPE] = KEY_PERIOD_TYPE;
            kopt[Tkey::ENUM_PERIOD] = KEY_PERIOD;
            kopt[Tkey::ENUM_FIRSTWHEN] = KEY_FIRSTWHEN;
            return handler(KEY_WHO, KEY_WHEN, KEY_WHERE, KEY_WHAT, KEY_HOW, KEY_UUID, kopt);
        };

        TCondHandler lamda_now_event_cond = [this](TFPcond& handler) -> std::string {
            std::map<Tkey, std::string> kopt;
            kopt[Tkey::ENUM_WHEN_TEXT] = KEY_WHEN_TEXT;
            kopt[Tkey::ENUM_MSG_ID] = KEY_MSG_ID;
            kopt[Tkey::ENUM_STATE] = KEY_STATE;
            return handler(KEY_WHO, KEY_WHEN, KEY_WHERE, KEY_WHAT, KEY_HOW, KEY_UUID, kopt);
        };

        TCondHandler lamda_past_event_cond = [this](TFPcond& handler) -> std::string {
            std::map<Tkey, std::string> kopt;
            kopt[Tkey::ENUM_WHEN_TEXT] = KEY_WHEN_TEXT;
            kopt[Tkey::ENUM_MSG_ID] = KEY_MSG_ID;
            kopt[Tkey::ENUM_STATE] = KEY_STATE;
            return handler(KEY_WHO, KEY_WHEN, KEY_WHERE, KEY_WHAT, KEY_HOW, KEY_UUID, kopt);
        };

        _mm_make_context_4con_[Ttype::ENUM_FUTURE][std::string(DB_TABLE_EVENT)] = lamda_future_event_cond;
        _mm_make_context_4con_[Ttype::ENUM_FUTURE][std::string(DB_TABLE_PERIOD)] = lamda_future_period_cond;
        _mm_make_context_4con_[Ttype::ENUM_NOW][std::string(DB_TABLE_EVENT)] = lamda_now_event_cond;
        _mm_make_context_4con_[Ttype::ENUM_PAST][std::string(DB_TABLE_EVENT)] = lamda_past_event_cond;
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
}


template std::string CDBhandler::convert_string<int8_t>( int8_t value );
template std::string CDBhandler::convert_string<int16_t>( int16_t value );
template std::string CDBhandler::convert_string<int32_t>( int32_t value );
template std::string CDBhandler::convert_string<uint8_t>( uint8_t value );
template std::string CDBhandler::convert_string<uint16_t>( uint16_t value );
template std::string CDBhandler::convert_string<uint32_t>( uint32_t value );
template std::string CDBhandler::convert_string<double>( double value );
template std::string CDBhandler::convert_string<float>( float value );

template<>
std::string CDBhandler::convert_string(std::string value) {
    return value;
}

template<>
std::string CDBhandler::convert_string(Tstate value) {
    switch( value ) {
    case Tstate::ENUM_TRIG:
        return "TRIGGERED";
    case Tstate::ENUM_RCV_ACK:
        return "RCV_ACK";
    case Tstate::ENUM_STARTED:
        return "STARTED";
    case Tstate::ENUM_DONE:
        return "DONE";
    case Tstate::ENUM_FAIL:
        return "FAIL";
    default:
        {
            std::string err = "Not Supported Tstate(" + std::to_string(static_cast<uint32_t>(value)) + ").";
            throw std::out_of_range(err);
        }
    }
}

template<typename T>
std::string CDBhandler::convert_string(T value) {
    return std::to_string(value);
}

std::string CDBhandler::get_value_context(Tkey key, std::string& value) {
    try {
        switch( key ) {
        case Tkey::ENUM_ID:
        case Tkey::ENUM_MSG_ID:
        case Tkey::ENUM_PERIOD:
        case Tkey::ENUM_WHEN:
            return value;
        case Tkey::ENUM_UUID:
        case Tkey::ENUM_FIRSTWHEN:
        case Tkey::ENUM_HOW:
        case Tkey::ENUM_PAYLOAD:
        case Tkey::ENUM_PERIOD_TYPE:
        case Tkey::ENUM_STATE:
        case Tkey::ENUM_WHAT:
        case Tkey::ENUM_WHEN_TEXT:
        case Tkey::ENUM_WHERE:
        case Tkey::ENUM_WHO:
            return "'" + value + "'";
        default:
            {
                std::string err = "Not Supported key(" + std::to_string(static_cast<uint16_t>(key)) + ")";
                throw std::out_of_range(err);
            }
        }
    }
    catch ( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
}

void CDBhandler::update_record_raw(Ttype db_type, const char* table_name, 
                                   Tkey cond_key, std::string& cond_val, 
                                   Tkey target_key, std::string& target_val) {
    try {
        std::string table;
        std::string context;
        std::string key_cond;
        std::string key_tar;

        if( table_name == NULL ) {
            throw std::invalid_argument("Table Name is NULL.");
        }

        if( cond_val.empty() == true ) {
            throw std::invalid_argument("Condition-value is NULL");
        }

        table = std::string(table_name);
        auto& db_inst = get_db_instance(db_type);

        // make context
        key_cond = _gm_key_names_all_.find(cond_key)->second;
        key_tar = _gm_key_names_all_.find(target_key)->second;
        context = table + " SET " + key_tar + " = " + target_val + " WHERE " + key_cond + " = " + cond_val;
        
        // query context
        if( db_inst->query_update( context ) != SQLITE_OK ) {
            std::string err = "query_update is failed. (UPDATE " + context + ")";
            throw std::runtime_error(err);
        }
    }
    catch ( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
}



}   // db