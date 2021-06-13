#include <map>
#include <cassert>

#include <logger.h>
#include <IDBGKarg.h>
#include <CDebugCMD.h>
#include <CException.h>
#include <CDBGKargVctrl.h>

namespace cmd_pkg {

static const std::map<std::string, std::string> map_IDBGKarg = {
    // std::make_pair<std::string, std::string>(CMD_STATE, CDBGKargState::SELF_NAME),
    std::make_pair<std::string, std::string>(CMD_VCONTROL, CDBGKargVctrl::SELF_NAME)
};


template std::shared_ptr<CDBGKargVctrl> IDBGKarg::get<CDBGKargVctrl>(void);

/***********************************
 * Definition of Public Function.
 */
IDBGKarg::IDBGKarg(std::string &cmd, std::shared_ptr<std::list<std::string>> &args, std::string class_type)
: _cmd_(cmd), _class_type_(class_type) { 
    if( CDebugCMD::validation_check(_cmd_, *args.get()) != true ) {
        clear();
        LOGERR("cmd, args validation-checking is failed.");
        throw CException(E_ERROR::E_ERR_FAIL_CHECKING_CMD_VALIDATION);
    }

    // check validation of right-mapping between 'cmd' & 'class_type'.
    auto itor = map_IDBGKarg.find(_cmd_);
    if (itor == map_IDBGKarg.end() || _class_type_ != itor->second ) {
        clear();
        LOGERR("validation-checking about class-type of IDBGKarg is failed.");
        throw CException(E_ERROR::E_ERR_FAIL_CHECKING_CMD_VALIDATION);
    }

    _args_ = args;
}

IDBGKarg::~IDBGKarg(void) {
    clear();
}

template <typename _OUT_CLASS_>
inline std::shared_ptr<_OUT_CLASS_> IDBGKarg::get(void) {
    // using OutType = std::shared_ptr<IDBGKarg>;
    try {
        auto myself = SharedThisType::shared_from_this();
        return std::dynamic_pointer_cast<_OUT_CLASS_>( myself );
    }
    catch( const std::exception &e ) {
        LOGERR("%s", e.what());
        throw e;
    }
}

std::string IDBGKarg::get_arg(size_t index) {
    try {
        if ( index >= _args_->size() || index < 0 ) {
            throw CException(E_ERROR::E_ERR_INVALID_INDEX_NUM);
        }
        return *(std::next(_args_->begin(), index));
    }
    catch( const std::exception &e ) {
        LOGERR("%s", e.what());
        throw dynamic_cast<const CException&>(e);
    }

    return std::string();
}


/***********************************
 * Definition of Private Function.
 */
inline void IDBGKarg::clear(void) {
    _cmd_.clear();
    _args_.reset();
    _class_type_.clear();
}



}


