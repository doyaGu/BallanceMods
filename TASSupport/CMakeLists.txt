# MinHook
add_subdirectory(minhook EXCLUDE_FROM_ALL)

# TASSupport
add_bml_mod(TASSupport TASSupport.cpp TASSupport.h physics_RT.cpp physics_RT.h)
target_link_libraries(TASSupport PRIVATE minhook)
install_bml_mod(TASSupport)