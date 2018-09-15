cmake_minimum_required(VERSION 2.8.4)
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/cmake/ArduinoToolchain.cmake)
set(PROJECT_NAME RGBATester)
set(ARDUINO_EXTRA_LIBRARIES_PATH "/Users/yona/Documents/Arduino/libraries")
project(${PROJECT_NAME})

# register_hardware_platform("/Users/yona/.arduino15/packages/adafruit/hardware/avr/1.4.9")


set(${CMAKE_PROJECT_NAME}_BOARD nano)
set(${CMAKE_PROJECT_NAME}_PORT /dev/tty.wchusbserial1460)

set(${CMAKE_PROJECT_NAME}_SKETCH GlowClothingRadio.ino)

# print_board_list()

#### Uncomment below additional settings as needed.
# set(${CMAKE_PROJECT_NAME}_BOARD mega)
# set(${CMAKE_PROJECT_NAME}_PORT /dev/ttyACM0)
# set(mega.build.mcu atmega2560)
# set(mega.upload.protocol wiring)
# set(mega.upload.speed 115200)

generate_arduino_firmware(${CMAKE_PROJECT_NAME})

