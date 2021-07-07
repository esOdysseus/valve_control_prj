# Project
- Name : Valve-Controller

### Simple-Description
- This APP control GPIO-pin to open/close Valves.

### Features
1. CMD-receiver : Receive command to control valves.
2. Valve-Ctrller: Control valves to open/close it.
3. Guarantor    : Ensure that the valve that was opened is willing to close.

## License
```
Copyright [2021-] 
Written by EunSeok Kim <es.odysseus@gmail.com>
```

### Developer
- esOdysseus (email: es.odysseus@gmail.com)

### Latest Release
- version 0.2.1 (Date: 2021-06-12)

---
## Installation
> Please refer following commands.
> So, you can see the Application(app_valve_control) in release folder.
   - work : Project folder path.

```shell
$ cd ${work}
$ bash ./build.sh -m release -t valve -arch x86
```
### Library Dependency
- communicator : for common-communication on Ethernet.
- pthread      : for create pthread.

---
### Example
- Environments
   - Arguments
      1. Path-of-desp-Alias      : "${work}/common/lib/communicator/config/desp_alias.json"
      2. Path-of-desp-Protocol   : "${work}/common/lib/communicator/config/desp_UniversalCMD_protocol.json"
   - Export-Variables
      1. LD_LIBRARY_PATH : "${work}/common/lib/communicator/lib/x86"
      2. VALVE_GPIO_ROOT : "${work}/valve_controller/test/gpio"

- You can test the APP. by using following guide-line.
   - Case-01
      ```shell
      $ cd ${work}
      $ ./release/valve_controller/bin/app_valve_control ${Path-of-desp-Alias} ${Path-of-desp-Protocol}
      ```
   - Case-02
      ```shell
      $ cd ${work}
      $ ./launcher.sh  -s-path ./valve_controller  -s-name run_x86.sh -build release
      ```

---
## History
Date | Version | Description
:----|:----:|:----
`2021-06-11` | Ver 0.2.0 | Commit refactoring Version with new common-communicator.
`2021-06-12` | Ver 0.2.1 | Add Function that process coupling OPEN/CLOSE command-pair.
