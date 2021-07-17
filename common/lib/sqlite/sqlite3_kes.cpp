#include <sqlite3_kes.h>

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
        std::cout << "IDBsqlite3::IDBsqlite3(): " << db_path << " loading is failed." << std::endl;
        std::cout << "[Error] " << e.what() << std::endl;
        exit();
    }
}

IDBsqlite3::~IDBsqlite3(void) {
    exit();
}

void IDBsqlite3::start(void) {
    if( _m_db_path_.empty() == true ) {
        throw std::runtime_error("db-path is NULL.");
    }

    open(_m_db_path_);
}

int IDBsqlite3::query_insert( std::string context ) {
    return execute_query( "INSERT INTO " + context );
}

int IDBsqlite3::query_insert_replace( std::string context ) {
    return execute_query( "INSERT OR REPLACE INTO " + context );
}

int IDBsqlite3::query_select( std::string context, TCBselect func ) {
    return execute_query( "SELECT " + context, &func );         // Blocking function.
}

std::shared_ptr<std::vector<IDBsqlite3::Trecord>> IDBsqlite3::query_select( std::string context ) {
    std::shared_ptr<std::vector<Trecord>> records = std::make_shared<std::vector<Trecord>>();
    TCBselect lamda_func = [&records](Trecord& record) -> void {
        std::cout << "push record to record-vector." << std::endl;
        records->push_back(record);
    };

    if( records.get() == NULL ) {
        throw std::runtime_error("records vector-memory allocation is failed.");
    }

    if( execute_query( "SELECT " + context, &lamda_func ) != SQLITE_OK ) {  // Blocking function.
        throw std::runtime_error("SELECT-query execution is failed.");
    }
    return records;
}


/**********************************
 * Protected Function Definition.
 ***/
int IDBsqlite3::execute_query(const std::string&& query, TCBselect* pfunc) {
    int rc = SQLITE_ERROR;
    char *zErrMsg = 0;

    if( _m_inst_ == NULL ) {
        std::cout << "IDBsqlite3::execute_query() DB-instance is NULL." << std::endl;
        return rc;
    }

    rc = sqlite3_exec(_m_inst_, query.c_str(), _m_cb_onselect_, (void*)pfunc, &zErrMsg);    // Block function until done calling call-back.
    if( rc != SQLITE_OK ){
        std::cout << "IDBsqlite3::execute_query(): Executing(" << query << ") is failed." << std::endl;
        std::cout << "[Error] SQL error(rc=" << rc << "): " << zErrMsg;
        sqlite3_free(zErrMsg);
    }
    return rc;
}

int IDBsqlite3::execute_query(const std::string& query, TCBselect* pfunc) {
    int rc = SQLITE_ERROR;
    char *zErrMsg = 0;

    if( _m_inst_ == NULL ) {
        std::cout << "IDBsqlite3::execute_query() DB-instance is NULL." << std::endl;
        return rc;
    }

    rc = sqlite3_exec(_m_inst_, query.c_str(), _m_cb_onselect_, (void*)pfunc, &zErrMsg);    // Block function until done calling call-back.
    if( rc != SQLITE_OK ){
        std::cout << "IDBsqlite3::execute_query(): Executing(" << query << ") is failed." << std::endl;
        std::cout << "[Error] SQL error(rc=" << rc << "): " << zErrMsg;
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
    if( _m_inst_ != NULL ) {
        std::string err = "Already DB-instance(" + db_path + ") is created.";
        throw std::runtime_error(err);
    }

    auto itr_locker = _mtx_lock_.find(db_path);
    if( itr_locker == _mtx_lock_.end() ) {
        std::string err = "Can not find " + db_path + " in mapper of DB-open-mutex-locker.";
        throw std::runtime_error(err);
    }

    std::cout << "Start IDBsqlite3::open() function." << std::endl;
    {
        int rc = SQLITE_ERROR;
        bool db_exist_flag = false;
        std::lock_guard<std::mutex> locker(itr_locker->second);

        /* Check whether database file exist. */
        db_exist_flag = check_file_exist(db_path);
        std::cout << "Is exist " + db_path + " file? (result=" << db_exist_flag << ")" << std::endl;

        /* Open database */
        rc = sqlite3_open_v2(db_path.c_str(), &_m_inst_, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
        if( rc != SQLITE_OK ) {
            std::string err = "Can't open database(rc=" + std::to_string(rc) + "): " + std::string( sqlite3_errmsg(_m_inst_) );
            throw std::runtime_error(err);
        }

        /* Configure database-settings */
        std::cout << "Opened database successfully rc=" << rc << ", db-instance=" << _m_inst_ << std::endl;
        if( db_exist_flag == false ) {
            std::cout << "Try to change Journal-mode to WAL." << std::endl;
            if( sqlite3_exec(_m_inst_, "PRAGMA journal_mode = WAL", NULL, NULL, NULL ) != SQLITE_OK ) {
                throw std::runtime_error("Can not change journal-mode=WAL.");
            }

            create_table_model();
        }
    }

    int(*callback)(void *,sqlite3*,const char*,int) =  reinterpret_cast<int(*)(void *,sqlite3*,const char*,int)>(&IDBsqlite3::callback_oncommit);

    /* Regist call-back for commit. */
    if( sqlite3_wal_hook(_m_inst_, callback, this ) == NULL ) {
        std::string err = "Can not regist cb_oncommit function pointer. (" + std::string( sqlite3_errmsg(_m_inst_) ) + ")";
        throw std::runtime_error(err);
    }
}

void IDBsqlite3::exit(void) {
    if( _m_inst_ != NULL ) {
        sqlite3_close(_m_inst_);
    }
    clear();
}

int IDBsqlite3::callback_oncommit(TdbInst* db, const char* source, int pages) {
    const std::string src_name = std::string(source);
    return cb_oncommit( src_name, pages );
};

int IDBsqlite3::callback_onselect(TCBselect* pfunc, int argc, char **argv, char **azColName) {
    Trecord record;

    if( pfunc != NULL ) {
        for(int i = 0; i<argc; i++) {
            record[ azColName[i] ] = argv[i] ? argv[i] : "NULL";
        }

        (*pfunc)( record );     // call custom-function.
    }
    return SQLITE_OK;
};


}   // db_pkg
