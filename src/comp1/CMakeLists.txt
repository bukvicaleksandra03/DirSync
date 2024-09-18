add_library(comp1 OBJECT
    comp1_main.cpp
    comp1.cpp
)
target_include_directories(comp1 PUBLIC ./inc ../inc)