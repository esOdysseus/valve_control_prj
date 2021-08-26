#include <iostream>

#include <contents/Contents.h>
#include <logger.h>

namespace principle {

using CMjson = json_mng::CMjson;

/// Where
const std::string cWhereGPS::KEY_LONG = "long";
const std::string cWhereGPS::KEY_LAT = "lat";
const std::string cWhereDB::KEY_TYPE = "type";
const std::string cWhereDB::KEY_PATH = "path";
const std::string cWhereDB::KEY_TABLE = "table";
/// What
const std::string cWhatVALVE::KEY_SEQ = "seq";
const std::string cWhatDB::KEY_TYPE = "type";
const std::string cWhatDB::KEY_TARGET = "target";
/// How
const std::string cHowVALVE::KEY_METHOD_PRE = "method-pre";
const std::string cHowVALVE::KEY_COSTTIME = "costtime";
const std::string cHowVALVE::KEY_METHOD_POST = "method-post";
const std::string cHowDB::KEY_METHOD = "method";
const std::string cHowDB::KEY_COND = "condition";


/*****************
 * Where/GPS : Function Definition of TContents Class
 */
template< cWhereGPS::Tcontents C >
using DTwhereGPS = typename Sindicator<Tprin::E_WHERE, Tdomain::E_GPS>::TconType<C>::Ttype;

template<>
auto cWhereGPS::get<cWhereGPS::Tcontents::E_LONG>( void ) 
    -> std::add_lvalue_reference< decltype((DTwhereGPS<cWhereGPS::Tcontents::E_LONG>())) >::type {
    if( map()[KEY_LONG].get() == NULL ) {
        map()[KEY_LONG] = std::make_shared<::base::Object< DTwhereGPS<Tcontents::E_LONG> >>();
    }
    return ::base::Object<DTwhereGPS<Tcontents::E_LONG>>::get( map()[KEY_LONG] );
}

template<>
auto cWhereGPS::get<cWhereGPS::Tcontents::E_LAT>( void ) 
    -> std::add_lvalue_reference< decltype((DTwhereGPS<cWhereGPS::Tcontents::E_LAT>())) >::type {
    if( map()[KEY_LAT].get() == NULL ) {
        map()[KEY_LAT] = std::make_shared<::base::Object< DTwhereGPS<Tcontents::E_LAT> >>();
    }
    return ::base::Object<DTwhereGPS<Tcontents::E_LAT>>::get( map()[KEY_LAT] );
}

Json_DataType cWhereGPS::encode( void ) {
    Json_DataType  json;
    LOGI("Enter");
    try {
        json = std::make_shared<CMjson>();
        if( json.get() == NULL ) {
            throw std::logic_error("JSON-Creating is failed.");
        }

        json->set_member(KEY_LONG, get<Tcontents::E_LONG>());
        json->set_member(KEY_LAT, get<Tcontents::E_LAT>());
    }
    catch ( const std::exception& e ) {
        LOGERR("%s", e.what());
        json.reset();
        throw e;
    }
    return json;
}

void cWhereGPS::decode( Json_DataType &json ) {
    LOGI("Enter");
    try {
        get<Tcontents::E_LONG>() = json->get_member<double>(KEY_LONG);
        get<Tcontents::E_LAT>() = json->get_member<double>(KEY_LAT);
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
}

/*****************
 * Where/DB : Function Definition of TContents Class
 */
template< cWhereDB::Tcontents C >
using DTwhereDB = typename Sindicator<Tprin::E_WHERE, Tdomain::E_DB>::TconType<C>::Ttype;

template<>
auto cWhereDB::get<cWhereDB::Tcontents::E_TYPE>( void ) 
    -> std::add_lvalue_reference< decltype((DTwhereDB<cWhereDB::Tcontents::E_TYPE>())) >::type {
    if( map()[KEY_TYPE].get() == NULL ) {
        map()[KEY_TYPE] = std::make_shared<::base::Object< DTwhereDB<Tcontents::E_TYPE> >>();
    }
    return ::base::Object<DTwhereDB<Tcontents::E_TYPE>>::get( map()[KEY_TYPE] );
}

template<>
auto cWhereDB::get<cWhereDB::Tcontents::E_PATH>( void ) 
    -> std::add_lvalue_reference< decltype((DTwhereDB<cWhereDB::Tcontents::E_PATH>())) >::type {
    if( map()[KEY_PATH].get() == NULL ) {
        map()[KEY_PATH] = std::make_shared<::base::Object< DTwhereDB<Tcontents::E_PATH> >>();
    }
    return ::base::Object<DTwhereDB<Tcontents::E_PATH>>::get( map()[KEY_PATH] );
}

template<>
auto cWhereDB::get<cWhereDB::Tcontents::E_TABLE>( void ) 
    -> std::add_lvalue_reference< decltype((DTwhereDB<cWhereDB::Tcontents::E_TABLE>())) >::type {
    if( map()[KEY_TABLE].get() == NULL ) {
        map()[KEY_TABLE] = std::make_shared<::base::Object< DTwhereDB<Tcontents::E_TABLE> >>();
    }
    return ::base::Object<DTwhereDB<Tcontents::E_TABLE>>::get( map()[KEY_TABLE] );
}

Json_DataType cWhereDB::encode( void ) {
    Json_DataType  json;
    LOGD("Enter");
    try {
        json = std::make_shared<CMjson>();
        if( json.get() == NULL ) {
            throw std::logic_error("JSON-Creating is failed.");
        }

        switch( get<Tcontents::E_TYPE>() ) {
        case Tdb_type::E_SQL:
            json->set_member(KEY_TYPE, "SQL");
            break;
        case Tdb_type::E_NOSQL:
            json->set_member(KEY_TYPE, "NOSQL");
            break;
        default:
            throw std::logic_error("Invalid Tdb_type, Please check it.");
        }
        
        json->set_member(KEY_PATH, get<Tcontents::E_PATH>());
        json->set_member(KEY_TABLE, get<Tcontents::E_TABLE>());
    }
    catch ( const std::exception& e ) {
        LOGERR("%s", e.what());
        json.reset();
        throw e;
    }
    return json;
}

void cWhereDB::decode( Json_DataType &json ) {
    LOGD("Enter");
    try {
        std::string type = json->get_member(KEY_TYPE);

        if( type == "SQL" ) { 
            get<Tcontents::E_TYPE>() = Tdb_type::E_SQL;
        } else if ( type == "NOSQL" ) {
            get<Tcontents::E_TYPE>() = Tdb_type::E_NOSQL;
        } else {
            std::string err = "Parsed member(" + type + ") is not supported Tdb_type.";
            throw std::invalid_argument(err);
        }
        get<Tcontents::E_PATH>() = json->get_member(KEY_PATH);
        get<Tcontents::E_TABLE>() = json->get_member(KEY_TABLE);
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
}


/*****************
 * What/VALVE : Function Definition of TContents Class
 */
template< cWhatVALVE::Tcontents C >
using DTwhatVALVE = typename Sindicator<Tprin::E_WHAT, Tdomain::E_VALVE>::TconType<C>::Ttype;

template<>
auto cWhatVALVE::get<cWhatVALVE::Tcontents::E_SEQ>( void ) 
    -> std::add_lvalue_reference< decltype((DTwhatVALVE<cWhatVALVE::Tcontents::E_SEQ>())) >::type {
    if( map()[KEY_SEQ].get() == NULL ) {
        map()[KEY_SEQ] = std::make_shared<::base::Object< DTwhatVALVE<Tcontents::E_SEQ> >>();
    }
    return ::base::Object<DTwhatVALVE<Tcontents::E_SEQ>>::get( map()[KEY_SEQ] );
}

Json_DataType cWhatVALVE::encode( void ) {
    Json_DataType  json;
    LOGI("Enter");
    try {
        json = std::make_shared<CMjson>();
        if( json.get() == NULL ) {
            throw std::logic_error("JSON-Creating is failed.");
        }

        json->set_member(KEY_SEQ, get<Tcontents::E_SEQ>());
    }
    catch ( const std::exception& e ) {
        LOGERR("%s", e.what());
        json.reset();
        throw e;
    }
    return json;
}

void cWhatVALVE::decode( Json_DataType &json ) {
    LOGI("Enter");
    try {
        get<Tcontents::E_SEQ>() = json->get_member<uint32_t>(KEY_SEQ);
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
}

/*****************
 * What/DB : Function Definition of TContents Class
 */
template< cWhatDB::Tcontents C >
using DTwhatDB = typename Sindicator<Tprin::E_WHAT, Tdomain::E_DB>::TconType<C>::Ttype;

template<>
auto cWhatDB::get<cWhatDB::Tcontents::E_TYPE>( void ) 
    -> std::add_lvalue_reference< decltype((DTwhatDB<cWhatDB::Tcontents::E_TYPE>())) >::type {
    if( map()[KEY_TYPE].get() == NULL ) {
        map()[KEY_TYPE] = std::make_shared<::base::Object< DTwhatDB<Tcontents::E_TYPE> >>();
    }
    return ::base::Object<DTwhatDB<Tcontents::E_TYPE>>::get( map()[KEY_TYPE] );
}

template<>
auto cWhatDB::get<cWhatDB::Tcontents::E_TARGET>( void ) 
    -> std::add_lvalue_reference< decltype((DTwhatDB<cWhatDB::Tcontents::E_TARGET>())) >::type {
    if( map()[KEY_TARGET].get() == NULL ) {
        map()[KEY_TARGET] = std::make_shared<::base::Object< DTwhatDB<Tcontents::E_TARGET> >>();
    }
    return ::base::Object<DTwhatDB<Tcontents::E_TARGET>>::get( map()[KEY_TARGET] );
}

Json_DataType cWhatDB::encode( void ) {
    Json_DataType  json;
    LOGI("Enter");
    try {
        Json_DataType  json_sub;
        json = std::make_shared<CMjson>();
        if( json.get() == NULL ) {
            throw std::logic_error("JSON-Creating is failed.");
        }

        auto& map_records = get<Tcontents::E_TARGET>();
        switch( get<Tcontents::E_TYPE>() ) {
        case Tdb_data::E_NONE:
            json->set_member(KEY_TYPE, "none");
            break;
        case Tdb_data::E_FIELDS:
            json->set_member(KEY_TYPE, "elements");
            break;
        case Tdb_data::E_RECORDS:
            json->set_member(KEY_TYPE, "records");
            break;
        default:
            throw std::logic_error("Invalid Tdb_data, Please check it.");
        }
        
        // Make Json-msg for KEY_TARGET
        json_sub = json->set_member(KEY_TARGET);
        for(auto itr=map_records.begin(); itr!=map_records.end(); itr++) {
            Json_DataType json_sub2 = json_sub->set_member(itr->first);
            for(auto itr_field=itr->second->begin(); itr_field!=itr->second->end(); itr_field++) {
                json_sub2->set_member(itr_field->first, itr_field->second);
            }
            json_sub->set_member(itr->first, json_sub2.get());
        }
        json->set_member(KEY_TARGET, json_sub.get());
    }
    catch ( const std::exception& e ) {
        LOGERR("%s", e.what());
        json.reset();
        throw e;
    }
    return json;
}

void cWhatDB::decode( Json_DataType &json ) {
    LOGI("Enter");
    try {
        std::shared_ptr< std::map<std::string, std::string> > m_record;
        auto& map_records = get<Tcontents::E_TARGET>();
        std::string type = json->get_member(KEY_TYPE);

        if( type == "records" ) { 
            get<Tcontents::E_TYPE>() = Tdb_data::E_RECORDS;
        } else if ( type == "elements" ) {
            get<Tcontents::E_TYPE>() = Tdb_data::E_FIELDS;
        } else if ( type == "none" ) {
            get<Tcontents::E_TYPE>() = Tdb_data::E_NONE;
        } else {
            std::string err = "Parsed member(" + type + ") is not supported Tdb_data.";
            throw std::invalid_argument(err);
        }

        // Make records for KEY_TARGET
        auto records = json->get_member<Json_DataType>(KEY_TARGET);
        for(auto itr=records->begin(); itr!=records->end(); itr++) {
            std::string first = CMjson::get_first(itr);
            std::shared_ptr<CMjson> record = CMjson::get_second<std::shared_ptr<CMjson>>(itr);

            m_record = std::make_shared<std::map<std::string, std::string>>();
            for(auto itr_field=record->begin(); itr_field!=record->end(); itr_field++) {
                std::string key = CMjson::get_first(itr_field);
                std::string value = CMjson::get_second(itr_field);

                (*m_record)[key] = value;
            }
            map_records[first] = m_record;
        }
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
}


/*****************
 * How/VALVE : Function Definition of TContents Class
 */
template< cHowVALVE::Tcontents C >
using DThowVALVE = typename Sindicator<Tprin::E_HOW, Tdomain::E_VALVE>::TconType<C>::Ttype;

template<>
auto cHowVALVE::get<cHowVALVE::Tcontents::E_METHOD_PRE>( void ) 
    -> std::add_lvalue_reference< decltype((DThowVALVE<cHowVALVE::Tcontents::E_METHOD_PRE>())) >::type {
    if( map()[KEY_METHOD_PRE].get() == NULL ) {
        map()[KEY_METHOD_PRE] = std::make_shared<::base::Object< DThowVALVE<Tcontents::E_METHOD_PRE> >>();
    }
    return ::base::Object<DThowVALVE<Tcontents::E_METHOD_PRE>>::get( map()[KEY_METHOD_PRE] );
}

template<>
auto cHowVALVE::get<cHowVALVE::Tcontents::E_COSTTIME>( void ) 
    -> std::add_lvalue_reference< decltype((DThowVALVE<cHowVALVE::Tcontents::E_COSTTIME>())) >::type {
    if( map()[KEY_COSTTIME].get() == NULL ) {
        map()[KEY_COSTTIME] = std::make_shared<::base::Object< DThowVALVE<Tcontents::E_COSTTIME> >>();
    }
    return ::base::Object<DThowVALVE<Tcontents::E_COSTTIME>>::get( map()[KEY_COSTTIME] );
}

template<>
auto cHowVALVE::get<cHowVALVE::Tcontents::E_METHOD_POST>( void ) 
    -> std::add_lvalue_reference< decltype((DThowVALVE<cHowVALVE::Tcontents::E_METHOD_POST>())) >::type {
    if( map()[KEY_METHOD_POST].get() == NULL ) {
        map()[KEY_METHOD_POST] = std::make_shared<::base::Object< DThowVALVE<Tcontents::E_METHOD_POST> >>();
    }
    return ::base::Object<DThowVALVE<Tcontents::E_METHOD_POST>>::get( map()[KEY_METHOD_POST] );
}

Json_DataType cHowVALVE::encode( void ) {
    Json_DataType  json;
    LOGI("Enter");
    try {
        json = std::make_shared<CMjson>();
        if( json.get() == NULL ) {
            throw std::logic_error("JSON-Creating is failed.");
        }

        json->set_member(KEY_METHOD_PRE, type_convert( get<Tcontents::E_METHOD_PRE>()) );
        json->set_member(KEY_COSTTIME, get<Tcontents::E_COSTTIME>());
        json->set_member(KEY_METHOD_POST, type_convert( get<Tcontents::E_METHOD_POST>()) );
    }
    catch ( const std::exception& e ) {
        LOGERR("%s", e.what());
        json.reset();
        throw e;
    }
    return json;
}

void cHowVALVE::decode( Json_DataType &json ) {
    LOGI("Enter");
    try {
        get<Tcontents::E_METHOD_PRE>() = type_convert<Tvalve_method>( json->get_member(KEY_METHOD_PRE) );
        get<Tcontents::E_COSTTIME>() = json->get_member<double>(KEY_COSTTIME);
        get<Tcontents::E_METHOD_POST>() = type_convert<Tvalve_method>( json->get_member(KEY_METHOD_POST) );
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
}


/*****************
 * How/DB : Function Definition of TContents Class
 */
template< cHowDB::Tcontents C >
using DThowDB = typename Sindicator<Tprin::E_HOW, Tdomain::E_DB>::TconType<C>::Ttype;

template<>
auto cHowDB::get<cHowDB::Tcontents::E_METHOD>( void ) 
    -> std::add_lvalue_reference< decltype((DThowDB<cHowDB::Tcontents::E_METHOD>())) >::type {
    if( map()[KEY_METHOD].get() == NULL ) {
        map()[KEY_METHOD] = std::make_shared<::base::Object< DThowDB<Tcontents::E_METHOD> >>();
    }
    return ::base::Object<DThowDB<Tcontents::E_METHOD>>::get( map()[KEY_METHOD] );
}

template<>
auto cHowDB::get<cHowDB::Tcontents::E_CONDITION>( void ) 
    -> std::add_lvalue_reference< decltype((DThowDB<cHowDB::Tcontents::E_CONDITION>())) >::type {
    if( map()[KEY_COND].get() == NULL ) {
        map()[KEY_COND] = std::make_shared<::base::Object< DThowDB<Tcontents::E_CONDITION> >>();
    }
    return ::base::Object<DThowDB<Tcontents::E_CONDITION>>::get( map()[KEY_COND] );
}

Json_DataType cHowDB::encode( void ) {
    Json_DataType  json;
    LOGI("Enter");
    try {
        Json_DataType  json_sub;
        auto& m_conds = get<Tcontents::E_CONDITION>();
        json = std::make_shared<CMjson>();
        if( json.get() == NULL ) {
            throw std::logic_error("JSON-Creating is failed.");
        }

        switch(get<Tcontents::E_METHOD>()) {
        case Tdb_method::E_SELECT:
            json->set_member(KEY_METHOD, "select");
            break;
        case Tdb_method::E_INSERT:
            json->set_member(KEY_METHOD, "insert");
            break;
        case Tdb_method::E_UPDATE:
            json->set_member(KEY_METHOD, "update");
            break;
        case Tdb_method::E_DELETE:
            json->set_member(KEY_METHOD, "delete");
            break;
        default:
            throw std::logic_error("Invalid Tdb_method, Please check it.");
        }
        
        // Make Json-msg for KEY_COND
        json_sub = json->set_member(KEY_COND);
        for(auto itr=m_conds.begin(); itr!=m_conds.end(); itr++) {
            json_sub->set_member(itr->first, itr->second);
        }
        json->set_member(KEY_COND, json_sub.get());
    }
    catch ( const std::exception& e ) {
        LOGERR("%s", e.what());
        json.reset();
        throw e;
    }
    return json;
}

void cHowDB::decode( Json_DataType &json ) {
    LOGI("Enter");
    try {
        std::string method = json->get_member(KEY_METHOD);
        auto& m_conds = get<Tcontents::E_CONDITION>();

        if( method == "select" ) {
            get<Tcontents::E_METHOD>() = Tdb_method::E_SELECT;
        } else if( method == "insert" ) {
            get<Tcontents::E_METHOD>() = Tdb_method::E_INSERT;
        } else if( method == "update" ) {
            get<Tcontents::E_METHOD>() = Tdb_method::E_UPDATE;
        } else if( method == "delete" ) {
            get<Tcontents::E_METHOD>() = Tdb_method::E_DELETE;
        } else {
            std::string err = "Parsed member(" + method + ") is not supported Tdb_method.";
            throw std::invalid_argument(err);
        }
        
        // Make condition-mapper for KEY_COND
        auto conds = json->get_member<Json_DataType>(KEY_COND);
        for(auto itr=conds->begin(); itr!=conds->end(); itr++) {
            std::string index = CMjson::get_first(itr);
            std::string condition = CMjson::get_second(itr);

            m_conds[index] = condition;
        }
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
}



}   // principle
