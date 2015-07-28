add_executable(async_client
    main.cpp
    )

target_link_libraries(async_client LINK_PUBLIC ${Boost_LIBRARIES})
target_link_libraries(async_client LINK_PUBLIC ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(async_client LINK_PUBLIC ${OPENSSL_LIBRARIES})
