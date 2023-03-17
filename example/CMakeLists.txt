link_directories(${WORKFLOW_LIB_DIR})

foreach(example PipeRedisExample QpsPoolExample LatchExample GoPipeExample
        RefPtrExample)
    add_executable(${example} ${example}.cpp)
    add_dependencies(${example} workflow-extra)

    target_link_libraries(${example}
        workflow
        workflow-extra
        Threads::Threads
        OpenSSL::SSL
        OpenSSL::Crypto
    )

endforeach()
