# ==========================================================================
# dependencies/LibUSB.cmake
# --------------------------------------------------------------------------
# USB device access library.  Conditional on MBOOTCORE_ENABLE_USB.
# Windows: WinUSB built-in.  Linux/macOS: libusb from GitHub.
# ==========================================================================

message(STATUS "")

set(MBOOTCORE_USB_SOURCE "none" CACHE INTERNAL "USB backend source")

if(MBOOTCORE_ENABLE_USB)
    if(WIN32)
        set(MBOOTCORE_USB_SOURCE "WinUSB (built-in)")
        set(MBOOTCORE_USB_VERSION "N/A (Windows SDK)")
        if(NOT TARGET LibUSB::LibUSB)
            add_library(LibUSB::LibUSB INTERFACE IMPORTED)
        endif()
    else()
        set(BOOTCORE_LIBUSB_SHARED ${BUILD_SHARED_LIBS})
        set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
        set(LIBUSB_INSTALL_TARGETS OFF CACHE BOOL "" FORCE)
        set(LIBUSB_EXPORT_INSTALL_TARGETS OFF CACHE BOOL "" FORCE)
        set(LIBUSB_BUILD_TESTING OFF CACHE BOOL "" FORCE)
        set(LIBUSB_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        # Disable udev on Linux only (does not exist on macOS)
        if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
            set(LIBUSB_ENABLE_UDEV OFF CACHE BOOL "" FORCE)
        endif()

        mbootcore_require_dependency(NAME libusb)

        set(BUILD_SHARED_LIBS ${BOOTCORE_LIBUSB_SHARED})

        if(NOT TARGET LibUSB::LibUSB)
            if(TARGET usb-1.0)
                add_library(LibUSB::LibUSB ALIAS usb-1.0)
            else()
                message(FATAL_ERROR
                    "libusb source exists but no usable target found. "
                    "Expected usb-1.0."
                )
            endif()
        endif()

        set(MBOOTCORE_USB_SOURCE "libusb (${MBOOTCORE_LIBUSB_VERSION})")
        set(MBOOTCORE_USB_VERSION "${MBOOTCORE_LIBUSB_VERSION}")

        # Object files merged into mbootcore via MonolithicArchive.cmake
    endif()

    if(NOT "${MBOOTCORE_USB_SOURCE}" STREQUAL "none")
        set(MBOOTCORE_HAVE_USB ON)
        message(STATUS "")
        message(STATUS "USB Backend")
        message(STATUS "  Status:   ENABLED")
        message(STATUS "  Provider: ${MBOOTCORE_USB_SOURCE}")
        message(STATUS "  Version:  ${MBOOTCORE_USB_VERSION}")
        message(STATUS "")
    else()
        set(MBOOTCORE_HAVE_USB OFF)
        message(STATUS "")
        message(STATUS "USB Backend")
        message(STATUS "  Status:  DISABLED")
        message(STATUS "  Reason:  libusb not found")
        message(STATUS "  Effect:  USB device discovery and communication unavailable")
        message(STATUS "")
    endif()
else()
    set(MBOOTCORE_HAVE_USB OFF)
    message(STATUS "")
    message(STATUS "USB Backend")
    message(STATUS "  Status:  DISABLED")
    message(STATUS "  Reason:  MBOOTCORE_ENABLE_USB=OFF")
    message(STATUS "")
endif()
