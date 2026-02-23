# 📡 STM8 IR Transmitter & Receiver

## Overview

This project implements a simple **Infrared (IR) communication system** using two **STM8S103F3P** microcontrollers:

- 📤 **IR_Transmitter**
- 📥 **IR_Receiver**

The transmitter sends **one byte of data** when a button is pressed.  
The receiver captures and decodes the IR signal and transmits the decoded byte over **UART** to a serial monitor.

Both transmitter and receiver must be flashed onto **two separate STM8S103F3P devices**.

---

## 🧠 How It Works

### Transmitter Side

- Uses **TIM2** to generate a ~38 kHz PWM carrier.
- Uses **TIM4** to generate microsecond delays.
- Button press triggers EXTI interrupt.
- A full IR frame is transmitted.

### Receiver Side

- Uses **TIM2** as a ~1 MHz free-running counter.
- EXTI captures rising/falling edges from IR demodulator.
- Pulse widths are stored in a buffer.
- Frame is decoded.
- Decoded byte is sent via **UART1 (9600 baud)**.
- LED indicates successful reception.

---

### Notes

- `infrared.c` and `infrared.h` are shared between TX and RX. (present separately in both folders)
- Standard Peripheral Library files are shared. (present separately in both folders)
- Each folder is compiled separately and flashed to different boards.

---

## 📡 Transmission Method

The protocol is **similar to NEC**, but simplified.

### Frame Structure

HEADER → DATA (1 byte) → END MARK

### Timing Parameters

| Signal Part   | Duration |
|---------------|----------|
| Header Mark   | 9000 µs  |
| Header Space  | 4500 µs  |
| Bit Mark      | 560 µs   |
| Logic 0 space | 560 µs   |
| Logic 1 space | 1690 µs  |
| End Mark      | 560 µs   |

### Encoding

- Data sent **LSB first**
- Each bit:
  - 560 µs carrier ON
  - Followed by short or long OFF duration
- Short space → Logic 0  
- Long space → Logic 1  

---

## 🔄 Decoding Process

Receiver:

1. Validates header timing (~9ms + ~4.5ms)
2. Measures mark/space pulse widths
3. Determines each bit based on space duration
4. Reconstructs 8-bit frame
5. Sends decoded byte via UART

---

## 🛠 Hardware Required

- 2 × STM8S103F3P
- IR LED (TX)
- IR Demodulator (e.g., VS1838B)
- Push Button
- LED (RX status)
- UART-USB Converter

---

## 🚀 How To Use

1. Flash `IR_Transmitter` to first STM8.
2. Flash `IR_Receiver` to second STM8.
3. Connect receiver UART to PC (9600 baud).
4. Power both boards.
5. Press transmitter button.
6. Observe decoded byte on serial monitor.

---

## 🖼 Connection Overview

Transmitter:

STM8 PD4 ── Resistor ──► IR LED ──► GND  
STM8 PC3 ── Button ──► GND  

Receiver:

IR Module OUT ──► STM8 PA2  
STM8 PD5 ──► USB-UART RX  
STM8 PD3 ──► LED ──► GND  

---

## ⚠ Important

- **Both boards must share a common ground** (especially if powered separately).
- Use a proper resistor for IR LED to avoid damage.
- Ensure correct UART voltage levels (3.3V vs 5V).

---

## ⚠ Limitations

- Only 1 byte per frame
- No checksum or CRC
- Basic timing validation
- No repeat-frame handling

---

## 🔮 Possible Improvements

- Add multi-byte transmission
- Implement full NEC protocol
- Add checksum/CRC
- Improve timing tolerance
- Add repeat code support

---

## 👨‍💻 Author

**Mehul Saini**  
Embedded Systems | STM8 | IR Communication