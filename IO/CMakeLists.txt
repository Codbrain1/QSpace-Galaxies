
add_library(IO STATIC)
add_library(QSpace::IO ALIAS IO)
set(IO_BIN
BIN/BINReader.h
BIN/BINReader.cpp

)
# IO_GRD
# IO_HDF5
# IO_VTK
target_sources(IO PUBLIC
            ReaderFactory.cpp
            ReaderFactory.h
            ReadResult.h
            SchemePresets.h
            SchemePresets.cpp
            ${IO_BIN}
)   

target_include_directories(IO PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(IO PUBLIC
        QSpace::Common
        Qt6::Core
)

add_subdirectory(BIN/Testing)