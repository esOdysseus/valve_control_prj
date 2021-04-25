#ifndef _C_DEBUG_COMMAND_H_
#define _C_DEBUG_COMMAND_H_

#include <list>
#include <string>
#include <memory>
#include <stdio.h>

#include <CCmd.h>
#include <IDBGKarg.h>

namespace cmd_pkg {

class CDebugCMD {
public:
    using ArgsType = std::list<std::string>;

public:
    CDebugCMD(std::string &from_who, const void *data, ssize_t data_size);

    ~CDebugCMD(void);

    bool is_there(void) { return _is_parsed_; }

    std::string get_resp_dest(void) { return _from_who_;  }

    std::string get_full_cmd(void) { return _full_cmd_;  }

    std::string get_cmd(void) { return _cmd_;   }

    std::string get_arg(size_t index);

    template <typename _OUT_CLASS_>
    std::shared_ptr<_OUT_CLASS_> get_arg(void) {
        return _smart_args_->get<_OUT_CLASS_>();
    }

private:
    friend class IDBGKarg;

    CDebugCMD(void) = delete;

    void clear(void);

    bool decode(const void *data, ssize_t data_size, std::shared_ptr<ArgsType> &args);

    bool check_sof(const void *data, ssize_t data_size);

    bool split_cmd_args(const char *data, std::shared_ptr<ArgsType> &args);

    static bool validation_check(std::string &cmd, std::list<std::string> &args);

private:
    bool _is_parsed_;

    std::string _from_who_;

    std::string _full_cmd_;

    std::string _cmd_;

    std::shared_ptr<IDBGKarg> _smart_args_;

    static constexpr const char* _valid_cmds_[] = { CMD_STATE, /*CMD_SERVICES,*/ CMD_VCONTROL, NULL }; 

};


} // cmd_pkg

#endif // _C_DEBUG_COMMAND_H_