PROJECT_NAME     := EPD-nRF51
TARGETS          := nrf51822_xxaa
OUTPUT_DIRECTORY := _build

PROJ_DIR := $(CURDIR)
SDK_ROOT := $(PROJ_DIR)
SD_PATH  := $(SDK_ROOT)/components/softdevice/s130

$(OUTPUT_DIRECTORY)/nrf51822_xxaa.out: \
  LINKER_SCRIPT  := $(SD_PATH)/toolchain/armgcc/armgcc_s130_nrf51822_xxaa.ld

# Source files common to all targets
SRC_FILES += \
  $(SDK_ROOT)/components/libraries/log/src/nrf_log_backend_serial.c \
  $(SDK_ROOT)/components/libraries/log/src/nrf_log_frontend.c \
  $(SDK_ROOT)/components/libraries/util/app_error.c \
  $(SDK_ROOT)/components/libraries/util/app_error_weak.c \
  $(SDK_ROOT)/components/libraries/timer/app_timer.c \
  $(SDK_ROOT)/components/libraries/scheduler/app_scheduler.c \
  $(SDK_ROOT)/components/libraries/util/app_util_platform.c \
  $(SDK_ROOT)/components/libraries/fstorage/fstorage.c \
  $(SDK_ROOT)/components/drivers_nrf/common/nrf_drv_common.c \
  $(SDK_ROOT)/components/drivers_nrf/clock/nrf_drv_clock.c \
  $(SDK_ROOT)/components/drivers_nrf/gpiote/nrf_drv_gpiote.c \
  $(SDK_ROOT)/components/drivers_nrf/spi_master/nrf_drv_spi.c \
  $(SDK_ROOT)/components/drivers_ext/segger_rtt/RTT_Syscalls_GCC.c \
  $(SDK_ROOT)/components/drivers_ext/segger_rtt/SEGGER_RTT.c \
  $(SDK_ROOT)/components/drivers_ext/segger_rtt/SEGGER_RTT_printf.c \
  $(SDK_ROOT)/components/ble/common/ble_advdata.c \
  $(SDK_ROOT)/components/ble/ble_advertising/ble_advertising.c \
  $(SDK_ROOT)/components/ble/common/ble_conn_params.c \
  $(SDK_ROOT)/components/ble/common/ble_srv_common.c \
  $(SDK_ROOT)/components/toolchain/gcc/gcc_startup_nrf51.s \
  $(SDK_ROOT)/components/toolchain/system_nrf51.c \
  $(SDK_ROOT)/components/softdevice/common/softdevice_handler/softdevice_handler.c \
  $(PROJ_DIR)/main.c \
  $(PROJ_DIR)/EPD/EPD_4in2.c \
  $(PROJ_DIR)/EPD/EPD_4in2_V2.c \
  $(PROJ_DIR)/EPD/EPD_4in2b_V2.c \
  $(PROJ_DIR)/EPD/EPD_driver.c \
  $(PROJ_DIR)/EPD/EPD_ble.c \
  $(PROJ_DIR)/GUI/Calendar.c \
  $(PROJ_DIR)/GUI/Lunar.c \
  $(PROJ_DIR)/GUI/fonts.c \
  $(PROJ_DIR)/GUI/Adafruit_GFX.c \
  $(PROJ_DIR)/GUI/u8g2_font.c

