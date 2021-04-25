#include <cassert>
#include <thread>
#include <chrono>
#include <list>
#include <string>
#include <fstream>
#include <iostream>
#include <stdio.h>

#include <logger.h>
#include <CProcValve.h>
#include <CVctrlCMD.h>
#include <CProcState.h>
#include <CProcService.h>
#include <MData.h>
#include <CException.h>
#include <CTranceiverCMD.h>

namespace proc_pkg {

using TimeType = time_pkg::CTime;
using TransType = cmd_pkg::CTranceiverCMD;
constexpr const unsigned int CProcValve::CMD_RELOADING_PERIOD;

/***********************************
 * Definition of Public Function
 */
CProcValve::CProcValve(void): IProc(SELF_NAME) {
    const char* RESERVED_CMDS_ROOT = getenv("RESERVED_CMDS_ROOT");
    _root_dbgk_cmd_.clear();

    if( RESERVED_CMDS_ROOT==NULL ) {
        LOGERR("RESERVED_CMDS_ROOT env-variable is NULL.");
        throw CException(E_ERROR::E_ERR_INVALID_NULL_VALUE);
    }
    _root_dbgk_cmd_ = RESERVED_CMDS_ROOT;
}

CProcValve::~CProcValve(void) {
    _root_dbgk_cmd_.clear();
}

std::shared_ptr<CProcValve::PacketType> CProcValve::make_packet_for_dbgcmd(std::shared_ptr<CMDDebug> &cmd) {
    double elapsed_time = -1.0;
    std::shared_ptr<CMDType> cmd_valve;
    std::shared_ptr<PacketType> packet = std::make_shared<PacketType>();
    std::shared_ptr<ArgsType> args;
    assert(cmd.get() != NULL);
    assert(cmd->get_cmd() == CMD_VCONTROL);

    try {
        // extract arguments
        args = cmd->get_arg<ArgsType>();

        // if need to save file, just do it.
        if ( args->need_save_file() == true ) {
            LOGW("Trig 'Writing DBGK-CMD' task, lazilly.");
            if( insert_cmd(cmd, TaskType::E_PROC_TASK_SAVE_FILE) != true ) {
                LOGERR("Inserting DBGK-CMD for saving-file is failed.");
                throw CException(E_ERROR::E_ERR_FAIL_INSERT_CMD);
            }
        }

        // check whether CMD-time is out-of-range for sending message to valve-device.
        elapsed_time = TimeType::elapsed_time(args->get_when(), TimeType::get<time_t>());
        if ( elapsed_time >= (double)(args->get_exe_maxlatency()) ) {
            LOGW("DBGK-CMD time(%f) is over than Execuable-Limit(%d seconds).", elapsed_time, args->get_exe_maxlatency());
            LOGW("We can't send Valve-CMD for DBGK-command.");
            throw CException(E_ERROR::E_NO_ERROR);
        }

        // check available of valve-device.
        if( check_service_available(args->get_who()) != true ) {
            LOGW("%s is out-of-service.", args->get_who().c_str());
            throw CException(E_ERROR::E_ERR_OUT_OF_SERVICE_VALVE);
        }

        // create valve-command.
        cmd_valve = create_valve_cmd(args);
        cmd_valve->set_flag(E_FLAG::E_FLAG_REQUIRE_ACK, 1);
        cmd_valve->set_flag(E_FLAG::E_FLAG_REQUIRE_ACT, 1);
        
        // encode valve-command & packing
        packet->proc_pointer = this;
        packet->asked_from = cmd->get_resp_dest();  // who ask this
        packet->request_to = args->get_who();       // who is received this
        packet->msg_id = cmd_valve->get_id();
        packet->data = cmd_valve->encode(packet->data_size);
        assert( packet->data != NULL && packet->data_size > 0 );

        // register packet to monitoring-data.
        assert( register_packet(packet, cmd_valve->get_flag()) == true );
        return packet;
    }
    catch (const CException &e) {
        if( e.get() == E_ERROR::E_NO_ERROR )
            throw;
        LOGERR("%s", e.what());
        packet.reset();
    }
    catch (const std::exception &e) {
        LOGERR("%s", e.what());
        packet.reset();
    }

    return packet;
}

void CProcValve::register_sent_msg(uint32_t msg_id, bool result) {
    using E_MNT_FLAG = monitor_pkg::CPacketMonitor::E_MNT_FLAG;
    if( _monitor_.update(msg_id, E_MNT_FLAG::EMNT_FLAG_SENT_OK, result) != true ) {
        LOGW("updating EMNT_FLAG_SENT_OK flag is failed.");
        throw CException(E_ERROR::E_ERR_FAIL_UPDATE_DATA);
    }
}

/***********************************
 * Definition of Protected Function
 */
bool CProcValve::data_update(TaskType type, std::shared_ptr<CMDType> cmd) {
    LOGD("Called.");
    uint32_t msg_id = cmd->get_id();
    std::shared_ptr<MNT::DataType> packet;
    CMDType::FlagType cmd_flag = cmd->get_flag(E_FLAG::E_FLAG_ACK_MSG | 
                                               E_FLAG::E_FLAG_ACTION_DONE |
                                               E_FLAG::E_FLAG_RESP_MSG);
    try {
        if( cmd_flag ) {    // for ACK/ACT-DONE/RESP
            // Check validation of flag in CPacketMD.
            flag_validation_check(msg_id, cmd_flag, cmd);

            // Set flag in CPacketMD.
            set_flag(msg_id, cmd_flag);

            // Set error-flag in CPacketMD.
            if ( (bool)(cmd->get_flag(E_FLAG::E_FLAG_STATE_ERROR)) == true ) {
                if ( _monitor_.update(msg_id, MNT_ERROR::EMNT_ERROR_STATE, 
                                    cmd->get_state()) != true ) {
                    LOGW("Updating of error-flag is failed.");
                    throw CException(E_ERROR::E_ERR_FAIL_UPDATE_DATA);
                }
            }

            if ( cmd_flag | E_FLAG::E_FLAG_RESP_MSG ) {
                // TODO need to decode of body in payload.

                // TODO need to re-act against the decoded cmd.

            }

            // TODO trig updating of CProcState

            // If the packet donesn't need to care, then remove the packet from CPacketMonitor.
            // [Attention] RESP-required packet is remained.
            if( removeable_from_monitor(msg_id) ) {
                if( _monitor_.remove(msg_id) != true ) {
                    LOGERR("Removing of packet(id=%d) is failed.", msg_id);
                    throw CException(E_ERROR::E_ERR_FAIL_UPDATE_DATA);
                }
            }
            LOGD("CPacketMD count[%d] in Monitor.", _monitor_.size());
        }
        else {      // for REQ/PUBLISH
            LOGW("Not Support about REQ&Publish packet.");
            // TODO Insert command for Request/Publish message.
            // if( insert_cmd_to_execute(cmd) != true ) {
            //     LOGERR("Inserting of REQ/PUB command is failed.");
            //     throw CException(E_ERROR::E_ERR_FAIL_INSERT_CMD);
            // }
        }

        return true;
    }
    catch ( const std::exception &e ) {
        LOGERR("%s", e.what());
    }

    return false;
}

bool CProcValve::register_packet(std::shared_ptr<PacketType> &packet, 
                                 CMDType::FlagType flag) {
    try {
        // Create new monitoring-data.
        if( _monitor_.insert(packet->msg_id, packet) != true ) {
            LOGW("Inserting packet to monitor-mapper is failed.");
            throw CException(E_ERROR::E_ERR_FAIL_INSERT_DATA);
        }

        // Set ACK requirement.
        if( flag & E_FLAG::E_FLAG_REQUIRE_ACK ) {
            if(_monitor_.update(packet->msg_id, MNT_FLAG::EMNT_FLAG_ACK_REQUIRE, 1) != true) {
                LOGERR("Updating ACK_require-flag is failed.");
                throw CException(E_ERROR::E_ERR_FAIL_UPDATE_DATA);
            }
        }

        // Set ACT requirement.
        if( flag & E_FLAG::E_FLAG_REQUIRE_ACT ) {
            if(_monitor_.update(packet->msg_id, MNT_FLAG::EMNT_FLAG_ACT_REQUIRE, 1) != true) {
                LOGERR("Updating ACT_require-flag is failed.");
                throw CException(E_ERROR::E_ERR_FAIL_UPDATE_DATA);
            }
        }

        // Set RESP requirement.
        if( flag & E_FLAG::E_FLAG_REQUIRE_RESP ) {
            if(_monitor_.update(packet->msg_id, MNT_FLAG::EMNT_FLAG_RESP_REQUIRE, 1) != true) {
                LOGERR("Updating RESP_require-flag is failed.");
                throw CException(E_ERROR::E_ERR_FAIL_UPDATE_DATA);
            }
        }
        return true;
    }
    catch (const std::exception &e) {
        LOGERR("%s", e.what());
    }

    return false;
}

bool CProcValve::create_custom_threads(void) {
    LOGD("Called.");
    assert( _is_continue_ == true );

    this->_runner_filemanager_ = std::thread(&CProcValve::run_periodical_file_manager, this);
    if ( this->_runner_filemanager_.joinable() == false ) {
        LOGW("run_file_manager thread creating is failed.");
        throw CException(E_ERROR::E_ERR_FAIL_CREATING_THREAD);
    }
    _is_custom_thread_ = true;
    LOGD("Done.");
    return _is_custom_thread_;
}

void CProcValve::destroy_custom_threads(void) {
    LOGD("Called.");
    assert( _is_continue_ == false );

    _is_custom_thread_ = false;
    if(_runner_filemanager_.joinable() == true) {
        _runner_filemanager_.join();
    }
    LOGD("Done.");
}

/***********************************
 * Definition of Private Function
 */
void CProcValve::flag_validation_check(uint32_t msg_id, 
                                       CMDType::FlagType cmd_flag, 
                                       std::shared_ptr<CMDType> &cmd) {
    LOGD("CPacketMD count[%d] in Monitor.", _monitor_.size());

    // Check existence of the-packet.
    if ( _monitor_.is_there(msg_id) == false ) {
        LOGERR("CPacketMD(key=%d) is not exist.", msg_id);
        throw CException(E_ERROR::E_ERR_NOT_HAVE_MEMBER);
    }
    // check ACK requir-flag.
    if ( cmd_flag & (CMDType::FlagType)E_FLAG::E_FLAG_ACK_MSG ) {
        if( (bool)(_monitor_.get(msg_id, MNT_FLAG::EMNT_FLAG_ACK_REQUIRE)) == false ) {
            LOGERR("CPacketMD(key=%d) is not require ACK-msg. But, it's received.", msg_id);
            throw CException(E_ERROR::E_ERR_INVALID_CMD);
        }
    }
    // check ACT-DONE requir-flag.
    if ( cmd_flag & (CMDType::FlagType)E_FLAG::E_FLAG_ACTION_DONE ) {
        if( (bool)(_monitor_.get(msg_id, MNT_FLAG::EMNT_FLAG_ACT_REQUIRE)) == false ) {
            LOGERR("CPacketMD(key=%d) is not require ACT-DONE-msg. But, it's received.", msg_id);
            throw CException(E_ERROR::E_ERR_INVALID_CMD);
        }
    }
    // check RESP requir-flag.
    if ( cmd_flag & (CMDType::FlagType)E_FLAG::E_FLAG_RESP_MSG ) {
        if( (bool)(_monitor_.get(msg_id, MNT_FLAG::EMNT_FLAG_RESP_REQUIRE)) == false ) {
            LOGERR("CPacketMD(key=%d) is not require RESP-msg. But, it's received.", msg_id);
            throw CException(E_ERROR::E_ERR_INVALID_CMD);
        }
    }
}

void CProcValve::set_flag(uint32_t msg_id, CMDType::FlagType cmd_flag) {

    if ( cmd_flag & (CMDType::FlagType)E_FLAG::E_FLAG_ACK_MSG ) {
        // Set acknowledge flag
        if ( _monitor_.update(msg_id, MNT_FLAG::EMNT_FLAG_ACK_RCV, 1) != true ) {
            LOGW("Updating of ack-received flag is failed.");
            throw CException(E_ERROR::E_ERR_FAIL_UPDATE_DATA);
        }
    }

    if ( cmd_flag & (CMDType::FlagType)E_FLAG::E_FLAG_ACTION_DONE ) {
        // Set action-done flag
        if ( _monitor_.update(msg_id, MNT_FLAG::EMNT_FLAG_ACT_DONE_RCV, 1) != true ) {
            LOGW("Updating of act-done-received flag is failed.");
            throw CException(E_ERROR::E_ERR_FAIL_UPDATE_DATA);
        }
    }

    if ( cmd_flag & (CMDType::FlagType)E_FLAG::E_FLAG_RESP_MSG ) {
        // Set response flag
        if ( _monitor_.update(msg_id, MNT_FLAG::EMNT_FLAG_RESP_RCV, 1) != true ) {
            LOGW("Updating of resp-received flag is failed.");
            throw CException(E_ERROR::E_ERR_FAIL_UPDATE_DATA);
        }
    }
}

bool CProcValve::removeable_from_monitor(uint32_t msg_id) {
    assert(msg_id > 0);

    try {
        // RESP-required packet don't touch.
        if( (bool)(_monitor_.get(msg_id, MNT_FLAG::EMNT_FLAG_RESP_REQUIRE)) == true ) {
            return false;
        }

        if( (bool)(_monitor_.get(msg_id, MNT_FLAG::EMNT_FLAG_ACT_REQUIRE)) == true ) {
            if( (bool)(_monitor_.get(msg_id, MNT_FLAG::EMNT_FLAG_ACT_DONE_RCV)) != true ) {
                return false;
            }
        }

        if( (bool)(_monitor_.get(msg_id, MNT_FLAG::EMNT_FLAG_ACK_REQUIRE)) == true ) {
            if( (bool)(_monitor_.get(msg_id, MNT_FLAG::EMNT_FLAG_ACK_RCV)) != true ) {
                return false;
            }
        }
    }
    catch( const std::exception &e ) {
        LOGERR("%s", e.what());
        throw CException(E_ERROR::E_ERR_FAIL_INSPECTING_DATA);
        return false;
    }

    return true;
}

bool CProcValve::check_service_available(std::string dest_app_name) {
    IProc *proc_inf = NULL;

    try {
        if ( (proc_inf = find_proc(CProcService::SELF_NAME)) == NULL ) {
            LOGW("There is not exist instance of \"%s\".", CProcService::SELF_NAME);
            return false;
        }

        // check service available.
        CProcService *proc_svc = (CProcService*)proc_inf;
        return proc_svc->is_available_service(dest_app_name);
    }
    catch (const std::exception &e) {
        LOGERR("%s", e.what());
    }

    return false;
}

std::shared_ptr<CProcValve::CMDType> CProcValve::create_valve_cmd(std::shared_ptr<ArgsType> &args) {
    std::shared_ptr<CMDType> cmd_valve;

    try {
        // CMD-Assumption => valve-ctrl  dest-valve  0~3  open/close  after-sec
        cmd_valve = std::make_shared<CMDType>(get_app_name(), args->get_who(), 
                                              E_FLAG::E_FLAG_REQUIRE_ACK);
        if( cmd_valve.get() == NULL ) {
            LOGERR("Memory allocation of \"cmd_valve\" is failed.");
            throw CException(E_ERROR::E_ERR_FAIL_MEM_ALLOC);
        }

        // Set valve-command according to debug-cmd.
        assert( cmd_valve->set_when(args->get_when()) == true );
        assert( cmd_valve->set_what(args->get_what()) == true );
        assert( cmd_valve->set_how(args->get_how()) == true );
        assert( cmd_valve->set_why("none") == true );
    }
    catch ( const std::exception &e ) {
        LOGERR("%s", e.what());
        cmd_valve.reset();
        throw dynamic_cast<const CException&>(e);
    }

    return cmd_valve;
}

#define KEY_OWNER       "owner"
#define KEY_DBGK_CMD    "dbgk-cmd"
#define DBGK_FILE_FORMAT    "{ \"" KEY_OWNER "\":\"%s\", \"" KEY_DBGK_CMD "\":\"%s\" }"

static void _gurantee_no_LF_CR_(std::string &str) {
    constexpr char target[] = "\n";
    std::size_t found = std::string::npos;
    assert( str.empty() == false );

    found = str.find(target);
    if( found == std::string::npos ) {
        return ;
    }

    while( found != std::string::npos ) {
        str.replace(found, sizeof(target), " ");
        found = str.find(target);
    }
}

static std::string _make_json_(std::string owner_str, std::string cmd_str) {
    int written_size = -1;
    ssize_t buf_size = sizeof(DBGK_FILE_FORMAT) + 256;
    char buf[buf_size] = {0,};

    try {
        _gurantee_no_LF_CR_(owner_str);
        _gurantee_no_LF_CR_(cmd_str);
        written_size = snprintf(buf, buf_size, DBGK_FILE_FORMAT, owner_str.data(), cmd_str.data());
        if( written_size <= 0 || written_size >= buf_size ) {
            LOGERR("snprintf(%d) is failed.", written_size);
            throw CException(E_ERROR::E_ERR_FAIL_EXECUTE_FUNC);
        }
    }
    catch( const std::exception &e ) {
        LOGERR("%s", e.what());
        throw;
    }

    return std::string(buf);
}

static std::shared_ptr<CProcValve::CMDDebug> _parse_json_(const char* json_str, ssize_t json_size) {
    std::string cmd_str;
    std::string owner_str;
    std::shared_ptr<json_mng::CMjson> json_manager;
    std::shared_ptr<CProcValve::CMDDebug> dbg_cmd;
    assert(json_str != NULL);
    assert(json_size > 0 && strlen(json_str) == json_size);

    try {
        json_manager = std::make_shared<json_mng::CMjson>();
        assert(json_manager->parse( json_str, json_size ) == true);

        owner_str = json_manager->get_member(KEY_OWNER)->data();
        cmd_str = json_manager->get_member(KEY_DBGK_CMD)->data();
        assert( owner_str.empty() == false && cmd_str.empty() == false );
        assert( owner_str.length() > 0 && cmd_str.length() > 0 );

        dbg_cmd = std::make_shared<CProcValve::CMDDebug>( owner_str, 
                                                          cmd_str.data(), 
                                                          cmd_str.length());
        assert( dbg_cmd.get() != NULL );
    }
    catch ( const std::exception &e ) {
        LOGERR("%s", e.what());
        throw;
    }

    return dbg_cmd;
}

std::list<std::string>& CProcValve::make_rawdata(std::shared_ptr<CMDDebug> &cmd, 
                                                 std::list<std::string> &event_cmds,
                                                 std::list<std::string> &period_cmds,
                                                 std::string &rawdata) {
    LOGD("Called.");
    std::string when_type;
    std::shared_ptr<ArgsType> args;

    try {
        args = cmd->get_arg<ArgsType>();
        when_type = args->get_when_type();
        
        // get string of command to write.
        if ( when_type == VALVE_WHEN_EVENT ) {
            rawdata = _make_json_( cmd->get_resp_dest().data(), cmd->get_full_cmd().data() );
            return event_cmds;
        }
        else if ( when_type == VALVE_WHEN_PERIOD ) {
            rawdata = _make_json_( cmd->get_resp_dest().data(), cmd->get_full_cmd().data() );
            return period_cmds;
        }
        else if( when_type == VALVE_WHEN_NOW ) {
            rawdata = _make_json_( cmd->get_resp_dest().data(), args->cvt_event_cmdstr().data() );
            return event_cmds;
        }
        else {
            LOGERR("Not Supported type(%s) of when.", when_type.data());
            throw CException(E_ERROR::E_ERR_NOT_SUPPORTED_TYPE);
        }
    }
    catch( const std::exception &e ) {
        LOGERR("%s", e.what());
        throw;
    }
}

bool CProcValve::dbgk_cmd_insert(std::shared_ptr<CMDDebug> &cmd, 
                                 std::list<std::string> &event_cmds,
                                 std::list<std::string> &period_cmds) {
    LOGD("Called.");
    double elapsed_time = 0.0;
    std::string cmd_full_str;
    std::shared_ptr<CMDDebug> cmd_infile;
    std::shared_ptr<ArgsType> args;
    std::shared_ptr<ArgsType> args_infile;
    

    try {
        args = cmd->get_arg<ArgsType>();
        std::list<std::string> &p_cmd_list = make_rawdata(cmd, event_cmds, period_cmds, 
                                                          cmd_full_str);

        // search index in list to insert cmd.
        auto itor=p_cmd_list.begin();
        for(; itor!=p_cmd_list.end(); itor++ ) {
            // compare when
            cmd_infile = _parse_json_( (*itor).data(), (*itor).length() );
            args_infile = cmd_infile->get_arg<ArgsType>();
            elapsed_time = TimeType::elapsed_time(args->get_when(), args_infile->get_when());
            if( elapsed_time < 0.0 ) {
                break;
            }
            else if( elapsed_time == 0.0 ) {
                if( args->get_who() != args_infile->get_who() )
                    continue;
                if( args->get_what() != args_infile->get_what() )
                    continue;

                LOGW("Both of CMDs. is crashed about Time.");
                throw CException(E_ERROR::E_ERR_INVALID_CMD);
            }
        }

        // insert cmd to list.
        p_cmd_list.insert(itor, cmd_full_str);
        return true;
    }
    catch( const std::exception &e ) {
        LOGERR("%s", e.what());
        throw;
    }

    return false;
}

void CProcValve::dbgk_cmds_insert(std::list<std::string> &event_cmds, std::list<std::string> &period_cmds) {
    std::shared_ptr<DCMD_STType> data;
    
    std::unique_lock<std::mutex> lock(_mtx_dbgk_cmd_list_);
    while( _dbgk_cmd_list_.size() ) {
        data.reset();
        data = *(_dbgk_cmd_list_.begin());
        _dbgk_cmd_list_.pop_front();
        lock.unlock();

        try {
            // Check WRITE_TASK & append cmd to list.
            assert( data->task_type == TaskType::E_PROC_TASK_SAVE_FILE );
            if( dbgk_cmd_insert(data->cmd, event_cmds, period_cmds) != true ) {
                LOGW("DBGK-CMD. inserting is failed.");
                throw CException(E_ERROR::E_ERR_FAIL_INSERT_CMD);
            }
        }
        catch ( const CException &e ) {
            if( e.get() != E_ERROR::E_ERR_INVALID_CMD ) {
                LOGERR("%s", e.what());
                throw;
            }
        }
        catch ( const std::exception &e ) {
            LOGERR("%s", e.what());
            throw;
        }

        lock.lock();
    }
    lock.unlock();
    
}

void CProcValve::dbgk_cmds_load(std::list<std::string> &cmds_list, bool mid_breakable, bool removeable) {
    std::shared_ptr<CMDDebug> cmd_infile;
    std::shared_ptr<ArgsType> args_infile;
    std::list<std::string>::iterator remove_itor;
    double now_t = TimeType::get<double>();
    assert (now_t != 0.0);

    for(auto itor=cmds_list.begin(); itor!=cmds_list.end(); ) {
        // compare when
        cmd_infile.reset();
        args_infile.reset();
        cmd_infile = _parse_json_( (*itor).data(), (*itor).length() );
        args_infile = cmd_infile->get_arg<ArgsType>();

        if( TimeType::elapsed_time(args_infile->get_when(), now_t) <= ((double)CMD_RELOADING_PERIOD+10.0) ) {
            LOGD("DBGK-msg to send : %s", cmd_infile->get_full_cmd().data());
            
            // trig sending valve-cmd
            args_infile->disable_need_save();   // it disable flag to not save file with this-command.
            assert( args_infile->need_save_file() == false );

            // insert cmd to tranceiver-list.
            if( TransType::insert_reloaded_cmd(cmd_infile) != true) {
                LOGERR("inserting DBGK-CMD to cmd-list of Tranceiver is failed.");
                throw CException(E_ERROR::E_ERR_FAIL_INSERT_CMD);
            }

            // Remove CMD from list.
            remove_itor = itor;
            itor++;
            if( removeable ) {
                cmds_list.erase(remove_itor);
            }
        }
        else {
            itor++;
            if( mid_breakable ) {
                break;
            }
        }
    }
}

int CProcValve::run_periodical_file_manager(void) { // Thread: DBGK-command read/write-routin, periodically.
    LOGD("Called.");
    std::string period_cmds_file;
    std::string event_cmds_file;
    unsigned int sleep_count = 0;
    unsigned int sleep_sec = 3;
    unsigned int sleep_max_count = CMD_RELOADING_PERIOD / sleep_sec;
    assert( _root_dbgk_cmd_.empty() == false );

    period_cmds_file = _root_dbgk_cmd_ + "/" + CProcValve::DBGK_PERIOD_CMDS_FILE;
    event_cmds_file = _root_dbgk_cmd_ + "/" + CProcValve::DBGK_EVENT_CMDS_FILE;

    while(_is_continue_) {
        try {
            if ( sleep_count >= sleep_max_count ) { // wait 1 minute
                file_managing(period_cmds_file, event_cmds_file); // do processing
                sleep_count = 0;
            }
            std::this_thread::sleep_for(std::chrono::seconds(sleep_sec));   // sleep 3 second
            sleep_count++;
        }
        catch ( const std::exception &e ) {
            LOGERR("%s", e.what());
            std::this_thread::sleep_for(std::chrono::seconds(1));      // 1 second sleep
        }
    }
}

void CProcValve::file_managing(std::string &period_cmds_file_path, std::string &event_cmds_file_path) {
    std::list<std::string> period_cmds;
    std::list<std::string> event_cmds;

    // read files
    if( read_file(period_cmds_file_path, period_cmds) != true ) {
        LOGERR("File(%s)-read is failed.", period_cmds_file_path.c_str());
        throw CException(E_ERROR::E_ERR_FAIL_READ_FILE);
    }
    if( read_file(event_cmds_file_path, event_cmds) != true ) {
        LOGERR("File(%s)-read is failed.", event_cmds_file_path.c_str());
        throw CException(E_ERROR::E_ERR_FAIL_READ_FILE);
    }

    // Insert cmds to lists.
    dbgk_cmds_insert(event_cmds, period_cmds);
    // Check reload-CMD period & Load validated-CMD from file.
    dbgk_cmds_load(period_cmds, false, false);
    dbgk_cmds_load(event_cmds , true , true);

    // write files
    if( write_file(period_cmds_file_path, period_cmds) != true ) {
        LOGERR("File(%s)-write is failed.", period_cmds_file_path.data());
        throw CException(E_ERROR::E_ERR_FAIL_WRITE_FILE);
    }
    if( write_file(event_cmds_file_path, event_cmds) != true ) {
        LOGERR("File(%s)-write is failed.", event_cmds_file_path.data());
        throw CException(E_ERROR::E_ERR_FAIL_WRITE_FILE);
    }

    // clear local-variables & wait next time.
    period_cmds.clear();
    event_cmds.clear();
}

bool CProcValve::read_file(std::string &file_path, std::list<std::string> &cmd_list) {
    std::ifstream fd;
    try {
        fd.open(file_path.data());
        if( fd.is_open() != true ) {
            LOGERR("File-Open is failed.(%s)", file_path.data());
            throw CException(E_ERROR::E_ERR_FAIL_OPEN_FILE);
        }

        // Read data line by line.
        std::string str_line;
        while(std::getline(fd, str_line)){
            cmd_list.push_back(str_line);
        }
        fd.close();
        return true;
    }
    catch( const std::exception &e ) {
        LOGERR("%s", e.what());
        if( fd.is_open() )
            fd.close();
    }
    return false;
}

bool CProcValve::write_file(std::string &file_path, std::list<std::string> &cmd_list) {
    std::ofstream fd;
    try{
        if( cmd_list.size() <= 0 ) {
            LOGW("CMD-list is empty.");
            return true;
        }

        fd.open(file_path.data());
        if( fd.is_open() != true ) {
            LOGERR("File-Open is failed.(%s)", file_path.data());
            throw CException(E_ERROR::E_ERR_FAIL_OPEN_FILE);
        }

        // Write data line by line.
        for ( auto itor=cmd_list.begin(); itor != cmd_list.end(); itor++ ) {
            fd << *itor;
            fd << "\n";
        }
        fd.close();
        return true;
    }
    catch( const std::exception &e ) {
        LOGERR("%s", e.what());
        if( fd.is_open() )
            fd.close();
    }
    return false;
}


}
