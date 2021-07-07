#include <CDBhandler.h>

#include <logger.h>

namespace db {


/*********************************
 * Definition of Public Function.
 */
CDBhandler::CDBhandler( void ) {
    ;   // TODO
}

CDBhandler::~CDBhandler( void ) {
    ;   // TODO
}

bool CDBhandler::init( void ) {
    try {
        ;   // TODO
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
}

/*********************************
 * Definition of Private Function.
 */
void CDBhandler::clear( void ) {
    ;   // TODO
}



}   // db