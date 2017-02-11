
add_library(iitracer SHARED src/iitracer.cc)

set(instrument_FLAGS
  "-finstrument-functions"
  "-finstrument-functions-exclude-file-list=/usr/"
  "-finstrument-functions-exclude-function-list=static_initialization_and_destruction,main"
)

target_compile_options(iitracer INTERFACE "${instrument_FLAGS}")
target_compile_options(iitracer PRIVATE "-Wall" "-Wextra" )

target_include_directories(iitracer PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
  PRIVATE src)

target_link_libraries(iitracer "-pthread -lunwind -lwtf")

add_executable(iitest test/test.cc)
target_compile_options(iitest PRIVATE "-Wall" "-Wextra")
target_link_libraries(iitest "-pthread" iitracer)


install(TARGETS iitracer LIBRARY DESTINATION lib)
install(DIRECTORY include/ DESTINATION include)
