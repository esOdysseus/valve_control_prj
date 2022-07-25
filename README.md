# valve_control_prj

## Description
Automatic control valves according to reserved schedule.
This project contains two-kinds of program like cmd_scheduler and valve_controller.
About each of program, Description is following.
 - cmd_scheduler : It interact with User and manage schedule as command for controlling valve.
 - valve_controller : It execute command to control valve that command is received from cmd_scheduler.
 
 
## Environment Variables
 - MACHINE_DEVICE_NAME : It indicate now H/W machine-name that is shared all of processes on the machine.


## Prerequisites
 - libc.so.6: version `GLIBC_2.28' (required by libsqlite3.so)


## Test Driven Development
 > With CMD-Scheduler
 - **echo -e "Principle6_json" | nc -u 192.168.1.2 20000**
 - **cat ./Principle6_test.txt | nc -u 192.168.1.2 20000**
    ```
    {   // Principle6_test.txt
        "version": "1.0.0",
        "who": {
            "app": "string",            // APP-path in alias
            "pvd": "string",            // Provider-id in alias
            "func": "string"            // Optional (for Service-oriented)
        },
        "when": {
            "type": "routine.week",           // valid-values : [ one-time, routine.week, routine.day, specific ]
            "time": {
                // "latency": "10.0",     // unit : second  (for one-time)
                "week": "mon",        // valid-values : (for routine.week) [ mon, tues, wednes, thurs, fri, satur, sun ]
                "period" : "2",   // valid-values : (for routine) [ 1 <= X ]: 2 week/day routine ...
                "date" : "2021-09-13",     // valid-values : (for routine & specific)
                "time" : "15:30:23"     // valid-values : (for routine & specific)
            }
        },
        "where": {
            "type": "center.gps",           // valid-values : [ 'center.gps', 'unknown', 'dont.care', 'db' ]
            "contents": {               // center.gps : 일때, 장소의 중심 좌표만 기록한다.
                "long": "123456789.123456",
                "lat": "123456789.123456"
            }
        },
        "what": {
            "type": "valve.swc",           // valid-values : [ 'valve.swc', 'db' ]
            "contents": {               // valve.swc : 일때, 어떤 switch를 선택할지를 나타낸다.
                "seq": "1"
            }
        },
        "how": {
            "type": "valve.swc",           // valid-values : [ 'valve.swc', 'db' ]
            "contents": {               // valve.swc : 일때, Action의 Start~Stop까지 묶어준다.
                "method-pre": "open",   // valid-values : [ open , close, none ]
                "costtime": "40.0",   // valid-values : seconds + point value
                                        //                0.0 : when method is close
                "method-post": "close"  // valid-values : [ open , close, none ]
            }
        },
        "why": {                        // Optional
            "desp": "Why description",
            "objective": ["key-words" ],
            "dependency": ["key-words" ]
        }
    }
    ```
 > With Valve-Controller
 