#ifndef _C_COMMAND_H_
#define _C_COMMAND_H_

#include <time_kes.h>

#define SOF_SIZE        4


/** Command-Type according to SOF */
#define CMD_UNIVERSE    "UCMD"
#define CMD_DEBUG       "DBGK"


/******************************
 * for DBGK command 
 * 
 ******* CMD: valve-ctrl *********
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
 */
#define CMD_STATE       "state"
// #define CMD_SERVICES    "services"
#define CMD_VCONTROL    "valve-ctrl"
#define CMD_VCONTROL_ARGCNT 5


/** WHAT-value for Valve */
typedef enum E_WHAT_VALVE {
    E_VALVE_NULL = -1,
    E_VALVE_LEFT_01 = 0,
    E_VALVE_LEFT_02 = 1,
    E_VALVE_LEFT_03 = 2,
    E_VALVE_LEFT_04 = 3,
    E_VALVE_CNT = 4
} E_WHAT_VALVE;


/** HOW-value for Valve */
#define VALVE_HOW_OPEN      "open"
#define VALVE_HOW_CLOSE     "close"


/** WHEN-types for Valve */
#define VALVE_WHEN_NOW      "now"
#define VALVE_WHEN_PERIOD   "period"
#define VALVE_WHEN_EVENT    "event"


#endif // _C_COMMAND_H_