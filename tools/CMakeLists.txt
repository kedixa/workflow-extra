link_directories(${WORKFLOW_LIB_DIR})

foreach(example wfdig)
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
