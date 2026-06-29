# Unified Digital Clock, Calendar, & Timer Suite
### Dual-Paradigm Implementation: Embedded C++ (Hardware) & Synthesizable Verilog (RTL)

Welcome to this comprehensive digital clock and timekeeping project! This repository showcases a complete end-to-end journey in digital systems engineering. It solves a complex multi-mode timing problem using two completely separate design methodologies:

1. **Embedded Microcontroller Framework:** Microcontroller firmware utilizing cooperative software timing, switch debouncing, and bus-multiplexed hardware driving physical 7-segment displays.
2. **Synthesizable Digital Logic (RTL):** Register-Transfer Level hardware description logic utilizing modular counters, custom clock dividers, and explicit Finite State Machines (FSM) to build a standalone silicon-ready time tracking chip.

---

## Tech Stack & Tools
* **Embedded System:** C++, Arduino IDE, Proteus/Hardware Schematics
* **RTL Design & Verification:** Verilog (IEEE 1364-2005), Icarus Verilog (`iverilog`), VVP Runtime Engine
* **Waveform Analysis:** GTKWave (VCD - Value Change Dump verification)
* **Discrete Electronics:** SN7447 BCD-to-7-Segment Decoder, Common Anode Displays, Tactile Push-Buttons

---

## Features Matrix

Both implementations are engineered to handle multi-modal operations seamlessly:

* **Timekeeping Engine:** Standard mod-60 arithmetic for seconds/minutes and mod-24/12 formats for hour accumulation.
* **Smart Calendar Module:** Dynamic date tracking with explicit logic to handle varied month lengths and **leap-year compensation** rules.
* **Configurable Alarm:** Compares true time arrays against a programmable register array to trigger an alarm buzzer for a specific duration.
* **Stopwatch Mode:** High-precision tracking down to hundredths of a second (centiseconds) featuring independent Start, Pause, and Reset states.
* **Countdown Timer:** Programmable down-counter with auto-expiration notification (buzzer control).

---

## Implementation Paradigms

### 1. The Physical Hardware & Microcontroller Layer (`Arduino / C++`)
Faced with the constraint of driving six individual 7-segment displays (42 distinct LED lines) using a pin-limited Arduino, the system employs a tightly coordinated hardware/software co-design:

* **BCD Bus Compaction:** A 4-bit Binary Coded Decimal (BCD) bus is routed from the Arduino to an external **SN7447 Decoder IC**. This chips converts the BCD values into standard 7-segment active-low outputs, reducing the required data pins from 7 to 4 per digit.
* **Time-Division Multiplexing:** The displays share the exact same 4-bit data lines. The Arduino rapidly cycles power to the common anodes of the displays one-by-one at a sub-millisecond refresh rate. This relies on the human **Persistence of Vision (PoV)** to give the visual illusion of an uninterrupted 6-digit display while optimizing power and pin allocation.
* **Input Conditioning:** Integrated software debouncing routines eliminate physical bounce-noise when switching modes or advancing time configurations via tactile pushbuttons.

### 2. The Register-Transfer Level Layer (`Verilog RTL`)
To transition the problem from a programmable microcontroller domain into standalone silicon hardware, the exact same state tracking machine was architected inside `clock.v` from scratch:

* **Hierarchical Clock Dividers:** Takes a high-frequency system clock (e.g., 50MHz) and establishes highly stable `tick_1hz` and `tick_100hz` enable lines using modulo counter networks.
* **Modular FSM Controller:** Implements a strict `menu_select` state engine using conditional `case` blocks. This isolates configurations so parameters can be updated in real-time (Clock, Alarm, Calendar, Stopwatch) without stopping the background timekeeper.
* **Behavioral Realignment:** Simulates hardware processes concurrently, eliminating sequential processor lag and implementing true parallel clock gating.

---

## Simulation & Verification Flow

The Verilog RTL implementation has been strictly validated via standard digital verification practices:

1. **The Testbench (`tb_clock.v`):** Synthesizes input stimulus patterns, mocks system clock pulses, overrides default parameters for accelerated simulation scaling, and rigorously evaluates corner cases (like the rollout from Feb 28th to Mar 1st on leap vs. non-leap years).
2. **Compilation & Execution (`iverilog` & `vvp`):** ```bash
   iverilog -o output.vvp clock.v tb_clock.v
   vvp output.vvp
3. **Waveform Debugging (`gtk.vcd`):** Generates a structural Value Change Dump file during execution. This file maps every flip-flop transition, bus change, and gate trigger, which is viewed inside **GTKWave** to prove 100% logical correctness and positive timing slack.

---
*Developed as part of the courses EE1003 - Scientific Programming for EE and EE1501 - Digital Systems Lab.*
