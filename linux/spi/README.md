# SPI for LMK&LMX
This driver is used to configure the LMK02828 and LMX2594 clock chips.
Based on the parameters of the `setclk` application, it configures the clock chips to use either the onboard crystal oscillator or an external input as the reference source.
The output frequencies of the clock chips match the default settings in the RFSoC4x2 reference manual, providing a 491.52 MHz input clock to both the ADC and DAC.