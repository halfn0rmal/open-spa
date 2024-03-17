idf_component_register(SRCS 
"gatts_table_creat_demo.c"
"src/input_manager.c" 
"src/output_manager.c" 
"src/state_handler.c" 
"src/bus_manager.c"
"src/config.c"
                    INCLUDE_DIRS ".")
target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-format")