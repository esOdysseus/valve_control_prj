#include <unistd.h>

#include <sqlite3_kes.h>
#include <logger.h>

namespace db_pkg {


std::map<std::string /*db-path*/, std::mutex /*locker*/> IDBsqlite3::_mtx_lock_;

/**********************************
 * Public Function Definition.
 ***/
IDBsqlite3::IDBsqlite3( const std::string& db_path ) {
    try {
        clear();
        _m_cb_onselect_ = reinterpret_cast<int (*)(void*,int,char**,char**)>(&IDBsqlite3::callback_onselect);
        _m_db_path_ = db_path;
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
        exit();
    }
}

IDBsqlite3::~IDBsqlite3(void) {
    exit();
}

void IDBsqlite3::start(void) {
    try {
        if( _m_db_path_.empty() == true ) {
            throw std::logic_error("db-path is NULL.");
        }

        open(_m_db_path_);
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
}

int IDBsqlite3::query_insert( std::string context ) {
    // context sample: "{table}({key01},{key02},{key0x}) VALUES('value01',value02,'value0x')"
    return execute_query( "INSERT INTO " + context + ";" );
}

int IDBsqlite3::query_insert_replace( std::string context ) {
    // context sample: "{table}({key01},{key02},{key0x}) VALUES('value01',value02,'value0x')"
    return execute_query( "INSERT OR REPLACE INTO " + context + ";" );
}

int IDBsqlite3::query_update( std::string context ) {
    // context sample: "{table} SET {key01} = 'value', {key02} = 'value' WHERE id = 1"
    return execute_query( "UPDATE " + context + ";" );
}

int IDBsqlite3::query_delete( std::string context ) {
    // context sample: "{table} WHERE id = 1"
    return execute_query( "DELETE FROM " + context + ";" );         // Blocking function.
}

int IDBsqlite3::query_select( std::string context, TCBselect func ) {
    // context sample: "* FROM {table} WHERE id = 1"
    return execute_query( "SELECT " + context + ";", &func );         // Blocking function.
}

std::shared_ptr<IDBsqlite3::TVrecord> IDBsqlite3::query_select( std::string context ) {
    try {
        std::shared_ptr<TVrecord> records = std::make_shared<TVrecord>();
        TCBselect lamda_func = [&records](std::shared_ptr<Trecord>& record) -> void {
            LOGD("push record to record-vector.");
            records->push_back(record);
        };

        if( records.get() == NULL ) {
            throw std::runtime_error("records vector-memory allocation is failed.");
        }

        // context sample: "* FROM {table} WHERE id = 1"
        if( execute_query( "SELECT " + context + ";", &lamda_func ) != SQLITE_OK ) {  // Blocking function.
            throw std::runtime_error("SELECT-query execution is failed.");
        }

        return records;
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
}


/**********************************
 * Protected Function Definition.
 ***/
int IDBsqlite3::execute_query(const std::string&& query, TCBselect* pfunc) {
    int rc = SQLITE_ERROR;
    char *zErrMsg = 0;

    if( _m_inst_ == NULL ) {
        LOGERR("DB-instance is NULL.");
        return rc;
    }

    do {
        rc = sqlite3_exec(_m_inst_, query.c_str(), _m_cb_onselect_, (void*)pfunc, &zErrMsg);    // Block function until done calling call-back.
        if( rc == SQLITE_BUSY ) {
            LOGW("100ms sleep because of SQLITE_BUSY.");
            usleep(100000);     // delay 100 ms
        }
    } while(rc == SQLITE_BUSY);

    if( rc != SQLITE_OK ){
        LOGERR("Executing(%s) is failed.", query.data());
        LOGERR("SQL error(rc=%d): %s", rc, zErrMsg);
        sqlite3_free(zErrMsg);
    }
    return rc;
}

int IDBsqlite3::execute_query(const std::string& query, TCBselect* pfunc) {
    int rc = SQLITE_ERROR;
    char *zErrMsg = 0;

    if( _m_inst_ == NULL ) {
        LOGERR("DB-instance is NULL.");
        return rc;
    }

    do {
        rc = sqlite3_exec(_m_inst_, query.c_str(), _m_cb_onselect_, (void*)pfunc, &zErrMsg);    // Block function until done calling call-back.
        if( rc == SQLITE_BUSY ) {
            LOGW("100ms sleep because of SQLITE_BUSY.");
            usleep(100000);     // delay 100 ms
        }
    } while(rc == SQLITE_BUSY);

    if( rc != SQLITE_OK ){
        LOGERR("Executing(%s) is failed.", query.data());
        LOGERR("SQL error(rc=%d): %s", rc, zErrMsg);
        sqlite3_free(zErrMsg);
    }
    return rc;
}


/**********************************
 * Private Function Definition.
 ***/
void IDBsqlite3::clear(void) {
    _m_inst_ = NULL;
    _m_cb_onselect_ = NULL;
    _m_db_path_.clear();
}

void IDBsqlite3::open(const std::string& db_path) {
    try {
        if( _m_inst_ != NULL ) {
            std::string err = "Already DB-instance(" + db_path + ") is created.";
            throw std::logic_error(err);
        }

        auto itr_locker = _mtx_lock_.find(db_path);
        if( itr_locker == _mtx_lock_.end() ) {
            std::string err = "Can not find " + db_path + " in mapper of DB-open-mutex-locker.";
            throw std::logic_error(err);
        }

        LOGD("Start function.");
        {
            int rc = SQLITE_ERROR;
            bool db_exist_flag = false;
            std::lock_guard<std::mutex> locker(itr_locker->second);

            /* Check whether database file exist. */
            db_exist_flag = check_file_exist(db_path);
            LOGI("Is exist %s file? (result=%d)", db_path.c_str(), db_exist_flag);

            /* Open database */
            rc = sqlite3_open_v2(db_path.c_str(), &_m_inst_, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
            if( rc != SQLITE_OK ) {
                std::string err = "Can't open database(rc=" + std::to_string(rc) + "): " + std::string( sqlite3_errmsg(_m_inst_) );
                throw std::runtime_error(err);
            }

            /* Configure database-settings */
            LOGD("Opened database successfully rc=%d, db-instance=0x%X", rc, _m_inst_);
            if( db_exist_flag == false ) {
                LOGI("Try to change Journal-mode to WAL.");
                if( sqlite3_exec(_m_inst_, "PRAGMA journal_mode = WAL", NULL, NULL, NULL ) != SQLITE_OK ) {
                    throw std::runtime_error("Can not change journal-mode=WAL.");
                }
            }
        }

        /* Create Tables */
        create_table_model();

        int(*callback)(void *,sqlite3*,const char*,int) =  reinterpret_cast<int(*)(void *,sqlite3*,const char*,int)>(&IDBsqlite3::callback_oncommit);
        /* Regist call-back for commit. */
        if( sqlite3_wal_hook(_m_inst_, callback, this ) == NULL ) {
            std::string err = "Can not regist cb_oncommit function pointer. (" + std::string( sqlite3_errmsg(_m_inst_) ) + ")";
            throw std::runtime_error(err);
        }
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
}

void IDBsqlite3::exit(void) {
    if( _m_inst_ != NULL ) {
        sqlite3_close(_m_inst_);
    }
    clear();
}

int IDBsqlite3::callback_oncommit(TdbInst* db, const char* source, int pages) {
    try {
        const std::string src_name = std::string(source);
        return cb_oncommit( src_name, pages );
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
    }
    return SQLITE_ERROR;
};

int IDBsqlite3::callback_onselect(TCBselect* pfunc, int argc, char **argv, char **azColName) {
    std::shared_ptr<Trecord> record;

    try {
        record = std::make_shared<Trecord>();
        if( record.get() == NULL ) {
            throw std::runtime_error("record memory-allocation is failed.");
        }

        if( pfunc != NULL ) {
            for(int i = 0; i<argc; i++) {
                (*record)[ azColName[i] ] = argv[i] ? argv[i] : "NULL";
            }

            (*pfunc)( record );     // call custom-function.
        }
        return SQLITE_OK;
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
    }
    return SQLITE_ERROR;
};


}   // db_pkg
