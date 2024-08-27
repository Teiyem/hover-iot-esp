# Hover Esp
Hover Esp is built using the ESP-IDF framework for ESP32 devices that is designed to be used with Hover.

[![CI Build Hover Esp](https://github.com/Teiyem/hover-iot-esp/actions/workflows/hover-iot-esp-ci.yml/badge.svg)](https://github.com/Teiyem/hover-iot-esp/actions/workflows/hover-iot-esp-ci.yml)

## Features
* Http/s server. ⚠️
* Wi-Fi Provisioning using AP Schema. ✅
* Over-the-air updates. ⚠️
* Mqtt
* Encryption. ✅

## Development
* Under certs are the development certificates
  esptool.py write_flash 0x340000 factory.bin

### Generating Protocol Buffers Files
This project uses Protocol Buffers to define data structures, and the following steps outline how to generate C files from the .proto files using CMake and Make.

#### Prerequisites
* **Protocol Buffers Compiler (protoc-c):** 
  * Install the Protocol Buffers Compiler by following the instructions at [Protocol Buffers - Installation](https://developers.google.com/protocol-buffers/docs/downloads).
* **CMake**

### Build Steps
1. **Configure the Project with CMake:**
   ```bash
   cmake ..
   ```
   This step sets up the build configuration based on the `CMakeLists.txt` file in the component root dir i.e components/iot_ota/CMakeLists.txt.

2. **Build the Project with Make:**
   ```bash
   make
   ```
   This command will generate the c ProtoBuf files and removes the CMake-generated files. If you want to regenerate the c ProtoBuf files, you need to run step 1 again.

   <br>If you want to clean up the generated files, you can use:
   ```bash
   make clean

### Additional Information
**Generated Files:**
    - The generated `.c` and `.h` files will be placed in the `proto-c` directory.


## Usage
Hover Esp is designed to provide a highly customizable solution for building IoT devices that can be controlled remotely through home automation systems.
$ $IDF_PATH/components/nvs_flash/nvs_partition_generator/nvs_partition_gen.py generate factory.csv factory.bin 0x6000

## Authors
Hover Esp was developed by Thabang Mmakgatla.

## Acknowledgments
* Special thanks to the developers of the ESP-IDF framework.
* Special thanks to [Ruslan V. Uss](https://github.com/UncleRus) for providing the esp-idf-lib.

## License
Hover Esp is licensed under the MIT license. See the LICENSE file for details.
