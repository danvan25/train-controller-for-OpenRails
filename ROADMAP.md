# Train Controller for Open Rails â Roadmap

This roadmap tracks the development of a physical train controller for
[Open Rails](https://www.openrails.org/). The project combines register-level
embedded development on a Raspberry Pi Pico 2, custom electronics, physical
controls, OLED displays, USB communication, and PC-side simulator integration.

The roadmap is expected to evolve as the supported Open Rails interfaces and
the final control-panel design become clearer.

## Project principles

- Use C++ and the official Raspberry Pi Pico SDK without the Arduino framework.
- Implement the GPIO, ADC, I2C, SH1106, graphics, and input layers in the project.
- Work at register level where it provides meaningful embedded-systems experience.
- Use a proven USB stack while designing the device descriptors and application protocol.
- Keep every milestone buildable, testable, documented, and suitable for a Git commit.
- Separate firmware, desktop integration, hardware design, and documentation.

## Milestone 0 â Development environment

Status: **Completed**

- [x] Create the GitHub repository.
- [x] Install Visual Studio Code and the official Raspberry Pi Pico extension.
- [x] Configure Pico SDK 2.3.0.
- [x] Configure the ARM GCC, CMake, and Ninja toolchain.
- [x] Create a C++ firmware project for Raspberry Pi Pico 2.
- [x] Target the Arm Cortex-M33 cores of the RP2350.
- [x] Compile the initial firmware successfully.
- [x] Upload firmware through USB with picotool.
- [x] Implement and test register-level control of the onboard LED.
- [x] Handle the RP2350 GPIO pad isolation during initialization.

## Milestone 1 â GPIO subsystem

Status: **Planned â onboard status LED completed**

- [ ] Study the RP2350 SIO, IO Bank 0, and Pads Bank 0 registers.
- [ ] Create a reusable register-level GPIO driver.
- [ ] Configure GPIO pins as inputs and outputs.
- [ ] Configure internal pull-up resistors.
- [ ] Read momentary push buttons.
- [ ] Read two-position switches.
- [ ] Read three-position switches with two GPIO inputs.
- [ ] Implement software debouncing.
- [ ] Detect state changes and generate input events.
- [ ] Detect invalid three-position switch states.
- [ ] Create a GPIO diagnostic test program.

## Milestone 2 â Analog controls

Status: **In progress â three analog controls operational**

- [ ] Study the RP2350 ADC registers and conversion process.
- [ ] Create a register-level ADC driver.
- [x] Read the traction controller slide potentiometer on GP26/ADC0.
- [x] Read the train-brake slide potentiometer on GP27/ADC1.
- [x] Read the profile-dependent secondary-brake slide potentiometer on GP28/ADC2.
- [ ] Determine and store minimum and maximum calibration values.
- [x] Normalize 12-bit ADC readings to integer percentages from 0 to 100.
- [x] Discard the first ADC conversion after channel switching.
- [x] Implement four-sample averaging.
- [x] Implement two-percent change deadbands.
- [ ] Detect disconnected or invalid analog controls where possible.
- [x] Create USB serial output for analog-input diagnostics.

## Milestone 3 â I2C subsystem

Status: **Operational â bus recovery remains planned**

- [x] Study the RP2350 I2C controller registers.
- [x] Create a register-level I2C master driver.
- [x] Configure the I2C clock frequency.
- [x] Implement START and STOP handling through the RP2350 I2C controller.
- [x] Implement 7-bit slave addressing.
- [x] Implement command and data transmission.
- [x] Handle ACK and NACK responses.
- [x] Add timeout and bus-error handling.
- [ ] Add bus recovery for a stuck peripheral where practical.
- [x] Support both RP2350 hardware I2C controllers.
- [x] Enable internal pull-ups and RP2350 input paths for I2C pins.
- [x] Validate alternative I2C0 pin assignments.
- [x] Create a dual-bus I2C diagnostic firmware target.

## Milestone 4 â SH1106 OLED displays

Status: **Completed for the current prototype**

- [x] Study the SH1106 command set and display memory organization.
- [x] Create a custom SH1106 driver without Arduino display libraries.
- [x] Initialize SH1106 OLED displays.
- [x] Send commands and display data over the custom I2C driver.
- [x] Implement a 128Ă64 monochrome framebuffer.
- [x] Account for the SH1106 controller's 132-column display RAM.
- [x] Implement the graphics primitives required by the current displays.
- [ ] Create or integrate a project-owned bitmap font format.
- [x] Render speed information and EVM graphics.
- [x] Operate two displays on separate I2C controllers.
- [x] Create display and I2C diagnostic firmware.
- [x] Raise the I2C refresh rate during initial display optimization.
- [x] Operate both OLEDs from an external 3.3 V supply with a common ground.

## Milestone 5 â Controller firmware architecture

Status: **In progress**

- [ ] Define a unified state model for every physical control.
- [x] Create a deterministic non-blocking main update loop.
- [x] Schedule ADC sampling independently from serial processing.
- [x] Update displays only when their state changes.
- [x] Separate hardware drivers, graphics, inputs, and application logic.
- [x] Add startup checks for both I2C controllers and OLED displays.
- [x] Define LED blink patterns for initialization and display errors.
- [ ] Add a calibration workflow.
- [ ] Store persistent configuration and calibration data.
- [ ] Add firmware version information.
- [ ] Document timing, memory, and CPU usage.

## Milestone 6 â USB communication

Status: **Operational using USB CDC; final protocol design remains planned**

- [ ] Select the final USB device architecture.
- [x] Integrate Pico SDK USB CDC serial communication.
- [ ] Create project-specific USB descriptors.
- [ ] Expose buttons as HID controls where appropriate.
- [ ] Expose analog levers as HID axes where appropriate.
- [ ] Evaluate a composite HID and bidirectional data device.
- [ ] Define a versioned application-level packet protocol.
- [x] Transfer throttle, train-brake, and secondary-brake state to the PC.
- [x] Transfer speed and EVM state from the PC to the Pico 2.
- [x] Add line-based message parsing and numeric validation.
- [x] Add PC-side serial write retries and error deduplication.
- [x] Create a standalone Pico-to-PC communication diagnostic target.
- [ ] Complete automatic USB disconnect and reconnect recovery.
- [ ] Document the USB protocol.

## Milestone 7 â Open Rails integration

Status: **In progress â core bidirectional integration operational**

- [x] Identify and use the Open Rails HTTP API on localhost:2150.
- [x] Map throttle, train brake, and engine brake through CABCONTROLS.
- [ ] Test standard USB HID joystick compatibility.
- [ ] Test keyboard-style HID mappings where useful.
- [x] Read current speed from Open Rails telemetry.
- [x] Derive EVM indications from upcoming signal aspect and speed data.
- [x] Identify speed and EVM state as OLED telemetry.
- [x] Implement the Windows Bridge companion application using WinHTTP.
- [x] Test bidirectional communication during simulator sessions.
- [x] Implement two-percent soft-pickup with target-crossing detection.
- [x] Implement retry and backoff behavior for intermittent HTTP errors.
- [x] Add the built-in `conventional_locomotive` control profile.
- [x] Map `SECONDARY_BRAKE` to `ENGINE_BRAKE` in the conventional profile.
- [ ] Add FLIRT and Uzsgyi profiles mapping the secondary control to `DYNAMIC_BRAKE`.
- [ ] Move profile definitions to user-editable configuration files.
- [ ] Document supported locomotives and known limitations.

## Milestone 8 â Physical controls

Status: **In progress â analog controls and displays operational**

### Analog controls

- [x] Traction controller â 1 kÎŠ linear slide potentiometer.
- [x] Train brake â 1 kÎŠ linear slide potentiometer.
- [x] Profile-dependent secondary brake â 1 kÎŠ linear slide potentiometer.

### Buttons and switches

- [ ] Vigilance control â momentary push button.
- [ ] Windshield wiper â two-position switch.
- [ ] Headlights â three-position switch.
- [ ] Sander â momentary push button.
- [ ] Horn â momentary push button.
- [ ] Reverser â three-position switch.
- [ ] Cab interior light â two-position switch.
- [ ] Pantograph 1 â two-position switch.
- [ ] Pantograph 2 â two-position switch.
- [ ] Emergency brake â push button.
- [ ] Five configurable auxiliary buttons.

### Displayed information

- [x] Current speed.
- [x] EVM status and indication.
- [ ] Brake values or states.
- [ ] USB and simulator connection status.
- [ ] Calibration and diagnostics information.

## Milestone 9 â Hardware design

Status: **Breadboard prototype in progress**

- [ ] Create the final Pico 2 pin assignment.
- [x] Verify the ADC resource budget for three analog controls.
- [ ] Create a complete schematic.
- [x] Select and test 1 kÎŠ linear slide potentiometers.
- [ ] Select mechanically appropriate buttons and switches.
- [ ] Design 3.3 V-safe analog input circuits.
- [ ] Add power filtering and decoupling.
- [ ] Add practical input protection.
- [ ] Define internal connectors and cable harnesses.
- [x] Build and test the current breadboard prototype.
- [x] Validate external 3.3 V OLED power with a shared Pico ground.
- [ ] Evaluate a custom PCB.
- [ ] Design the enclosure and front panel.
- [ ] Add labels and serviceable internal wiring.
- [ ] Produce a bill of materials.

## Milestone 10 â Integration, testing, and release

Status: **In progress**

- [ ] Integrate every input, both displays, and USB communication.
- [x] Test all three analog controls independently.
- [x] Test simultaneous keyboard and physical-control operation.
- [ ] Test long-term firmware stability.
- [ ] Test USB disconnect and reconnect scenarios.
- [x] Test noisy and boundary ADC values and add filtering/deadbands.
- [ ] Test calibration persistence.
- [ ] Test behavior when Open Rails is not running.
- [ ] Test behavior when telemetry is unavailable.
- [ ] Measure boot time and input latency.
- [ ] Complete build and wiring documentation.
- [ ] Complete user and calibration instructions.
- [ ] Tag the first usable firmware release.

## Future possibilities

These features are outside the initial scope but may be considered later:

- Additional gauges or displays.
- Indicator LEDs with adjustable brightness.
- Audible feedback or a small speaker.
- Haptic or mechanical feedback for control positions.
- Additional locomotive profiles and user-editable profile files.
- User-configurable button mappings.
- Firmware update support without BOOTSEL interaction.
- Automated firmware builds with GitHub Actions.
- A custom PCB and professionally manufactured enclosure.

---

## Immediate next steps

1. Add and test the first reusable digital-input path with debouncing.
2. Integrate the vigilance, horn, and sander push buttons.
3. Add FLIRT and Uzsgyi profiles using the discovered `DYNAMIC_BRAKE` control.
4. Continue long-duration simulator, HTTP recovery, and USB reconnect testing.
5. Document the current Pico 2 pin assignment and external OLED power wiring.