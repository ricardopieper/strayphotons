add_library(picojson INTERFACE)

target_include_directories(
    picojson
    INTERFACE
        ./
        ./picojson
)
