# Register this directory as an ESP-IDF component.
# This tells the build system to look for source files here.
# SRCS specifies the source files to compile within this component.
# This should list your main.c (or main.cpp) file.
idf_component_register(SRCS "main.c")
