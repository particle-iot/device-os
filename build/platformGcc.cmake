# Add Boost
find_package(Boost
  1.59.0 EXACT
  REQUIRED
  COMPONENTS program_options random regex system thread
)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
  link_libraries(${Boost_LIBRARIES})
endif()