# Include folders common to all targets
INC_FOLDERS += \
  $(PROJ_DIR)/config \
  $(PROJ_DIR)/EPD \
  $(PROJ_DIR)/GUI \
  $(SDK_ROOT)/components/toolchain \
  $(SDK_ROOT)/components/drivers_nrf/clock \
  $(SDK_ROOT)/components/drivers_nrf/hal \
  $(SDK_ROOT)/components/drivers_nrf/common \
  $(SDK_ROOT)/components/drivers_nrf/delay \
  $(SDK_ROOT)/components/drivers_nrf/gpiote \
  $(SDK_ROOT)/components/drivers_nrf/spi_master \
  $(SDK_ROOT)/components/drivers_ext/segger_rtt \
  $(SDK_ROOT)/components/libraries/fstorage \
  $(SDK_ROOT)/components/libraries/experimental_section_vars \
  $(SDK_ROOT)/components/libraries/log \
  $(SDK_ROOT)/components/libraries/log/src \
  $(SDK_ROOT)/components/libraries/timer \
  $(SDK_ROOT)/components/libraries/scheduler \
  $(SDK_ROOT)/components/libraries/util \
  $(SDK_ROOT)/components/device \
  $(SDK_ROOT)/components/toolchain \
  $(SDK_ROOT)/components/toolchain/cmsis/include \
  $(SDK_ROOT)/components/toolchain/gcc \
  $(SDK_ROOT)/components/ble/common \
  $(SDK_ROOT)/components/ble/ble_advertising \
  $(SDK_ROOT)/components/softdevice/common/softdevice_handler \
  $(SDK_ROOT)/components/softdevice/s130/headers \
  $(SDK_ROOT)/components/softdevice/s130/headers/nrf51

# Libraries common to all targets
LIB_FILES += \

# C flags common to all targets
CFLAGS += -DSOFTDEVICE_PRESENT
CFLAGS += -DNRF51
CFLAGS += -DS130
CFLAGS += -DBLE_STACK_SUPPORT_REQD
CFLAGS += -DSWI_DISABLE0
CFLAGS += -DNRF51822
CFLAGS += -DNRF_SD_BLE_API_VERSION=2
CFLAGS += -mcpu=cortex-m0
CFLAGS += -mthumb -mabi=aapcs
CFLAGS +=  -Wall -Werror -O3 -g3
CFLAGS += -mfloat-abi=soft
# keep every function in separate section, this allows linker to discard unused ones
CFLAGS += -ffunction-sections -fdata-sections -fno-strict-aliasing
CFLAGS += -fno-builtin --short-enums 

# C++ flags common to all targets
CXXFLAGS += \

# Assembler flags common to all targets
ASMFLAGS += -x assembler-with-cpp
ASMFLAGS += -DSOFTDEVICE_PRESENT
ASMFLAGS += -DNRF51
ASMFLAGS += -DS130
ASMFLAGS += -DBLE_STACK_SUPPORT_REQD
ASMFLAGS += -DSWI_DISABLE0
ASMFLAGS += -DNRF51822
ASMFLAGS += -DNRF_SD_BLE_API_VERSION=2

# Linker flags
LDFLAGS += -mthumb -mabi=aapcs -L $(TEMPLATE_PATH) -T$(LINKER_SCRIPT)
LDFLAGS += -mcpu=cortex-m0
# let linker to dump unused sections
LDFLAGS += -Wl,--gc-sections
# use newlib in nano version
LDFLAGS += --specs=nano.specs -lc -lnosys


.PHONY: $(TARGETS) default all clean help flash flash_softdevice

# Default target - first one defined
default: nrf51822_xxaa

# Print all targets that can be built
help:
	@echo following targets are available:
	@echo 	nrf51822_xxaa

TEMPLATE_PATH := $(SDK_ROOT)/components/toolchain/gcc

include $(TEMPLATE_PATH)/Makefile.common

$(foreach target, $(TARGETS), $(call define_target, $(target)))

# Flash the program
flash: $(OUTPUT_DIRECTORY)/nrf51822_xxaa.hex
	@echo Flashing: $<
	nrfjprog --program $< -f nrf51 --sectorerase
	nrfjprog --reset -f nrf51

# Flash softdevice
flash_softdevice:
	@echo Flashing: s130_nrf51_2.0.1_softdevice.hex
	nrfjprog --program $(SD_PATH)/hex/s130_nrf51_2.0.1_softdevice.hex -f nrf51 --sectorerase 
	nrfjprog --reset -f nrf51

erase:
	nrfjprog --eraseall -f nrf51
