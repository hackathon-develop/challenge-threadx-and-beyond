#  Copyright (c) Microsoft
#  Copyright (c) 2024 Eclipse Foundation
# 
#  This program and the accompanying materials are made available 
#  under the terms of the MIT license which is available at
#  https://opensource.org/license/mit.
# 
#  SPDX-License-Identifier: MIT
# 
#  Contributors: 
#     Microsoft         - Initial version
#     Frédéric Desbiens - 2024 version.

# Define ThreadX user configuration
set(TX_USER_FILE "${CMAKE_CURRENT_LIST_DIR}/threadx/tx_user.h" CACHE STRING "Enable TX user configuration")

# Define NetXDuo user configuration
set(NX_USER_FILE "${CMAKE_CURRENT_LIST_DIR}/netxduo/nx_user.h" CACHE STRING "Enable NX user configuration")
set(NXD_ENABLE_FILE_SERVERS OFF CACHE BOOL "Disable fileX dependency by netxduo")

# Define Zenoh-pico user configuration
set(WITH_THREADX ON CACHE Bool "Enable ThreadX target for Zenoh-pico")
set(BUILD_SHARED_LIBS OFF CACHE Bool "Disable shared the building of shared libraries")
set(Z_FEATURE_MULTI_THREAD 0 CACHE STRING "Toggle multithread")


# Core libraries
add_subdirectory(${SHARED_LIB_DIR}/threadx threadx)
add_subdirectory(${SHARED_LIB_DIR}/netxduo netxduo)
add_subdirectory(${SHARED_LIB_DIR}/jsmn jsmn)


# This will get the include directories for ThreadX and NetXDuo, and will propagate them to the
# global include path before compiling zenoh-pico. Thus, zenoh-pico can see the tx_api.h and
# nx_api.h.
get_target_property(THREADX_INCLUDES azrtos::threadx INCLUDE_DIRECTORIES)
get_target_property(NETXDUO_INCLUDES azrtos::netxduo INCLUDE_DIRECTORIES)
include_directories(${THREADX_INCLUDES} ${NETXDUO_INCLUDES})
add_subdirectory(${SHARED_LIB_DIR}/zenoh-pico zenoh-pico)


add_subdirectory(stm32cubef4)
add_subdirectory(mxchip_bsp)
add_subdirectory(wiced_sdk)
