#ifndef _H_INTERFACE_CONTENTS_
#define _H_INTERFACE_CONTENTS_

#include <map>
#include <string>
#include <memory>

#include <json_manipulator.h>
#include <contents/ContentsType.h>
#include <contents/Object.h>

namespace principle {

template< Tprin P, Tdomain D >
class TContents;


class IBaseContents {
public:
    IBaseContents(Tprin prin, Tdomain domain) {   
        clear();
        _m_prin_ = prin;
        _m_domain_ = domain;
    }

    ~IBaseContents( void ) {    clear();    }

    virtual Json_DataType encode( void ) = 0;

    Tprin& principle(void)  { return _m_prin_;  }

    Tdomain& domain(void)  { return _m_domain_;  }

protected:
    std::map<std::string, std::shared_ptr<::base::IType>>& map(void) {
        return _m_member_;
    }

private:
    IBaseContents(void) = delete;

    void clear( void ) {    _m_member_.clear();     }

private:
    std::map<std::string, std::shared_ptr<::base::IType>>  _m_member_;

    Tprin _m_prin_;

    Tdomain _m_domain_;

};


/*****************
 * Where/GPS : TContents Template-Class
 */
template<>
class TContents<Tprin::E_WHERE, Tdomain::E_GPS>: public IBaseContents {
public:
    using Tcontents = typename Sindicator<Tprin::E_WHERE,Tdomain::E_GPS>::Tcontents;

    template<Tcontents C>
    using Ttype = typename Sindicator<Tprin::E_WHERE,Tdomain::E_GPS>::TconType<C>::Ttype;

public:
    TContents(void):IBaseContents(Tprin::E_WHERE, Tdomain::E_GPS) {}

    TContents( Json_DataType &json ):IBaseContents(Tprin::E_WHERE, Tdomain::E_GPS) {  decode(json);   }

    ~TContents(void) = default;

    Json_DataType encode( void ) override;

    template< Tcontents C >
    auto get( void ) -> typename std::add_lvalue_reference< decltype((Ttype<C>())) >::type;

private:
    void decode( Json_DataType &json );

private:
    static const std::string KEY_LONG;
    static const std::string KEY_LAT;

};

/*****************
 * Where/DB : TContents Template-Class
 */
template<>
class TContents<Tprin::E_WHERE, Tdomain::E_DB>: public IBaseContents {
public:
    using Tcontents = typename Sindicator<Tprin::E_WHERE,Tdomain::E_DB>::Tcontents;

    template<Tcontents C>
    using Ttype = typename Sindicator<Tprin::E_WHERE,Tdomain::E_DB>::TconType<C>::Ttype;

public:
    TContents(void):IBaseContents(Tprin::E_WHERE, Tdomain::E_DB) {}

    TContents( Json_DataType &json ):IBaseContents(Tprin::E_WHERE, Tdomain::E_DB) {  decode(json);   };

    ~TContents(void) = default;

    Json_DataType encode( void ) override;

    template< Tcontents C >
    auto get( void ) -> typename std::add_lvalue_reference< decltype((Ttype<C>())) >::type;

private:
    void decode( Json_DataType &json );

private:
    static const std::string KEY_TYPE;
    static const std::string KEY_PATH;
    static const std::string KEY_TABLE;

};


/*****************
 * What/VALVE : TContents Template-Class
 */
template<>
class TContents<Tprin::E_WHAT, Tdomain::E_VALVE>: public IBaseContents {
public:
    using Tcontents = typename Sindicator<Tprin::E_WHAT,Tdomain::E_VALVE>::Tcontents;

    template<Tcontents C>
    using Ttype = typename Sindicator<Tprin::E_WHAT,Tdomain::E_VALVE>::TconType<C>::Ttype;

public:
    TContents(void):IBaseContents(Tprin::E_WHAT, Tdomain::E_VALVE) {}

    TContents( Json_DataType &json ):IBaseContents(Tprin::E_WHAT, Tdomain::E_VALVE) {  decode(json);   };

    ~TContents(void) = default;

    Json_DataType encode( void ) override;

