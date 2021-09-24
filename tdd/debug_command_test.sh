#!/bin/bash
ROOT_PATH=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

CMD_FILE_PATH=./Principle6_test.txt
IP_CMD_SCHEDULER=192.168.1.2
PORT_CMD_SCHEDULER=20000        # for def_debugger Provider.

# Try to send UDP-command according to FILE_PATH to CMD_SCHEDULER.
cat ${CMD_FILE_PATH} | nc -u ${IP_CMD_SCHEDULER} ${PORT_CMD_SCHEDULER}

