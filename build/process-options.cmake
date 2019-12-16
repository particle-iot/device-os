add_compile_definitions(
    PLATFORM_NAME=${PARTICLE_PLATFORM}
    PLATFORM_ID=${PARTICLE_PLATFORM_ID}
    PLATFORM_THREADING=${PARTICLE_PLATFORM_THREADING}
    USBD_VID_SPARK=${PARTICLE_USBD_VID}
    USBD_PID_CDC=${PARTICLE_USBD_PID}
)

if (${PARTICLE_COMPILE_LTO})
    message(STATUS "Enabled link time optimizations")
    add_compile_options(
        -flto
    )

    add_link_options(
        -fuse-linker-plugin
    )
endif()

if (${PARTICLE_RELEASE_BUILD})
    message(STATUS "Generating release build")
    add_compile_definitions(
        RELEASE_BUILD
    )
else()
    message(STATUS "Generating debug build")
    add_compile_definitions(
        DEBUG_BUILD
    )
endif()
