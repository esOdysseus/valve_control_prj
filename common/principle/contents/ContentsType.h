#ifndef _H_CONTENTS_TYPES_DEFINITION_
#define _H_CONTENTS_TYPES_DEFINITION_

#include <map>
#include <vector>
#include <string>
#include <memory>

namespace principle {


using Tdb_type = enum class enum_db_type: uint32_t {
    E_SQL,
    E_NOSQL
};

using Tdb_data = enum class enum_db_data: uint32_t {
    E_NONE,
    E_RECORDS,
    E_FIELDS
};

using Tdb_method = enum class enum_db_method: uint32_t {
    E_SELECT,
    E_INSERT,
    E_UPDATE,
    E_DELETE
};

using Tvalve_method = enum class enum_valve_method: uint32_t {
    E_NONE,
    E_OPEN,
    E_CLOSE
};

template<typename T>
std::string type_convert(T value);

template<typename T>
T type_convert(std::string value);

using Tprin = enum class enum_principle {
    E_WHERE,
    E_WHAT,
    E_HOW,
    E_WHY
};

using Tdomain = enum class enum_domain {
    E_VALVE,
    E_GPS,
    E_DB
};

template< Tprin P, Tdomain D >
struct Sindicator;

/*************
 * Where/GPS : Type-definition
 */
template<>
struct Sindicator< Tprin::E_WHERE, Tdomain::E_GPS > {
    using Tcontents = enum class enum_contents {
        E_LONG=1,
        E_LAT
    };

    template<Tcontents C>
    struct Scontent;

    template<Tcontents C>
    using TconType = struct Scontent<C>;
};

template<>
struct Sindicator< Tprin::E_WHERE, Tdomain::E_GPS >::Scontent< Sindicator< Tprin::E_WHERE, Tdomain::E_GPS >::Tcontents::E_LONG > {
    using Ttype = double;
};

template<>
struct Sindicator< Tprin::E_WHERE, Tdomain::E_GPS >::Scontent< Sindicator< Tprin::E_WHERE, Tdomain::E_GPS >::Tcontents::E_LAT > {
    using Ttype = double;
};

/*************
 * Where/DB : Type-definition
 */
template<>
struct Sindicator< Tprin::E_WHERE, Tdomain::E_DB > {
    using Tcontents = enum class enum_contents {
        E_TYPE=1,
        E_PATH,
        E_TABLE
    };

    template<Tcontents C>
    struct Scontent;

    template<Tcontents C>
    using TconType = struct Scontent<C>;
};

template<>
struct Sindicator< Tprin::E_WHERE, Tdomain::E_DB >::Scontent< Sindicator< Tprin::E_WHERE, Tdomain::E_DB >::Tcontents::E_TYPE > {
    using Ttype = Tdb_type;
};

template<>
struct Sindicator< Tprin::E_WHERE, Tdomain::E_DB >::Scontent< Sindicator< Tprin::E_WHERE, Tdomain::E_DB >::Tcontents::E_PATH > {
    using Ttype = std::string;
};

template<>
struct Sindicator< Tprin::E_WHERE, Tdomain::E_DB >::Scontent< Sindicator< Tprin::E_WHERE, Tdomain::E_DB >::Tcontents::E_TABLE > {
    using Ttype = std::string;
};


/*************
 * What/VALVE : Type-definition
 */
template<>
struct Sindicator< Tprin::E_WHAT, Tdomain::E_VALVE > {
    using Tcontents = enum class enum_contents {
        E_SEQ=1
    };

    template<Tcontents C>
    struct Scontent;

    template<Tcontents C>
    using TconType = struct Scontent<C>;
};

template<>
struct Sindicator< Tprin::E_WHAT, Tdomain::E_VALVE >::Scontent< Sindicator< Tprin::E_WHAT, Tdomain::E_VALVE >::Tcontents::E_SEQ > {
    using Ttype = uint32_t;
};

/*************
 * What/DB : Type-definition
 */
template<>
struct Sindicator< Tprin::E_WHAT, Tdomain::E_DB > {
    using Tcontents = enum class enum_contents {
        E_TYPE=1,
        E_TARGET
    };

    template<Tcontents C>
    struct Scontent;

    template<Tcontents C>
    using TconType = struct Scontent<C>;
};

template<>
struct Sindicator< Tprin::E_WHAT, Tdomain::E_DB >::Scontent< Sindicator< Tprin::E_WHAT, Tdomain::E_DB >::Tcontents::E_TYPE > {
    using Ttype = Tdb_data;
};

template<>
struct Sindicator< Tprin::E_WHAT, Tdomain::E_DB >::Scontent< Sindicator< Tprin::E_WHAT, Tdomain::E_DB >::Tcontents::E_TARGET > {
    using Ttype = std::map< std::string, std::shared_ptr<std::map<std::string, std::string>> >;
};


/*************
 * How/VALVE : Type-definition
 */
template<>
struct Sindicator< Tprin::E_HOW, Tdomain::E_VALVE > {
    using Tcontents = enum class enum_contents {
        E_METHOD_PRE=1,
        E_COSTTIME,
        E_METHOD_POST
    };

    template<Tcontents C>
    struct Scontent;

    template<Tcontents C>
    using TconType = struct Scontent<C>;
};

template<>
struct Sindicator< Tprin::E_HOW, Tdomain::E_VALVE >::Scontent< Sindicator< Tprin::E_HOW, Tdomain::E_VALVE >::Tcontents::E_METHOD_PRE > {
    using Ttype = Tvalve_method;
};

template<>
struct Sindicator< Tprin::E_HOW, Tdomain::E_VALVE >::Scontent< Sindicator< Tprin::E_HOW, Tdomain::E_VALVE >::Tcontents::E_COSTTIME > {
    using Ttype = double;
};

template<>
struct Sindicator< Tprin::E_HOW, Tdomain::E_VALVE >::Scontent< Sindicator< Tprin::E_HOW, Tdomain::E_VALVE >::Tcontents::E_METHOD_POST > {
    using Ttype = Tvalve_method;
};

/*************
 * How/DB : Type-definition
 */
template<>
struct Sindicator< Tprin::E_HOW, Tdomain::E_DB > {
    using Tcontents = enum class enum_contents {
        E_METHOD=1,
        E_CONDITION
    };

    template<Tcontents C>
    struct Scontent;

    template<Tcontents C>
    using TconType = struct Scontent<C>;
};

template<>
struct Sindicator< Tprin::E_HOW, Tdomain::E_DB >::Scontent< Sindicator< Tprin::E_HOW, Tdomain::E_DB >::Tcontents::E_METHOD > {
    using Ttype = Tdb_method;
};

template<>
struct Sindicator< Tprin::E_HOW, Tdomain::E_DB >::Scontent< Sindicator< Tprin::E_HOW, Tdomain::E_DB >::Tcontents::E_CONDITION > {
    using Ttype = std::map<std::string, std::string>;
};






} // principle


#endif // _H_CONTENTS_TYPES_DEFINITION_