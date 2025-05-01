# JAM Byte Code Player for Embedded Systems

This repository provides tools for programming Altera CPLDs and FPGAs using the JAM Byte Code Player. It includes the following components:

## Components

1. **Original JAM Byte Code Player v2.3.2**  
   The latest unmodified version of the JAM Byte Code Player, as provided by Intel. It supports programming Altera devices via JTAG using JBC (`.jbc`) files.

2. **Ported and Slimmed-Down Version**  
   A lightweight version of the JAM Byte Code Player, tailored for embedded systems. This version is designed to function as a library for microcontrollers such as STM32 or similar MCUs, enabling programming of Altera CPLDs and FPGAs in resource-constrained environments.

## Key Features

- **JTAG Programming**: Supports programming Altera devices using JBC (`.jbc`) files.
- **Embedded System Optimization**: Designed for use in embedded systems with limited resources.
- **Customizable Library**: The slimmed-down version can be easily integrated into various embedded platforms.
- **In-Application Programming (IAP)**: Enables dynamic loading and execution of JBC files for in-application programming.

## Use Cases

This repository is ideal for developers working on embedded systems who require a reliable and efficient solution for programming Altera devices. It is particularly suited for applications where:

- Resource constraints demand a lightweight programming library.
- In-application programming (IAP) is needed for dynamic firmware updates.
