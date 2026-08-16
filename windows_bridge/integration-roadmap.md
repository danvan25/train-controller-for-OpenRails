# Open Rails Train Controller — Integration Roadmap

This document is the living implementation order for the Pico 2 train controller and the Windows/Open Rails integration.

## Current architecture

```text
Physical controls
    -> Raspberry Pi Pico 2 firmware
    -> USB CDC serial protocol
    -> Windows C++ bridge
    -> Open Rails HTTP API

Open Rails HTTP API
    -> Windows C++ bridge
    -> USB CDC serial protocol
    -> Raspberry Pi Pico 2 firmware
    -> OLED displays
```

## Status legend

- `[x]` completed and tested
- `[>]` next task
- `[ ]` planned
- `[?]` requires investigation

## Milestones

### 1. Embedded display foundations

- [x] Raspberry Pi Pico 2 project and Pico SDK build environment
- [x] Low-level I2C master implementation
- [x] Two SH1106 OLED displays on separate I2C controllers
- [x] Speed gauge rendering and animation
- [x] EVM symbol rendering and test animation

### 2. First physical input

- [x] B1K linear potentiometer connected to GP26 / ADC0
- [x] ADC measurement across the full 0–4095 range
- [x] Reusable `LinearPotentiometer` module
- [x] Rounded integer conversion to 0–100 percent
- [x] Manual console test support

### 3. First end-to-end Open Rails control

- [x] Pico serial message: `THROTTLE=<0..100>`
- [x] Windows bridge opens the Pico COM port
- [x] Windows bridge parses throttle messages
- [x] WinHTTP client sends `POST /API/CABCONTROLS`
- [x] Open Rails 1.6.1 receives the throttle value
- [x] Physical throttle controls the in-game throttle

### 4. Refactor the Windows bridge

- [>] Move serial communication from `main.cpp` into `SerialPort`
- [ ] Move line parsing into `MessageParser`
- [ ] Move Open Rails HTTP communication into `OpenRailsClient`
- [ ] Keep `main.cpp` responsible only for orchestration
- [ ] Preserve the currently working throttle path after the refactor

Target structure:

```text
windows_bridge/
├── CMakeLists.txt
├── include/
│   ├── message_parser.hpp
│   ├── openrails_client.hpp
│   └── serial_port.hpp
└── src/
    ├── main.cpp
    ├── message_parser.cpp
    ├── openrails_client.cpp
    └── serial_port.cpp
```

### 5. Stabilize analog input

- [ ] Add a one-percent change threshold or hysteresis
- [ ] Prevent ADC noise from producing unnecessary HTTP requests
- [ ] Send a periodic state refresh even when the value does not change
- [ ] Define behaviour after reconnecting the Pico or restarting Open Rails

### 6. Reverse data path: real speed

- [ ] Add HTTP GET support to `OpenRailsClient`
- [ ] Read real train speed from Open Rails
- [ ] Convert the received speed to an integer km/h value
- [ ] Send `SPEED=<km/h>` from Windows to the Pico
- [ ] Add non-blocking serial reception to the Pico firmware
- [ ] Parse and validate the `SPEED` message on the Pico

Planned direction:

```text
Open Rails speed
    -> HTTP GET
    -> Windows bridge
    -> SPEED=<km/h>
    -> Pico
    -> speed OLED
```

### 7. Replace the speed test animation

- [ ] Remove the automatic 0–160–0 speed animation from normal mode
- [ ] Keep the animation available as a manual display test
- [ ] Store the latest received speed in controller state
- [ ] Render the real Open Rails speed on the speed gauge
- [ ] Define a safe display state when communication is lost

### 8. EVM integration

- [?] Inspect data returned by `/API/TRAININFO`
- [?] Inspect data returned by `/API/TRACKMONITORDISPLAY`
- [?] Determine whether signal aspect, allowed speed and TCS state are sufficient for EVM
- [?] Determine whether the selected locomotive and route provide Hungarian EVM information
- [ ] Define a documented mapping from Open Rails data to `EvmSignal`
- [ ] Send `EVM=<state>` from Windows to the Pico
- [ ] Drive the EVM OLED from the received state

Important: the Open Rails public web API exposes train speed, allowed speed and track/signal information, but no dedicated Hungarian EVM state has been confirmed. EVM output must not be guessed from speed alone.

### 9. Remaining analog controls

- [ ] Locomotive brake potentiometer
- [ ] Train/emergency brake potentiometer
- [ ] Reuse calibration, percentage conversion and filtering
- [ ] Extend the serial protocol and Open Rails control mapping

### 10. Buttons and switches

- [ ] Reusable digital input module
- [ ] Debouncing
- [ ] Momentary buttons: vigilance, sander, horn, emergency brake
- [ ] Two-position switches: wipers, interior light, pantographs
- [ ] Three-position switches: headlights, reverser
- [ ] Optional additional buttons
- [ ] Edge-triggered messages for momentary controls
- [ ] State messages for maintained switches

### 11. Reliability and configuration

- [ ] Serial reconnect support
- [ ] Open Rails API reconnect support
- [ ] COM-port configuration outside the source code
- [ ] Open Rails host and port configuration
- [ ] Communication timeout and failsafe behaviour
- [ ] Protocol error reporting
- [ ] Startup state synchronization

### 12. Final integration

- [ ] Full input/output test mode
- [ ] Normal operating mode
- [ ] Wiring and pin-assignment documentation
- [ ] Controller protocol documentation
- [ ] Physical prototype enclosure and panel assembly
- [ ] End-to-end test with multiple locomotives and routes

## Serial protocol — current draft

Messages are newline-terminated ASCII text.

Pico to Windows:

```text
THROTTLE=42
```

Planned Windows to Pico:

```text
SPEED=83
EVM=SPEED_80
```

Rules to formalize later:

- one message per line
- uppercase field names
- integer values where practical
- invalid or unknown messages are ignored and logged
- every message ends with `\n`

## Next task

Refactor the Windows bridge into `SerialPort`, `MessageParser` and `OpenRailsClient` without changing its current behaviour.

## Update rule

After every completed milestone:

1. change its status to `[x]`;
2. move `[>]` to the next concrete task;
3. record any protocol or architecture decision made during implementation;
4. commit the roadmap together with the related code changes.