    template< Tcontents C >
    auto get( void ) -> typename std::add_lvalue_reference< decltype((Ttype<C>())) >::type;

private:
    void decode( Json_DataType &json );

private:
    static const std::string KEY_SEQ;

};

/*****************
 * What/DB : TContents Template-Class
 */
template<>
class TContents<Tprin::E_WHAT, Tdomain::E_DB>: public IBaseContents {
public:
    using Tcontents = typename Sindicator<Tprin::E_WHAT,Tdomain::E_DB>::Tcontents;

    template<Tcontents C>
    using Ttype = typename Sindicator<Tprin::E_WHAT,Tdomain::E_DB>::TconType<C>::Ttype;

public:
    TContents(void):IBaseContents(Tprin::E_WHAT, Tdomain::E_DB) {}

    TContents( Json_DataType &json ):IBaseContents(Tprin::E_WHAT, Tdomain::E_DB) {  decode(json);   };

    ~TContents(void) = default;

    Json_DataType encode( void ) override;

    template< Tcontents C >
    auto get( void ) -> typename std::add_lvalue_reference< decltype((Ttype<C>())) >::type;

private:
    void decode( Json_DataType &json );

private:
    static const std::string KEY_TYPE;
    static const std::string KEY_TARGET;

};


/*****************
 * How/VALVE : TContents Template-Class
 */
template<>
class TContents<Tprin::E_HOW, Tdomain::E_VALVE>: public IBaseContents {
public:
    using Tcontents = typename Sindicator<Tprin::E_HOW,Tdomain::E_VALVE>::Tcontents;

    template<Tcontents C>
    using Ttype = typename Sindicator<Tprin::E_HOW,Tdomain::E_VALVE>::TconType<C>::Ttype;

public:
    TContents(void):IBaseContents(Tprin::E_HOW, Tdomain::E_VALVE) {}

    TContents( Json_DataType &json ):IBaseContents(Tprin::E_HOW, Tdomain::E_VALVE) {  decode(json);   };

    ~TContents(void) = default;

    Json_DataType encode( void ) override;

    template< Tcontents C >
    auto get( void ) -> typename std::add_lvalue_reference< decltype((Ttype<C>())) >::type;

private:
    void decode( Json_DataType &json );

private:
    static const std::string KEY_METHOD_PRE;
    static const std::string KEY_COSTTIME;
    static const std::string KEY_METHOD_POST;

};

/*****************
 * How/DB : TContents Template-Class
 */
template<>
class TContents<Tprin::E_HOW, Tdomain::E_DB>: public IBaseContents {
public:
    using Tcontents = typename Sindicator<Tprin::E_HOW,Tdomain::E_DB>::Tcontents;

    template<Tcontents C>
    using Ttype = typename Sindicator<Tprin::E_HOW,Tdomain::E_DB>::TconType<C>::Ttype;

public:
    TContents(void):IBaseContents(Tprin::E_HOW, Tdomain::E_DB) {}

    TContents( Json_DataType &json ):IBaseContents(Tprin::E_HOW, Tdomain::E_DB) {  decode(json);   };

    ~TContents(void) = default;

    Json_DataType encode( void ) override;

    template< Tcontents C >
    auto get( void ) -> typename std::add_lvalue_reference< decltype((Ttype<C>())) >::type;

private:
    void decode( Json_DataType &json );

private:
    static const std::string KEY_METHOD;
    static const std::string KEY_COND;

};



using cWhereGPS = TContents<Tprin::E_WHERE, Tdomain::E_GPS>;
using cWhereDB = TContents<Tprin::E_WHERE, Tdomain::E_DB>;
using cWhatVALVE = TContents<Tprin::E_WHAT, Tdomain::E_VALVE>;
using cWhatDB = TContents<Tprin::E_WHAT, Tdomain::E_DB>;
using cHowVALVE = TContents<Tprin::E_HOW, Tdomain::E_VALVE>;
using cHowDB = TContents<Tprin::E_HOW, Tdomain::E_DB>;



}   // principle


#endif // _H_INTERFACE_CONTENTS_