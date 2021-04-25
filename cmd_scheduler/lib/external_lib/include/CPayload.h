#ifndef _CPAYLOAD_H_
#define _CPAYLOAD_H_

#include <string>
#include <memory>


class IProtocolInf;

namespace payload
{
    typedef enum E_ERROR {
        E_NO_ERROR = 0,
        E_INVALID_MEMBER_VARIABLES = 2,
        E_ITS_NOT_SUPPORTED_TYPE = 3,
        E_INVALID_VALUE = 4
    }E_ERROR;

    typedef enum E_PAYLOAD_FLAG {   // We will use it as Bit-Masking type flag.
        E_NONE = 0,
        E_KEEP_PAYLOAD_AFTER_TX = 1
    } E_PAYLOAD_FLAG;

    /******************************
     * Class of Payload.
     */
    class CPayload : public std::enable_shared_from_this<CPayload> {
    public:
        CPayload(std::string name);

        virtual ~CPayload(void);

        /** Get Protocol-Name */
        const std::string get_name(void);

        /** Get Protocol Instance */
        std::shared_ptr<IProtocolInf> get(std::string proto_name);

        /** Get Payload */
        const void* get_payload(size_t& payload_length);

        /** Set Payload */
        bool set_payload(const void* msg, size_t msg_size);

        /** is payload empty? */
        bool is_empty(void);

        /** Get Flag with regard to Payload-Operating. */
        uint32_t get_op_flag(E_PAYLOAD_FLAG target=E_PAYLOAD_FLAG::E_NONE);

        /** Set Flag with regard to Payload-Operating. */
        void set_op_flag(E_PAYLOAD_FLAG target, bool value);

    };

}

#endif // _CPAYLOAD_H_