add_library(comp2 OBJECT
    comp2_main.cpp
    comp2.cpp
)
target_include_directories(comp2 PUBLIC ./inc ../inc)