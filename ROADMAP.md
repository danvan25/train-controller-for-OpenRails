# Train Controller for Open Rails — Roadmap

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

## Milestone 0 — Development environment

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

## Milestone 1 — GPIO subsystem

Status: **Planned**

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

## Milestone 2 — Analog controls

Status: **Planned**

- [ ] Study the RP2350 ADC registers and conversion process.
- [ ] Create a register-level ADC driver.
- [ ] Read the traction controller slide potentiometer.
- [ ] Read the locomotive brake slide potentiometer.
- [ ] Read the rapid-brake slide potentiometer.
- [ ] Determine and store minimum and maximum calibration values.
- [ ] Normalize raw ADC readings to controller values.
- [ ] Implement noise filtering and averaging.
- [ ] Implement configurable dead zones.
- [ ] Detect disconnected or invalid analog controls where possible.
- [ ] Create an analog-input diagnostic mode.

## Milestone 3 — I2C subsystem

Status: **Planned**

- [ ] Study the RP2350 I2C controller registers.
- [ ] Create a register-level I2C master driver.
- [ ] Configure the I2C clock frequency.
- [ ] Implement START and STOP handling.
- [ ] Implement 7-bit slave addressing.
- [ ] Implement command and data transmission.
- [ ] Handle ACK and NACK responses.
- [ ] Add timeout and bus-error handling.
- [ ] Add bus recovery for a stuck peripheral where practical.
- [ ] Support both RP2350 hardware I2C controllers.
- [ ] Verify communication with an I2C address scan.

## Milestone 4 — SH1106 OLED displays

Status: **Planned**

- [ ] Study the SH1106 command set and display memory organization.
- [ ] Create a custom SH1106 driver without Arduino display libraries.
- [ ] Initialize one 128×64 SH1106 OLED display.
- [ ] Send commands and display data over the custom I2C driver.
- [ ] Implement a 128×64 monochrome framebuffer.
- [ ] Account for the SH1106 controller's 132-column display RAM.
- [ ] Implement pixel, line, rectangle, and progress-bar drawing.
- [ ] Create or integrate a project-owned bitmap font format.
- [ ] Render text and numeric values.
- [ ] Operate two displays on separate I2C controllers.
- [ ] Design a display diagnostics screen.
- [ ] Measure and optimize display refresh time.

## Milestone 5 — Controller firmware architecture

Status: **Planned**

- [ ] Define a unified state model for every physical control.
- [ ] Create a deterministic main update loop.
- [ ] Schedule digital input scanning and ADC sampling.
- [ ] Schedule display updates independently from input sampling.
- [ ] Separate hardware drivers from application logic.
- [ ] Add startup self-tests.
- [ ] Define firmware error codes and diagnostic states.
- [ ] Add a calibration workflow.
- [ ] Store persistent configuration and calibration data.
- [ ] Add firmware version information.
- [ ] Document timing, memory, and CPU usage.

## Milestone 6 — USB communication

Status: **Planned**

- [ ] Select the final USB device architecture.
- [ ] Integrate the supported USB stack from the Pico SDK ecosystem.
- [ ] Create project-specific USB descriptors.
- [ ] Expose buttons as HID controls where appropriate.
- [ ] Expose analog levers as HID axes where appropriate.
- [ ] Evaluate a composite HID and bidirectional data device.
- [ ] Define a versioned application-level packet protocol.
- [ ] Transfer controller state from the Pico 2 to the PC.
- [ ] Transfer simulator telemetry from the PC to the Pico 2.
- [ ] Add message validation and connection-state handling.
- [ ] Handle USB disconnects and reconnects safely.
- [ ] Document the USB protocol.

## Milestone 7 — Open Rails integration

Status: **Research required**

- [ ] Identify the Open Rails input interfaces relevant to the controller.
- [ ] Map traction, braking, direction, and auxiliary controls.
- [ ] Test standard USB HID joystick compatibility.
- [ ] Test keyboard-style HID mappings where useful.
- [ ] Investigate available speed telemetry.
- [ ] Investigate available EVM or train-protection telemetry.
- [ ] Identify other simulator values suitable for the OLED displays.
- [ ] Determine whether a PC-side companion application is required.
- [ ] Implement the minimum PC-side bridge if required.
- [ ] Test bidirectional communication during a simulator session.
- [ ] Document supported locomotives and known limitations.

## Milestone 8 — Physical controls

Status: **Planned**

### Analog controls

- [ ] Traction controller — slide potentiometer.
- [ ] Locomotive brake — slide potentiometer.
- [ ] Rapid brake — slide potentiometer.

### Buttons and switches

- [ ] Vigilance control — momentary push button.
- [ ] Windshield wiper — two-position switch.
- [ ] Headlights — three-position switch.
- [ ] Sander — momentary push button.
- [ ] Horn — momentary push button.
- [ ] Reverser — three-position switch.
- [ ] Cab interior light — two-position switch.
- [ ] Pantograph 1 — two-position switch.
- [ ] Pantograph 2 — two-position switch.
- [ ] Emergency brake — push button.
- [ ] Five configurable auxiliary buttons.

### Displayed information

- [ ] Traction controller value.
- [ ] Current speed.
- [ ] EVM status or indication.
- [ ] Brake values or states.
- [ ] USB and simulator connection status.
- [ ] Calibration and diagnostics information.

## Milestone 9 — Hardware design

Status: **Planned**

- [ ] Create the final Pico 2 pin assignment.
- [ ] Verify the GPIO and ADC resource budget.
- [ ] Create a complete schematic.
- [ ] Select suitable linear slide potentiometers.
- [ ] Select mechanically appropriate buttons and switches.
- [ ] Design 3.3 V-safe analog input circuits.
- [ ] Add power filtering and decoupling.
- [ ] Add practical input protection.
- [ ] Define internal connectors and cable harnesses.
- [ ] Build and test a breadboard prototype.
- [ ] Evaluate a custom PCB.
- [ ] Design the enclosure and front panel.
- [ ] Add labels and serviceable internal wiring.
- [ ] Produce a bill of materials.

## Milestone 10 — Integration, testing, and release

Status: **Planned**

- [ ] Integrate every input, both displays, and USB communication.
- [ ] Test every control independently.
- [ ] Test simultaneous control changes.
- [ ] Test long-term firmware stability.
- [ ] Test USB disconnect and reconnect scenarios.
- [ ] Test noisy and boundary ADC values.
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
- Multiple locomotive profiles.
- User-configurable button mappings.
- Firmware update support without BOOTSEL interaction.
- Automated firmware builds with GitHub Actions.
- A custom PCB and professionally manufactured enclosure.

---

The immediate next step is **Milestone 1: GPIO subsystem**, beginning with a
small reusable register-level GPIO driver and a debounced push-button test.
