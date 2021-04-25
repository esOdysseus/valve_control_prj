#ifndef INTERFACE_PROTOCOL_H_
#define INTERFACE_PROTOCOL_H_

#include <list>
#include <map>
#include <memory>

#include <CPayload.h>


class IProtocolInf : public payload::CPayload {
public:
    IProtocolInf(void);

    IProtocolInf(std::string name);

    ~IProtocolInf(void);

    /** Get Keys for properties. */
    virtual std::shared_ptr<std::list<std::string>> get_keys(void);

    /** Get Value for property-key. */
    virtual std::string get_property(const std::string key);

    /** Set Value for property-key. */
    template <typename T>
    bool set_property(const std::string key, T value);

};


#endif // INTERFACE_PROTOCOL_H_