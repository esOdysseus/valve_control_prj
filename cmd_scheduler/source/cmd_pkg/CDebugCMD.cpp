#include <cassert>
#include <sstream>
#include <memory>
#include <memory.h>

#include <logger.h>
#include <CDebugCMD.h>
#include <CException.h>
#include <CDBGKargVctrl.h>
// #include <CDBGKargState.h>

namespace cmd_pkg {

constexpr const char* CDebugCMD::_valid_cmds_[];

/***************************************
 * Definition of Public Function.
 */
CDebugCMD::CDebugCMD(std::string &from_who, const void *data, ssize_t data_size)
{
    /** 
     * reload_cmd_period : period(unit: second) of CMD-Reloading from saved-file to execute next-CMD.
     */
    LOGD("Called.");
    std::shared_ptr<ArgsType> args = std::make_shared<ArgsType>();
    clear();

    try {
        // Decode command.
        _from_who_ = from_who;
        _full_cmd_ = std::string((const char*)data);
        if( decode(data,data_size, args) != true ) {
            LOGERR("Decoding is failed.");
        }

        // Create Smart-Args class.
        if ( _cmd_ == CMD_VCONTROL ) {
            _smart_args_ = std::make_shared<CDBGKargVctrl>(_cmd_, args);
        }
        // else if ( _cmd_ == CMD_STATE ) {
        //     _smart_args_ = std::make_shared<CDBGKargState>(_cmd_, args, reload_cmd_period);
        // }
        else {
            LOGERR("Not supported DBGK-cmd(%s)", _cmd_.c_str());
            throw CException(E_ERROR::E_ERR_NOT_SUPPORTED_CMD);
        }
    }
    catch ( const std::exception &e ) {
        LOGERR("%s", e.what());
        // throw dynamic_cast<const CException&>(e);
        throw;
    }
}

CDebugCMD::~CDebugCMD(void) {
    clear();
}

std::string CDebugCMD::get_arg(size_t index) {
    if (is_there()) {
        return _smart_args_->get_arg(index);
    }
    return std::string();
}

inline void CDebugCMD::clear(void) {
    _is_parsed_ = false;
    _from_who_.clear();
    _full_cmd_.clear();
    _cmd_.clear();
    _smart_args_.reset();
}

bool CDebugCMD::decode(const void *raw_data, ssize_t data_size, std::shared_ptr<ArgsType> &args) {
    LOGD("Called");
    const char* data = NULL;
    assert( raw_data != NULL );
    assert( data_size > 0 );

    if( _is_parsed_ == false ) {
        assert( check_sof(raw_data, data_size) == true );
        data = (const char*)raw_data + SOF_SIZE;
        assert( (data_size -= SOF_SIZE) > 0 );

        LOGD("Debug CMD = %s", data);
        if( split_cmd_args(data, args) != true ) {
            return false;
        }

        _is_parsed_ = true;
    }

    return _is_parsed_;
}

bool CDebugCMD::check_sof(const void *data, ssize_t data_size) {
    char SOF[SOF_SIZE+1] = {0,};

    bzero(SOF, SOF_SIZE+1);
    memcpy(SOF, data, SOF_SIZE);
    return !strcmp(SOF, CMD_DEBUG);
}

bool CDebugCMD::split_cmd_args(const char *data, std::shared_ptr<ArgsType> &args) {
    std::stringstream ss;
    std::string item;
    bool is_cmd = true;

    try {
        ss.str(data);
        while (std::getline(ss, item, ' ')) {
            if (is_cmd == true ) {
                if ( item.empty() == false ) {
                    _cmd_ = item;
                    is_cmd = false;
                    LOGD("CMD = %s", _cmd_.c_str());
                }
            }
            else {
                args->push_back(item);
                LOGD("ARG = %s", item.c_str());
            }
        }

        // validation check.
        return validation_check(_cmd_, *args.get());
    }
    catch (const std::exception &e) {
        LOGERR("%s", e.what());
    }

    return false;
}

bool CDebugCMD::validation_check(std::string &cmd, std::list<std::string> &args) {
    int cnt = 0;

    try {
        // check Empty Command.
        if (cmd.empty() == true)  {
            LOGW("There is not Command in packet. Please check it.");
            throw CException(E_ERROR::E_ERR_EMPTY_CMD);
        }

        // check validation of Command.
        while( _valid_cmds_[cnt] != NULL ) {
            if ( cmd == _valid_cmds_[cnt] ) {
                break;
            }
            cnt++;
        }

        if (_valid_cmds_[cnt] == NULL) {
            LOGW("Invalid Command. (%s)", cmd.c_str());
            throw CException(E_ERROR::E_ERR_INVALID_CMD);
        }

        // check validation of Argument-count.
        if( cmd == CMD_VCONTROL ) {
            if ( args.size() < CMD_VCONTROL_ARGCNT ) {
                LOGW("Invalid Argument-count. (%s: %d)", CMD_VCONTROL, args.size());
                throw CException(E_ERROR::E_ERR_INVALID_ARG_COUNT);
            }
        }
        return true;
    }
    catch ( const std::exception &e ) {
        LOGERR("%s", e.what());
    }

    return false;
}


}   // namespace cmd_pkg