add_library(be SHARED "")

file(GLOB_RECURSE privateHeaderFiles "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

target_sources(be PRIVATE
)

target_link_libraries(be PUBLIC root linprog icon agg icu)


add_library(bootstrap_be STATIC "")

target_sources(bootstrap_be PRIVATE
)

target_include_directories(bootstrap_be PRIVATE
	"../../headers/private/interface"
)


add_subdirectory(app)
add_subdirectory(game)
add_subdirectory(interface)
add_subdirectory(locale)
#add_subdirectory(network)
add_subdirectory(shared)
add_subdirectory(storage)
add_subdirectory(support)
add_subdirectory(textencoding)
add_subdirectory(translation)
add_subdirectory(tracker)
