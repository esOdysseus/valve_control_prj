#ifndef _CLASS_DEBUG_KES_ARGUMENTS_FOR_VALVE_CONTROL_H_
#define _CLASS_DEBUG_KES_ARGUMENTS_FOR_VALVE_CONTROL_H_

#include <CCmd.h>
#include <IDBGKarg.h>

namespace cmd_pkg {

/****** CMD: valve-ctrl *********
 *** Property Sequence *
 *      Magic-Code      : ex) DBGK
 *      CMD-type        : ex) valve-ctrl
 *      Who-name(ARG-01): ex) alias_udp_valve
 *      What-index      : valid-range = [0~3]
 *      How-type        : valid-value = [open, close]
 *      When-type       : valid-value = [now, period, event]
 *        [now] ACT-latency   : unit is second
 *        [period] week       : valid-range = [0~6] // 일~토
 *                 time       : format = [hour:minute:second]
 *        [event] date        : format = [year-month-day]
 *                time        : format = [hour:minute:second]
 * 
 *** Example *
 *    DBGK valve-ctrl alias_udp_valve 0 open now 10
 *      => Valve-Controller가 valve 0번을 지금부터 10초 후에 Open해라.
 *    DBGK valve-ctrl alias_udp_valve 1 close period 2 13:45:00
 *      => Valve-Controller가 valve 1번을 매주-수요일 13시45분에 Close해라.
 *    DBGK valve-ctrl alias_udp_valve 3 open event 2020-04-25 14:05:00
 *      => Valve-Controller가 valve 3번을 2020년 4월 25일 14시05분에 Open해라.
 * 
 *********************************/

class CDBGKargVctrl : public IDBGKarg {
public:
    static constexpr const char* SELF_NAME = "CDBGKargVctrl";

public:
    using E_TARGET = E_WHAT_VALVE;
    using E_WEEK = time_pkg::E_WEEK_TIME_VALVE;
    static constexpr const char* HOW_VALUES[] = { VALVE_HOW_OPEN, VALVE_HOW_CLOSE, NULL };
    static constexpr const char* WHEN_TYPES[] = { VALVE_WHEN_NOW, VALVE_WHEN_PERIOD, VALVE_WHEN_EVENT };
    static constexpr const char* TIME_FORMAT = "%T";
    static constexpr const char* DATE_FORMAT = "%Y-%m-%d";

public:
    CDBGKargVctrl(std::string &cmd, 
                  std::shared_ptr<std::list<std::string>> &args, 
                  bool disable_need_save=false);

    ~CDBGKargVctrl(void);

    void disable_need_save(void) { _need_save_ = false; }

    bool need_save_file(void) override;

    std::string get_who(void) { return _who_; }

    E_TARGET get_what(void) { return _what_; }

    std::string get_how(void) { return _how_; }

    time_t get_when(void) { return _when_; }

    std::string get_when_type(void) { return _when_type_; }

    unsigned int get_exe_maxlatency(void) { return _max_latency_sec_; }

    std::string cvt_event_cmdstr(void);

private:
    CDBGKargVctrl(void) = delete;

    void clear(void);

    bool validation_check(std::string &str, const char* const candidates[]) const;

    E_TARGET extract_what(void);

    std::string extract_how(void);

    std::string extract_when_type(void);

private:
    std::string _who_;

    E_TARGET _what_;

    std::string _how_;

    std::string _when_type_;

    time_t _when_;

    bool _need_save_;

    unsigned int _max_latency_sec_;

};


}

#endif // _CLASS_DEBUG_KES_ARGUMENTS_FOR_VALVE_CONTROL_H_