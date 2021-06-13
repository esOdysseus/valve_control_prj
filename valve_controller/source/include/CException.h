#ifndef _C_EXCEPTION_H_
#define _C_EXCEPTION_H_

#include <stdexcept>
#include <logger.h>

typedef enum E_ERROR {
    E_NO_ERROR                      = 0,
    
    E_ERR_FAIL_INSERT_DATA          = 1,
    E_ERR_FAIL_UPDATE_DATA,
    E_ERR_FAIL_TRIG_UPDATE_DATA,
    E_ERR_FAIL_RUNNING_CMD          = 4,
    E_ERR_FAIL_INSERT_CMD,
    E_ERR_FAIL_EXECUTE_CMD,
    E_ERR_FAIL_TRIG_EXECUTE_CMD,
    E_ERR_FAIL_ENCODING_CMD,
    E_ERR_FAIL_DECODING_CMD,
    E_ERR_FAIL_CHECKING_CMD_PROC,
    E_ERR_FAIL_MAKING_PACKET        = 11,
    E_ERR_FAIL_SENDING_PACKET,
    E_ERR_FAIL_SENDING_ACT_DONE,
    E_ERR_FAIL_CREATING_THREAD      = 14,
    E_ERR_FAIL_INVOKING_SHELL,
    E_ERR_FAIL_MEM_ALLOC,
    E_ERR_FAIL_CONVERTING_TIME,
    E_ERR_FAIL_EXECUTE_FUNC,
    E_ERR_FAIL_INITE_GPIO_ROOT,
    E_ERR_FAIL_INSPECTING_DATA,
    E_ERR_FAIL_TIME_UPDATE,

    E_ERR_INVALID_CMD               = 22,
    E_ERR_INVALID_VALUE,
    E_ERR_INVALID_NULL_VALUE,
    E_ERR_INVALID_ARG_COUNT,
    E_ERR_INVALID_INDEX_NUM,

    E_ERR_NOT_SUPPORTED_CMD         = 27,
    E_ERR_NOT_SUPPORTED_WHAT,
    E_ERR_NOT_SUPPORTED_HOW,
    E_ERR_NOT_SUPPORTED_TYPE,
    E_ERR_NOT_EXISTED_PROC,

    E_ERR_NOT_HAVE_MEMBER           = 32,
    E_ERR_NOT_ARRAY,

    E_ERR_NEED_TIME_SYNC            = 34,

    E_ERR_EMPTY_CMD                 = 35,
    E_ERR_OUT_OF_SERVICE_VALVE,
} E_ERROR;


class CException: public std::exception
{
public:
    /** Constructor (C strings).
     *  @param message C-style string error message.
     *                 The string contents are copied upon construction.
     *                 Hence, responsibility for deleting the char* lies
     *                 with the caller. 
     */
    explicit CException(E_ERROR err_num):
      __err_n(err_num)
      {
      }

    /** Destructor.
     * Virtual to allow for subclassing.
     */
    virtual ~CException(void) throw (){}

    /** Returns a pointer to the (constant) error description.
     *  @return A pointer to a const char*. The underlying memory
     *          is in posession of the Exception object. Callers must
     *          not attempt to free the memory.
     */
    virtual const char* what(void) const throw(){
        return exception_switch(__err_n);
    }

