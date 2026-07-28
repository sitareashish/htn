add_executable(test_net
    vendor/munit.c
    test_net.c
)
target_include_directories(test_net PRIVATE vendor)
target_link_libraries(test_net PRIVATE htn_lib)

# Munit itself has warnings we don't want to fight.
target_compile_options(test_net PRIVATE
    -Wno-conversion -Wno-shadow -Wno-format
)

add_test(NAME test_net COMMAND test_net --no-fork)

add_executable(test_conn
    vendor/munit.c
    test_conn.c
)
target_include_directories(test_conn PRIVATE vendor)
target_link_libraries(test_conn PRIVATE htn_lib)
target_compile_options(test_conn PRIVATE -Wno-conversion -Wno-shadow -Wno-format)
add_test(NAME test_conn COMMAND test_conn --no-fork)
