add_subdirectory(comp1)
add_subdirectory(comp2)
include(Format)
include(ClangTidy)

add_executable(comp1_exec comp.cpp file_sys_util.cpp sockets_util.cpp)
target_include_directories(comp1_exec PUBLIC ./inc)
target_link_libraries(comp1_exec PRIVATE comp1)
Format(comp1_exec .)

add_executable(comp2_exec comp.cpp file_sys_util.cpp sockets_util.cpp)
target_include_directories(comp2_exec PUBLIC ./inc)
target_link_libraries(comp2_exec PRIVATE comp2)
Format(comp2_exec .)

AddClangTidy(comp2_exec)
AddClangTidy(comp1_exec)

# Install commands
install(TARGETS comp1_exec DESTINATION bin)
install(TARGETS comp2_exec DESTINATION bin)