#ifndef _CLASS_DATABASE_HANDLER_H_
#define _CLASS_DATABASE_HANDLER_H_

#include <memory>
#include <string>

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

};


}   // namespace db


#endif // _CLASS_DATABASE_HANDLER_H_