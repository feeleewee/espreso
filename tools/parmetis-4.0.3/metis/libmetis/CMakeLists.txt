# Add this directory for internal users.
include_directories(.)
# Find sources.
file(GLOB metis_sources *.c)
# Build libmetis.
add_library(metis ${METIS_LIBRARY_TYPE} ${GKlib_sources} ${metis_sources})
if(UNIX)
  target_link_libraries(metis m)
endif()

install(TARGETS metis
  LIBRARY DESTINATION lib/lib${IDXTYPEWIDTH}
  RUNTIME DESTINATION lib/lib${IDXTYPEWIDTH}
  ARCHIVE DESTINATION lib/lib${IDXTYPEWIDTH})
