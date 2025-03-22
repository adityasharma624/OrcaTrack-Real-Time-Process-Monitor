add_library(monitor_lib
    process_monitor.cpp
)

target_include_directories(monitor_lib PUBLIC
    ${CMAKE_SOURCE_DIR}
)

if(WIN32)
    target_link_libraries(monitor_lib PRIVATE
        pdh.lib
        psapi.lib
        version.lib
        advapi32.lib
    )
endif() 