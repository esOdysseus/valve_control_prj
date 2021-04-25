#ifndef _INTERFACE_DEBUG_KES_ARGUMENTS_H_
#define _INTERFACE_DEBUG_KES_ARGUMENTS_H_

#include <list>
#include <string>
#include <memory>

namespace cmd_pkg {


class IDBGKarg : public std::enable_shared_from_this<IDBGKarg> {
private:
    using SharedThisType = std::enable_shared_from_this<IDBGKarg>;
    using ArgsType = std::list<std::string>;

public:
    IDBGKarg(std::string &cmd, std::shared_ptr<std::list<std::string>> &args, std::string class_type=NoneType);

    virtual ~IDBGKarg(void);

    template <typename _OUT_CLASS_>
    std::shared_ptr<_OUT_CLASS_> get(void);

    std::string get_arg(size_t index);

    virtual bool need_save_file(void) = 0;

protected:
    std::string get_cmd(void) { return _cmd_; }

    std::string get_type(void) { return _class_type_; }

private:
    IDBGKarg(void) = delete;

    void clear(void);

private:
    std::string _cmd_;

    std::shared_ptr<ArgsType> _args_;

    std::string _class_type_;

    static constexpr const char* NoneType="none";

};


}  // namespace cmd_pkg

#endif // _INTERFACE_DEBUG_KES_ARGUMENTS_H_