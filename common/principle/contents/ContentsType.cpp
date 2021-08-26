#include <contents/ContentsType.h>

#include <logger.h>

namespace principle {



/***************************
 * Tdb_type converter
 */
template<>
std::string type_convert<Tdb_type>(Tdb_type value) {
    switch( value ) {
    case Tdb_type::E_SQL:
        return "SQL";
    case Tdb_type::E_NOSQL:
        return "NOSQL";
    default:
        {
            std::string err = "Not Supported value.(" + std::to_string(static_cast<uint32_t>(value)) + ")";
            throw std::invalid_argument(err);
        }
    }
}

template<>
Tdb_type type_convert<Tdb_type>(std::string value) {
    if( value == "SQL" ) {
        return Tdb_type::E_SQL;
    } else if( value == "NOSQL" ) {
        return Tdb_type::E_NOSQL;
    } else {
        std::string err = "Not Supported value.(" + value + ")";
        throw std::invalid_argument(err);
    }
}

/***************************
 * Tdb_data converter
 */
template<>
std::string type_convert<Tdb_data>(Tdb_data value) {
    switch( value ) {
    case Tdb_data::E_NONE:
        return "none";
    case Tdb_data::E_FIELDS:
        return "elements";
    case Tdb_data::E_RECORDS:
        return "records";
    default:
        {
            std::string err = "Not Supported value.(" + std::to_string(static_cast<uint32_t>(value)) + ")";
            throw std::invalid_argument(err);
        }
    }
}

template<>
Tdb_data type_convert<Tdb_data>(std::string value) {
    if( value == "records" ) {
        return Tdb_data::E_RECORDS;
    } else if( value == "elements" ) {
        return Tdb_data::E_FIELDS;
    } else if( value == "none" ) {
        return Tdb_data::E_NONE;
    } else {
        std::string err = "Not Supported value.(" + value + ")";
        throw std::invalid_argument(err);
    }
}

/***************************
 * Tdb_method converter
 */
template<>
std::string type_convert<Tdb_method>(Tdb_method value) {
    switch( value ) {
    case Tdb_method::E_SELECT:
        return "select";
    case Tdb_method::E_INSERT:
        return "insert";
    case Tdb_method::E_UPDATE:
        return "update";
    case Tdb_method::E_DELETE:
        return "delete";
    default:
        {
            std::string err = "Not Supported value.(" + std::to_string(static_cast<uint32_t>(value)) + ")";
            throw std::invalid_argument(err);
        }
    }
}

template<>
Tdb_method type_convert<Tdb_method>(std::string value) {
    if( value == "select" ) {
        return Tdb_method::E_SELECT;
    } else if( value == "insert" ) {
        return Tdb_method::E_INSERT;
    } else if( value == "update" ) {
        return Tdb_method::E_UPDATE;
    } else if( value == "delete" ) {
        return Tdb_method::E_DELETE;
    } else {
        std::string err = "Not Supported value.(" + value + ")";
        throw std::invalid_argument(err);
    }
}

/***************************
 * Tvalve_method converter
 */
template<>
std::string type_convert<Tvalve_method>(Tvalve_method value) {
    switch( value ) {
    case Tvalve_method::E_NONE:
        return "none";
    case Tvalve_method::E_OPEN:
        return "open";
    case Tvalve_method::E_CLOSE:
        return "close";
    default:
        {
            std::string err = "Not Supported value.(" + std::to_string(static_cast<uint32_t>(value)) + ")";
            throw std::invalid_argument(err);
        }
    }
}

template<>
Tvalve_method type_convert<Tvalve_method>(std::string value) {
    if( value == "open" ) {
        return Tvalve_method::E_OPEN;
    } else if( value == "close" ) {
        return Tvalve_method::E_CLOSE;
    } else if( value == "none" ) {
        return Tvalve_method::E_NONE;
    } else {
        std::string err = "Not Supported value.(" + value + ")";
        throw std::invalid_argument(err);
    }
}



}   // principle