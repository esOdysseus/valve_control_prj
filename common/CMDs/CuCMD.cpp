#include <cassert>
#include <random>
#include <memory>
#include <iostream>
#include <string.h>

#include <logger.h>
#include <CMDs/CuCMD.h>
#include <CException.h>
#include <IProtocolInf.h>
#include <Common.h>
#include <time_kes.h>


namespace cmd {


constexpr const char* CuCMD::NAME;
constexpr const char* CuCMD::PROTOCOL_NAME;


/**********************************
 * Definition of Public Function.
 */
CuCMD::CuCMD(std::string my_app_path, std::string my_pvd_id)
: ICommand( my_app_path, my_pvd_id ) {
    clear();
}

CuCMD::CuCMD( alias::CAlias& myself, FlagType flag_val)
: ICommand( myself ) {
    clear();

    _msg_id_ = gen_random_msg_id();
    _flag_ = flag_val;
    _state_ = 0;
}

// Copy Constructor
CuCMD::CuCMD( const CuCMD& cmd )
: ICommand( cmd.get_from() ) {
    clear();

    // copy command
    _flag_ = cmd._flag_;
    _state_ = cmd._state_;
    _msg_id_ = cmd._msg_id_;
    _send_time_d_ = cmd._send_time_d_;
    
    _who_ = cmd._who_;
    _when_ = cmd._when_;
    _where_ = cmd._where_;
    _what_ = cmd._what_;
    _how_ = cmd._how_;
    _why_ = cmd._why_;
    this->set_flag_parse( cmd.is_parsed(), cmd.get_rcv_time() );
}

CuCMD::~CuCMD(void) {
    clear();
}

// presentator
bool CuCMD::decode(std::shared_ptr<IProtocolInf>& protocol) {
    if( protocol.get() == NULL ) {
        LOGW("Protocol is empty.");
        return false;
    }

    try {
        size_t payload_size = 0;
        std::string from_full_path;
        const char* payload = (const char*)protocol->get_payload(payload_size);

        if( is_parsed() == false) {
            // unpacking
            _flag_ = std::stoi(protocol->get_property("flag"), nullptr, 10);
            _state_ = std::stoi(protocol->get_property("state"), nullptr, 10);
            _msg_id_ = std::stoi(protocol->get_property("msg_id"), nullptr, 10);
            from_full_path = protocol->get_property("from");
            // Don't need to convert from_full_path to app_path & pvd_id.
            // parsing when time.
            _send_time_d_ = std::stod(protocol->get_property("when"));

            if( payload != NULL && payload_size > 0 ) {
                // parsing json payload (where, what, how, why)
                Json_DataType json_manager;
                json_manager = std::make_shared<json_mng::CMjson>();
                LOGD("strlen(payload)=%d , length=%d", strlen(payload), payload_size);
                assert( json_manager->parse(payload, payload_size) == true);

                // check UniversalCMD version.
                auto ver = extract_version(json_manager);
                if( ver != version() ) {
                    std::string err = "VERSION(" + ver + ") of json-context != " + version();
                    throw std::invalid_argument(err);
                }
                // parse principle-6.
                _who_ = extract_who(json_manager);
                _when_ = extract_when(json_manager, _send_time_d_);
                _where_ = extract_where(json_manager);
                _what_ = extract_what(json_manager);
                _how_ = extract_how(json_manager);
                _why_ = extract_why(json_manager);
                LOGD( "Success parse of Json buffer." );
            }

            // mark receive-time of this packet using my-system time.
            set_flag_parse( true );
        }
    }
    catch ( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw CException(E_ERROR::E_ERR_FAIL_DECODING_CMD);
    }

    return is_parsed();
}

std::shared_ptr<payload::CPayload> CuCMD::encode( std::shared_ptr<ICommunicator>& handler ) {
    std::shared_ptr<payload::CPayload> message;
    std::shared_ptr<IProtocolInf> protocol;

    if( handler.get() == NULL ) {
        LOGW("Communicator is not exist.");
        return message;
    }

    try {
        message = handler->create_payload();
        if( message.get() == NULL ) {
            throw std::logic_error("Message-Creating is failed.");
        }
        protocol = message->get(PROTOCOL_NAME);
    
        protocol->set_property("flag", _flag_);
        protocol->set_property("state", _state_);
        protocol->set_property("msg_id", _msg_id_);

        if ( get_flag(E_FLAG_ACK_MSG) == false && 
             get_flag(E_FLAG_ACTION_DONE) == false && 
             get_flag(E_FLAG_KEEPALIVE) == false ) {
            const char* body = NULL;
            Json_DataType json_manager;

            // make json body (where, what, how, why)
            json_manager = std::make_shared<json_mng::CMjson>();
            assert( json_manager.get() != NULL );
            // set UniversalCMD version.
            assert(apply_version(json_manager) == true);
            // set principle-6.
            assert(apply_who(json_manager, _who_) == true);
            assert(apply_when(json_manager, _when_) == true);
            assert(apply_where(json_manager, _where_) == true);
            assert(apply_what(json_manager, _what_) == true);
            assert(apply_how(json_manager, _how_) == true);
            assert(apply_why(json_manager, _why_) == true);
            assert( (body = json_manager->print_buf()) != NULL );

            protocol->set_payload( body, strlen(body) );
        }
    }
    catch ( const std::exception& e ) {
        LOGERR("%s", e.what());
        message.reset();
        protocol.reset();
        throw CException(E_ERROR::E_ERR_FAIL_ENCODING_CMD);
    }

    return message;
}

// getter
CuCMD::FlagType CuCMD::get_flag(FlagType pos) {
    assert( pos != (FlagType)E_FLAG::E_FLAG_NONE);
    return _flag_ & pos;
}

// setter
void CuCMD::set_id(unsigned long value) { 
    _msg_id_ = value;
    if( _msg_id_ == 0 ) {
        _msg_id_ = gen_random_msg_id();
    }
}

void CuCMD::set_flag(E_FLAG pos, FlagType value) {
    int shift_cnt = 0;

    if ( pos == E_FLAG::E_FLAG_NONE ) {
        _flag_ = value;
    }
    else {
        // Assumption : pos is continuous-bitmask.
        while( ((1<<shift_cnt) & pos) == 0 ) {
            shift_cnt++;
            assert( shift_cnt < (sizeof(FlagType)*8) );
        }

        _flag_ = (_flag_ & (~pos)) | ((value << shift_cnt) & pos);
    }
}

void CuCMD::set_state(uint16_t value) {
    _state_ = value;
}

// printer
std::string CuCMD::print_send_time(void) {  // print when-data for human-readable.
    assert( is_parsed() == true );
    char when_string[100];

    try {
        auto time_tm = time_pkg::CTime::convert<struct tm>( _send_time_d_ );
        strftime(when_string, 100, "CMD-When is [%B %d, %Y] time is [%T]", &time_tm);
        LOGD( "%s", when_string );
        return when_string;
    }
    catch( const std::exception &e ) {
        LOGERR("print_send_time is failed." );
    }

    return std::string();
}


/***********************************
 * Definition of Private Function.
 */
void CuCMD::clear(void) {
    _msg_id_ = 0;
    _flag_ = E_FLAG::E_FLAG_NONE;
    _state_ = 0;
    _send_time_d_ = 0.0;

    _who_.reset();
    _when_.reset();
    _where_.reset();
    _what_.reset();
    _how_.reset();
    _why_.reset();
    set_flag_parse(false);
}

uint32_t CuCMD::gen_random_msg_id(void) {
    uint32_t new_msgid = 0;
    int32_t id_min = 1;
    int32_t id_max = 2147483647;
    std::random_device rd;  //Will be used to obtain a seed for the random number engine
    std::mt19937 gen( rd() ); 
    std::uniform_int_distribution<> dist(id_min, id_max); 
    
    LOGD("Random MSG-ID Min : %d", (uint32_t)(dist.min()) );
    LOGD("Random MSG-ID Max : %d", (uint32_t)(dist.max()) );

    new_msgid = (uint32_t)(dist( gen ));
    assert( ((uint32_t)id_min) <= new_msgid && new_msgid <= ((uint32_t)id_max) );
    LOGI("Generated new MSG-ID=%d", new_msgid);

    return new_msgid;
}


} // namespace cmd
