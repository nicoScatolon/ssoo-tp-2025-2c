cmake_minimum_required(VERSION 3.20)
project(multimodulo C)

# Define los módulos
set(MODULES master querycontrol storage worker)

# Target ALL - compila todos los módulos
add_custom_target(all_modules ALL
        COMMENT "Compilando todos los módulos..."
)

# Crea targets individuales para cada módulo
foreach(MODULE ${MODULES})
    # Target de compilación (make all)
    add_custom_target(${MODULE}
            COMMAND make -C ${CMAKE_SOURCE_DIR}/${MODULE}
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/${MODULE}
            COMMENT "Compilando ${MODULE}..."
            USES_TERMINAL
    )

    # Target clean individual
    add_custom_target(${MODULE}_clean
            COMMAND make -C ${CMAKE_SOURCE_DIR}/${MODULE} clean
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/${MODULE}
            COMMENT "Limpiando ${MODULE}..."
            USES_TERMINAL
    )

    # Agregar módulo al target all_modules
    add_dependencies(all_modules ${MODULE})
endforeach()

# Target CLEAN global - limpia todos los módulos
add_custom_target(clean_all
        COMMENT "Limpiando todos los módulos..."
)

foreach(MODULE ${MODULES})
    add_dependencies(clean_all ${MODULE}_clean)
endforeach()