add_executable(dune_copasi dune-copasi.cc)
target_link_libraries(dune_copasi PRIVATE TIFF::TIFF muparser::muparser dune_copasi_lib)