# Clock Configuration Example for RFSoC4x2

This repository provides an example of configuring the clock chips **LMX04828** and **LMK2594** on the RFSoC4x2 board using **SPI** in bare-metal and Linux OS, so that user can use the board without PYNQ.

The compatible Linux drivers and applications code can be found in the `linux` folder. Please note that cross-compilation is required here.

> **Note**  
> Please use the official reg values rather than generating your own in **TICS Pro**. Unless you are highly familiar with the software, even minor configuration differences can lead to unexpected errors.

The hexadecimal configuration values used for the clock chips can be generated from **TICS Pro**, or use official PYNQ clock configuration files:
- `LMK04828_245.76.txt`
- `LMX2594_291.52.txt`

These files are available in the PYNQ RFSoC4x2 repository:  
<https://github.com/Xilinx/RFSoC-PYNQ/tree/master/boards/RFSoC4x2/packages/tics/tics/register_txts>

---

## Bare-metal Getting Started

### 1. Build the Vivado Project

1. Launch **Vivado 2025.1**.
2. Source the provided TCL script:
   ```tcl
   source ./hardware/design_1.tcl
   ```
3. Generate the bitstream.
4. Export the hardware design.

### 2. Build the Vitis Project
1. Launch **Vitis 2025.1** and set your workspace.

2. Create a **platform component** from the ``.xdc`` file provided in the ``hw_description`` folder
(or use your own design exported from Vivado).

3. Create a new **application component**.

4. Import the source files from the ``sourceFile`` directory.

5. Build and run the application.

The bare-metal driver builds with both Vitis flows: the SDT flow (Vitis 2023.2 and later, `SDT` defined) looks up the SPI/GPIO drivers by base address, the classic flow (Vitis 2020.2 to 2023.1) by device ID. Define `CLK_DEBUG` to print the SPI read-back of every register.

It has been used unchanged on the board with Vitis 2020.2: after `write_clk(0)`, `write_clk(1)`, `write_clk(2)` the DAC tile 228 and ADC tile 226 PLLs lock at 3.93216 GSPS from the 491.52 MHz reference (see [rfsoc_ofdm / RFSoC4x2 ofdm_video](https://github.com/uceeyuf/rfsoc_ofdm/tree/rfsoc4x2-ofdm-video/boards/RFSoC4x2/ofdm_video)).
