# Copy and configure the following in your cmake script before adding the regolith-lain subdirectory.
# Due to thinking about her I completely forgot what BAS stands for two minutes after making the file.
# Update: It stands for "Before Add Subdirectory" ...

# VVV COPY FROM HERE VVV

# ===== regolith-lain BAS begin =====
# Location of regolith-lain.
set( RGH_ROOT_DIR "$ENV{RGH_ROOT_DIR}" )
include( "${RGH_ROOT_DIR}/tools.cmake" )

# Build target plate.
# - "uC" - target is a microcontroller device.
# - "OS" - target is an operating system ready device. 
set( RGH_TARGET_PLATE "" )

# Target operating system.
# - OSp:
#   - "Windows"
#   - "Linux"
# - uCp:
#   - <empty>
#   - "FreeRTOS"
set( RGH_TARGET_OS "" ) 

# Target platform.
# - OSp:
#   - <empty>
# - uCp:
#   - "esp32"
set( RGH_TARGET_PLATFORM "" )

# * Which external components to make, indicated by a string containing 'y' and 'n' ( for "yes" and "no" ), in the order in which they are built in the script.
# Example: "ynnny" - makes the first and fifth external component.
set( RGH_EXCOM_POPCOUNT
    "n"
)

# Add regolith-lain.
add_subdirectory( "${RGH_ROOT_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/regolith-lain" )
# ===== regolith-lain BAS end =====