    E_ERROR get(void) {
        return __err_n;
    }

private:
    const char* exception_switch(E_ERROR __err_n) const throw() {
        switch( __err_n ) {
        case E_ERR_FAIL_INSERT_DATA:
            return "E_ERR_FAIL_INSERT_DATA occured.";
        case E_ERR_FAIL_UPDATE_DATA:
            return "E_ERR_FAIL_UPDATE_DATA occured.";
        case E_ERR_FAIL_TRIG_UPDATE_DATA:
            return "E_ERR_FAIL_TRIG_UPDATE_DATA occured.";
        case E_ERR_FAIL_RUNNING_CMD:
            return "E_ERR_FAIL_RUNNING_CMD occured.";
        case E_ERR_FAIL_INSERT_CMD:
            return "E_ERR_FAIL_INSERT_CMD occured.";
        case E_ERR_FAIL_EXECUTE_CMD:
            return "E_ERR_FAIL_EXECUTE_CMD occured.";
        case E_ERR_FAIL_TRIG_EXECUTE_CMD:
            return "E_ERR_FAIL_TRIG_EXECUTE_CMD occured.";
        case E_ERR_FAIL_ENCODING_CMD:
            return "E_ERR_FAIL_ENCODING_CMD occured.";
        case E_ERR_FAIL_DECODING_CMD:
            return "E_ERR_FAIL_DECODING_CMD occured.";
        case E_ERR_FAIL_CHECKING_CMD_PROC:
            return "E_ERR_FAIL_CHECKING_CMD_PROC occured.";
        case E_ERR_FAIL_MAKING_PACKET:
            return "E_ERR_FAIL_MAKING_PACKET occured.";
        case E_ERR_FAIL_SENDING_PACKET:
            return "E_ERR_FAIL_SENDING_PACKET occured.";
        case E_ERR_FAIL_SENDING_ACT_DONE:
            return "E_ERR_FAIL_SENDING_ACT_DONE occured.";
        case E_ERR_FAIL_CREATING_THREAD:
            return "E_ERR_FAIL_CREATING_THREAD occured.";
        case E_ERR_FAIL_INVOKING_SHELL:
            return "E_ERR_FAIL_INVOKING_SHELL occured.";
        case E_ERR_FAIL_MEM_ALLOC:
            return "E_ERR_FAIL_MEM_ALLOC occured.";
        case E_ERR_FAIL_CONVERTING_TIME:
            return "E_ERR_FAIL_CONVERTING_TIME occured.";
        case E_ERR_FAIL_EXECUTE_FUNC:
            return "E_ERR_FAIL_EXECUTE_FUNC occured.";
        case E_ERR_FAIL_INITE_GPIO_ROOT:
            return "E_ERR_FAIL_INITE_GPIO_ROOT occured.";
        case E_ERR_FAIL_INSPECTING_DATA:
            return "E_ERR_FAIL_INSPECTING_DATA occured.";
        case E_ERR_FAIL_TIME_UPDATE:
            return "E_ERR_FAIL_TIME_UPDATE occured.";

        case E_ERR_INVALID_CMD:
            return "E_ERR_INVALID_CMD occured.";
        case E_ERR_INVALID_VALUE:
            return "E_ERR_INVALID_VALUE occured.";
        case E_ERR_INVALID_NULL_VALUE:
            return "E_ERR_INVALID_NULL_VALUE occured.";
        case E_ERR_INVALID_ARG_COUNT:
            return "E_ERR_INVALID_ARG_COUNT occured.";
        case E_ERR_INVALID_INDEX_NUM:
            return "E_ERR_INVALID_INDEX_NUM occured.";

        case E_ERR_NOT_SUPPORTED_CMD:
            return "E_ERR_NOT_SUPPORTED_CMD occured.";
        case E_ERR_NOT_SUPPORTED_WHAT:
            return "E_ERR_NOT_SUPPORTED_WHAT occured.";
        case E_ERR_NOT_SUPPORTED_HOW:
            return "E_ERR_NOT_SUPPORTED_HOW occured.";
        case E_ERR_NOT_SUPPORTED_TYPE:
            return "E_ERR_NOT_SUPPORTED_TYPE occured.";
        case E_ERR_NOT_EXISTED_PROC:
            return "E_ERR_NOT_EXISTED_PROC occured.";

        case E_ERR_NOT_HAVE_MEMBER:
            return "E_ERR_NOT_HAVE_MEMBER occured.";
        case E_ERR_NOT_ARRAY:
            return "E_ERR_NOT_ARRAY occured.";

        case E_ERR_NEED_TIME_SYNC:
            return "E_ERR_NEED_TIME_SYNC occured.";

        case E_ERR_EMPTY_CMD:
            return "E_ERR_EMPTY_CMD occured.";
        case E_ERR_OUT_OF_SERVICE_VALVE:
            return "E_ERR_OUT_OF_SERVICE_VALVE occured.";
        case E_NO_ERROR:
            LOGD("E_NO_ERROR occured.");
            break;
        default:
            LOGERR("Not Supported E_ERROR type.");
            break;
        }

        return NULL;
    }

protected:
    /** Error message.
     */
    E_ERROR __err_n;
};

#endif // _C_EXCEPTION_H_
