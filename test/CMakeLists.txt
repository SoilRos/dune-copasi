enable_testing()

dune_symlink_to_source_files(FILES "grids" DESTINATION "." )
dune_symlink_to_source_files(FILES "reference" DESTINATION "." )
dune_symlink_to_source_files(FILES "data" DESTINATION "." )

# Some dune tests modify the EXCLUDE_FROM_ALL propertie of targets
# Therefore, we store them temporally and set them back after seting up the tests
get_target_property(TMP_SD_EFA dune_copasi_sd EXCLUDE_FROM_ALL)
get_target_property(TMP_MD_EFA dune_copasi_md EXCLUDE_FROM_ALL)

dune_add_test(NAME            test_dynamic_power_local_finite_element
              SOURCES         test_dynamic_power_local_finite_element.cc
              LABELS          unit)

dune_add_test(NAME            test_concepts_pdelab
              SOURCES         test_concepts_pdelab.cc
              LABELS          unit)

dune_add_test(NAME            test_concepts_grid
              SOURCES         test_concepts_grid.cc
              LABELS          unit)

dune_add_test(NAME            test_concepts_typetree
              SOURCES         test_concepts_typetree.cc
              LABELS          unit)

dune_add_test(NAME            test_tiff_grayscale
              SOURCES         test_tiff_grayscale.cc
              LINK_LIBRARIES  TIFF::TIFF
              LABELS          unit)

add_custom_target(build_system_tests DEPENDS dune_copasi)

add_executable(dune_copasi_sd_compare EXCLUDE_FROM_ALL dune_copasi_sd_compare.cc)
add_executable(dune_copasi_md_compare EXCLUDE_FROM_ALL dune_copasi_md_compare.cc)
add_executable(dune_copasi_md_jacobian EXCLUDE_FROM_ALL test_jacobian.cc)

target_link_libraries(dune_copasi_sd_compare PRIVATE dune_copasi_sd_lib)
target_link_libraries(dune_copasi_md_compare PRIVATE dune_copasi_md_lib)
target_link_libraries(dune_copasi_md_jacobian PRIVATE dune_copasi_md_lib)

add_dependencies(build_system_tests
  dune_copasi_sd
  dune_copasi_sd_compare
  dune_copasi_md_compare
)

add_dependencies(build_unit_tests
  dune_copasi_md_jacobian
)

if (dune-testtools_FOUND)

  add_system_test_per_target(
    TARGET          dune_copasi_sd_compare
    INIFILE         test_exp.mini)

  add_system_test_per_target(
    TARGET          dune_copasi_md_compare
    INIFILE         test_exp.mini)

  add_system_test_per_target(
    TARGET          dune_copasi_sd_compare
    INIFILE         test_initial.mini)

  add_system_test_per_target(
    TARGET          dune_copasi_md_compare
    INIFILE         test_initial.mini)

  add_system_test_per_target(
    TARGET          dune_copasi_sd_compare
    INIFILE         test_gauss.mini)

  add_system_test_per_target(
    TARGET          dune_copasi_md_compare
    INIFILE         test_gauss.mini)

  add_system_test_per_target(
    TARGET          dune_copasi_md_compare
    INIFILE         test_gauss_mixed_geometry.mini)

  add_system_test_per_target(
    TARGET          dune_copasi_sd
    SCRIPT          dune_vtkcompare.py
    INIFILE         test_tiff.mini)

  add_system_test_per_target(
    TARGET          dune_copasi_sd
    INIFILE         test_brusselator.mini)

  add_system_test_per_target(
    TARGET          dune_copasi_md
    SCRIPT          dune_vtkcompare.py
    INIFILE         test_tiff.mini)

  add_system_test_per_target(
    TARGET          dune_copasi_md
    SCRIPT          dune_vtkcompare.py
    INIFILE         test_cell.mini)

  add_system_test_per_target(
    TARGET          dune_copasi_sd
    INIFILE         test_no_writer.mini)

  add_system_test_per_target(
    TARGET          dune_copasi_md
    INIFILE         test_no_writer.mini)

  add_system_test_per_target(
    TARGET          dune_copasi_md_jacobian
    INIFILE         test_cell.mini)

  set_property(TEST "dune_copasi_md_jacobian_test_cell" PROPERTY LABELS unit)
  set_property(TEST "dune_copasi_md_jacobian_test_cell" PROPERTY LABELS unit)

  add_system_test_per_target(
    TARGET          dune_copasi_md_jacobian
    INIFILE         test_NFkappaB.mini)

  set_property(TEST "dune_copasi_md_jacobian_test_NFkappaB" PROPERTY LABELS unit)
  set_property(TEST "dune_copasi_md_jacobian_test_NFkappaB" PROPERTY LABELS unit)

endif()

# Set back EXCLUDE_FROM_ALL target properties
set_target_properties(dune_copasi_sd PROPERTIES EXCLUDE_FROM_ALL ${TMP_SD_EFA})
set_target_properties(dune_copasi_md PROPERTIES EXCLUDE_FROM_ALL ${TMP_MD_EFA})
