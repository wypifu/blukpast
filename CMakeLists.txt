cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
set(CMAKE_LEGACY_CYGWIN_WIN32 0)
set(CMAKE_VERBOSE_MAKEFILE on)
project("blukpast")

IF(NOT DEFINED LINK_MODE)
    set(LINK_MODE bothLink)
ENDIF(NOT DEFINED LINK_MODE)

set(LIB_NAME_STATIC ${PROJECT_NAME}Static)
set(LIB_NAME_SHARED ${PROJECT_NAME})
# LIB_NAME kept for compatibility (single-mode paths below)
IF(${LINK_MODE} MATCHES staticLink AND NOT ${LINK_MODE} MATCHES bothLink)
	set(LIB_NAME ${LIB_NAME_STATIC})
ELSE()
	set(LIB_NAME ${LIB_NAME_SHARED})
ENDIF()

IF(WIN32)
    include_directories(
        $ENV{GLFW}\\include
        $ENV{VULKAN_SDK}\\include
        ${CMAKE_SOURCE_DIR}
        )
	link_directories($ENV{VULKAN_SDK}/Lib $ENV{GLFW}/lib-vc2019)
ELSE()
    include_directories(${CMAKE_SOURCE_DIR})
ENDIF(WIN32)

set(CMAKE_C_STANDARD 23)

message(STATUS "\tCmaking " ${LIB_NAME})
message(STATUS "\tsrc_dir : " ${CMAKE_CURRENT_SOURCE_DIR})
message(STATUS "\tbkp_lib_dir : " : ${CMAKE_CURRENT_BINARY_DIR})
message(STATUS "\tlink_mode : " ${LINK_MODE})
message(STATUS "\tVK : " $ENV{VULKAN_SDK}\\include)
message(STATUS "\tGLFW : " $ENV{GLFW}\\include)
message(STATUS "\tBuildType: " ${CMAKE_BUILD_TYPE})

if(NOT MSVC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msse4.1")
endif()

IF(WIN32)
	set(CMAKE_C_FLAGS_DEBUG "-Wall ${CMAKE_C_FLAGS} -Wno-null-dereference -D_CRT_SECURE_NO_WARNINGS -DDEBUG -DBKP_EXPORT -Wwrite-strings -Wuninitialized -Wunreachable-code -Wshadow -O0 -ggdb")
	set(CMAKE_C_FLAGS_RELEASE "-Wall ${CMAKE_C_FLAGS} -Wno-null-dereference -D_CRT_SECURE_NO_WARNINGS -DBKP_EXPORT -Wwrite-strings -Wuninitialized -Wunreachable-code -Wshadow -Wfloat-equal -O3")
ELSE()
	set(CMAKE_C_FLAGS_DEBUG "-Wall ${CMAKE_C_FLAGS} -DDEBUG -Wwrite-strings -Wuninitialized -Wanalyzer-use-of-uninitialized-value -Wunreachable-code -Wshadow -O0 -ggdb -pg")
	set(CMAKE_C_FLAGS_RELEASE "-Wall ${CMAKE_C_FLAGS} -Wwrite-strings -Wuninitialized -Wanalyzer-use-of-uninitialized-value -Wunreachable-code -Wshadow -Wfloat-equal -O3")
ENDIF(WIN32)

set(core_src
        blukpast.c

		thirdparty/SPIRV/spirv_reflect.c
		gfx/vulkan/vulkan.c
		gfx/vulkan/vk_cmd.c
		gfx/vulkan/vk_info.c
		gfx/vulkan/vk_pipeline.c
		gfx/vulkan/vk_shader_module.c
		gfx/vulkan/pipelines_helper.c
		gfx/vulkan/vkAllocator.c
		gfx/vulkan/vk_buffer.c
		gfx/vulkan/vk_texture_batch.c

		gfx/window_manager/windowing.c
		gfx/bkp_font.c

		system/bkp_allocator.c
		system/bkp_array.c
		system/bkp_input.c
		system/bkp_stack.c
		system/bkp_list.c
		system/bkp_map.c
		system/bkp_hash.c
		system/bkp_string.c
		system/bkp_container_functions.c
		system/bkp_fs.c
		system/bkp_log.c
		system/bkp_path.c
		system/bkp_systemTime.c
		system/bkp_thread.c
		system/bkp_utf8.c
		system/bkp_random.c

		math/basics.c
		math/bkp_float.c
		math/bkp_mat.c
		math/bkp_vect.c
		math/bkp_linear_algebra2D.c
		math/bkp_linear_algebra3D.c
		math/bkp_vertex_geometries.c

		loaders/bkp_loaders.c
		loaders/bkp_loader_gltf.c
		loaders/bkp_loader_obj.c
		loaders/bkp_loader_stl.c
		loaders/bkp_loader_ply.c
        )

IF(${LINK_MODE} MATCHES staticLink OR ${LINK_MODE} MATCHES bothLink)
	add_library(${LIB_NAME_STATIC} STATIC ${core_src})
	IF(WIN32)
		target_link_libraries(${LIB_NAME_STATIC} glfw3 vulkan-1 winmm)
	ELSE()
		target_link_libraries(${LIB_NAME_STATIC} pthread glfw vulkan)
	ENDIF(WIN32)
ENDIF(${LINK_MODE} MATCHES staticLink OR ${LINK_MODE} MATCHES bothLink)

IF(${LINK_MODE} MATCHES dynamicLink OR ${LINK_MODE} MATCHES bothLink)
	add_library(${LIB_NAME_SHARED} SHARED ${core_src})
	IF(WIN32)
		target_link_libraries(${LIB_NAME_SHARED} glfw3 vulkan-1 winmm)
	ELSE()
		target_link_libraries(${LIB_NAME_SHARED} pthread glfw vulkan)
	ENDIF(WIN32)
ENDIF(${LINK_MODE} MATCHES dynamicLink OR ${LINK_MODE} MATCHES bothLink)

find_program(GLSLC glslc HINTS $ENV{VULKAN_SDK}/bin $ENV{VULKAN_SDK}/Bin)

add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/tutorial/00_hello_world/shaders/triangle.vert.spv
           ${CMAKE_BINARY_DIR}/tutorial/00_hello_world/shaders/triangle.frag.spv
           ${CMAKE_BINARY_DIR}/tutorial/00_hello_world/shaders/sphere.vert.spv
           ${CMAKE_BINARY_DIR}/tutorial/00_hello_world/shaders/sphere.frag.spv
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/tutorial/00_hello_world/shaders
    COMMAND ${GLSLC} ${CMAKE_SOURCE_DIR}/tutorial/00_hello_world/shaders/triangle.vert -o ${CMAKE_BINARY_DIR}/tutorial/00_hello_world/shaders/triangle.vert.spv
    COMMAND ${GLSLC} ${CMAKE_SOURCE_DIR}/tutorial/00_hello_world/shaders/triangle.frag -o ${CMAKE_BINARY_DIR}/tutorial/00_hello_world/shaders/triangle.frag.spv
    COMMAND ${GLSLC} ${CMAKE_SOURCE_DIR}/tutorial/00_hello_world/shaders/sphere.vert   -o ${CMAKE_BINARY_DIR}/tutorial/00_hello_world/shaders/sphere.vert.spv
    COMMAND ${GLSLC} ${CMAKE_SOURCE_DIR}/tutorial/00_hello_world/shaders/sphere.frag   -o ${CMAKE_BINARY_DIR}/tutorial/00_hello_world/shaders/sphere.frag.spv
    DEPENDS ${CMAKE_SOURCE_DIR}/tutorial/00_hello_world/shaders/triangle.vert
            ${CMAKE_SOURCE_DIR}/tutorial/00_hello_world/shaders/triangle.frag
            ${CMAKE_SOURCE_DIR}/tutorial/00_hello_world/shaders/sphere.vert
            ${CMAKE_SOURCE_DIR}/tutorial/00_hello_world/shaders/sphere.frag
)
add_custom_target(tutorial_00_shaders DEPENDS
    ${CMAKE_BINARY_DIR}/tutorial/00_hello_world/shaders/triangle.vert.spv
    ${CMAKE_BINARY_DIR}/tutorial/00_hello_world/shaders/triangle.frag.spv
    ${CMAKE_BINARY_DIR}/tutorial/00_hello_world/shaders/sphere.vert.spv
    ${CMAKE_BINARY_DIR}/tutorial/00_hello_world/shaders/sphere.frag.spv
)

add_executable(tutorial_00 tutorial/00_hello_world/main.c ${core_src})
add_dependencies(tutorial_00 tutorial_00_shaders)
IF(WIN32)
	target_link_libraries(tutorial_00 glfw3 vulkan-1 winmm)
ELSE()
	target_link_libraries(tutorial_00 pthread glfw vulkan m)
ENDIF(WIN32)

add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/tutorial/01_playground/shaders/scene.vert.spv
           ${CMAKE_BINARY_DIR}/tutorial/01_playground/shaders/scene.frag.spv
           ${CMAKE_BINARY_DIR}/tutorial/01_playground/shaders/sky.vert.spv
           ${CMAKE_BINARY_DIR}/tutorial/01_playground/shaders/sky.frag.spv
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/tutorial/01_playground/shaders
    COMMAND ${GLSLC} ${CMAKE_SOURCE_DIR}/tutorial/01_playground/shaders/scene.vert -o ${CMAKE_BINARY_DIR}/tutorial/01_playground/shaders/scene.vert.spv
    COMMAND ${GLSLC} ${CMAKE_SOURCE_DIR}/tutorial/01_playground/shaders/scene.frag -o ${CMAKE_BINARY_DIR}/tutorial/01_playground/shaders/scene.frag.spv
    COMMAND ${GLSLC} ${CMAKE_SOURCE_DIR}/tutorial/01_playground/shaders/sky.vert   -o ${CMAKE_BINARY_DIR}/tutorial/01_playground/shaders/sky.vert.spv
    COMMAND ${GLSLC} ${CMAKE_SOURCE_DIR}/tutorial/01_playground/shaders/sky.frag   -o ${CMAKE_BINARY_DIR}/tutorial/01_playground/shaders/sky.frag.spv
    DEPENDS ${CMAKE_SOURCE_DIR}/tutorial/01_playground/shaders/scene.vert
            ${CMAKE_SOURCE_DIR}/tutorial/01_playground/shaders/scene.frag
            ${CMAKE_SOURCE_DIR}/tutorial/01_playground/shaders/sky.vert
            ${CMAKE_SOURCE_DIR}/tutorial/01_playground/shaders/sky.frag
)
add_custom_target(tutorial_01_shaders DEPENDS
    ${CMAKE_BINARY_DIR}/tutorial/01_playground/shaders/scene.vert.spv
    ${CMAKE_BINARY_DIR}/tutorial/01_playground/shaders/scene.frag.spv
    ${CMAKE_BINARY_DIR}/tutorial/01_playground/shaders/sky.vert.spv
    ${CMAKE_BINARY_DIR}/tutorial/01_playground/shaders/sky.frag.spv
)

add_executable(tutorial_01 tutorial/01_playground/main.c tutorial/common/scene.c ${core_src})
add_dependencies(tutorial_01 tutorial_01_shaders)
IF(WIN32)
	target_link_libraries(tutorial_01 glfw3 vulkan-1 winmm)
ELSE()
	target_link_libraries(tutorial_01 pthread glfw vulkan m)
ENDIF(WIN32)

add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/tutorial/02_camera/shaders/scene.vert.spv
           ${CMAKE_BINARY_DIR}/tutorial/02_camera/shaders/scene.frag.spv
           ${CMAKE_BINARY_DIR}/tutorial/02_camera/shaders/sky.vert.spv
           ${CMAKE_BINARY_DIR}/tutorial/02_camera/shaders/sky.frag.spv
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/tutorial/02_camera/shaders
    COMMAND ${GLSLC} ${CMAKE_SOURCE_DIR}/tutorial/02_camera/shaders/scene.vert -o ${CMAKE_BINARY_DIR}/tutorial/02_camera/shaders/scene.vert.spv
    COMMAND ${GLSLC} ${CMAKE_SOURCE_DIR}/tutorial/02_camera/shaders/scene.frag -o ${CMAKE_BINARY_DIR}/tutorial/02_camera/shaders/scene.frag.spv
    COMMAND ${GLSLC} ${CMAKE_SOURCE_DIR}/tutorial/02_camera/shaders/sky.vert   -o ${CMAKE_BINARY_DIR}/tutorial/02_camera/shaders/sky.vert.spv
    COMMAND ${GLSLC} ${CMAKE_SOURCE_DIR}/tutorial/02_camera/shaders/sky.frag   -o ${CMAKE_BINARY_DIR}/tutorial/02_camera/shaders/sky.frag.spv
    DEPENDS ${CMAKE_SOURCE_DIR}/tutorial/02_camera/shaders/scene.vert
            ${CMAKE_SOURCE_DIR}/tutorial/02_camera/shaders/scene.frag
            ${CMAKE_SOURCE_DIR}/tutorial/02_camera/shaders/sky.vert
            ${CMAKE_SOURCE_DIR}/tutorial/02_camera/shaders/sky.frag
)
add_custom_target(tutorial_02_shaders DEPENDS
    ${CMAKE_BINARY_DIR}/tutorial/02_camera/shaders/scene.vert.spv
    ${CMAKE_BINARY_DIR}/tutorial/02_camera/shaders/scene.frag.spv
    ${CMAKE_BINARY_DIR}/tutorial/02_camera/shaders/sky.vert.spv
    ${CMAKE_BINARY_DIR}/tutorial/02_camera/shaders/sky.frag.spv
)

add_executable(tutorial_02 tutorial/02_camera/main.c tutorial/common/scene.c ${core_src})
add_dependencies(tutorial_02 tutorial_02_shaders)
IF(WIN32)
	target_link_libraries(tutorial_02 glfw3 vulkan-1 winmm)
ELSE()
	target_link_libraries(tutorial_02 pthread glfw vulkan m)
ENDIF(WIN32)

add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/tutorial/03_pbr_ibl/shaders/scene.vert.spv
           ${CMAKE_BINARY_DIR}/tutorial/03_pbr_ibl/shaders/scene.frag.spv
           ${CMAKE_BINARY_DIR}/tutorial/03_pbr_ibl/shaders/sky.vert.spv
           ${CMAKE_BINARY_DIR}/tutorial/03_pbr_ibl/shaders/sky.frag.spv
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/tutorial/03_pbr_ibl/shaders
    COMMAND ${GLSLC} ${CMAKE_SOURCE_DIR}/tutorial/03_pbr_ibl/shaders/scene.vert -o ${CMAKE_BINARY_DIR}/tutorial/03_pbr_ibl/shaders/scene.vert.spv
    COMMAND ${GLSLC} ${CMAKE_SOURCE_DIR}/tutorial/03_pbr_ibl/shaders/scene.frag -o ${CMAKE_BINARY_DIR}/tutorial/03_pbr_ibl/shaders/scene.frag.spv
    COMMAND ${GLSLC} ${CMAKE_SOURCE_DIR}/tutorial/03_pbr_ibl/shaders/sky.vert   -o ${CMAKE_BINARY_DIR}/tutorial/03_pbr_ibl/shaders/sky.vert.spv
    COMMAND ${GLSLC} ${CMAKE_SOURCE_DIR}/tutorial/03_pbr_ibl/shaders/sky.frag   -o ${CMAKE_BINARY_DIR}/tutorial/03_pbr_ibl/shaders/sky.frag.spv
    DEPENDS ${CMAKE_SOURCE_DIR}/tutorial/03_pbr_ibl/shaders/scene.vert
            ${CMAKE_SOURCE_DIR}/tutorial/03_pbr_ibl/shaders/scene.frag
            ${CMAKE_SOURCE_DIR}/tutorial/03_pbr_ibl/shaders/sky.vert
            ${CMAKE_SOURCE_DIR}/tutorial/03_pbr_ibl/shaders/sky.frag
)
add_custom_target(tutorial_03_shaders DEPENDS
    ${CMAKE_BINARY_DIR}/tutorial/03_pbr_ibl/shaders/scene.vert.spv
    ${CMAKE_BINARY_DIR}/tutorial/03_pbr_ibl/shaders/scene.frag.spv
    ${CMAKE_BINARY_DIR}/tutorial/03_pbr_ibl/shaders/sky.vert.spv
    ${CMAKE_BINARY_DIR}/tutorial/03_pbr_ibl/shaders/sky.frag.spv
)

add_executable(tutorial_03 tutorial/03_pbr_ibl/main.c tutorial/03_pbr_ibl/ibl.c tutorial/common/scene.c ${core_src})
add_dependencies(tutorial_03 tutorial_03_shaders copy_gltf_assets)
IF(WIN32)
	target_link_libraries(tutorial_03 glfw3 vulkan-1 winmm)
ELSE()
	target_link_libraries(tutorial_03 pthread glfw vulkan m)
ENDIF(WIN32)

add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/tutorial/04_font_input/shaders/hud.vert.spv
           ${CMAKE_BINARY_DIR}/tutorial/04_font_input/shaders/hud.frag.spv
           ${CMAKE_BINARY_DIR}/tutorial/04_font_input/shaders/rect.vert.spv
           ${CMAKE_BINARY_DIR}/tutorial/04_font_input/shaders/rect.frag.spv
           ${CMAKE_BINARY_DIR}/tutorial/04_font_input/AdwaitaMono-Regular.ttf
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/tutorial/04_font_input/shaders
    COMMAND ${GLSLC} ${CMAKE_SOURCE_DIR}/tutorial/04_font_input/shaders/hud.vert  -o ${CMAKE_BINARY_DIR}/tutorial/04_font_input/shaders/hud.vert.spv
    COMMAND ${GLSLC} ${CMAKE_SOURCE_DIR}/tutorial/04_font_input/shaders/hud.frag  -o ${CMAKE_BINARY_DIR}/tutorial/04_font_input/shaders/hud.frag.spv
    COMMAND ${GLSLC} ${CMAKE_SOURCE_DIR}/tutorial/04_font_input/shaders/rect.vert -o ${CMAKE_BINARY_DIR}/tutorial/04_font_input/shaders/rect.vert.spv
    COMMAND ${GLSLC} ${CMAKE_SOURCE_DIR}/tutorial/04_font_input/shaders/rect.frag -o ${CMAKE_BINARY_DIR}/tutorial/04_font_input/shaders/rect.frag.spv
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/tutorial/04_font_input/AdwaitaMono-Regular.ttf
        ${CMAKE_BINARY_DIR}/tutorial/04_font_input/AdwaitaMono-Regular.ttf
    DEPENDS ${CMAKE_SOURCE_DIR}/tutorial/04_font_input/shaders/hud.vert
            ${CMAKE_SOURCE_DIR}/tutorial/04_font_input/shaders/hud.frag
            ${CMAKE_SOURCE_DIR}/tutorial/04_font_input/shaders/rect.vert
            ${CMAKE_SOURCE_DIR}/tutorial/04_font_input/shaders/rect.frag
            ${CMAKE_SOURCE_DIR}/tutorial/04_font_input/AdwaitaMono-Regular.ttf
)
add_custom_target(tutorial_04_shaders DEPENDS
    ${CMAKE_BINARY_DIR}/tutorial/04_font_input/shaders/hud.vert.spv
    ${CMAKE_BINARY_DIR}/tutorial/04_font_input/shaders/hud.frag.spv
    ${CMAKE_BINARY_DIR}/tutorial/04_font_input/shaders/rect.vert.spv
    ${CMAKE_BINARY_DIR}/tutorial/04_font_input/shaders/rect.frag.spv
    ${CMAKE_BINARY_DIR}/tutorial/04_font_input/AdwaitaMono-Regular.ttf
)

add_executable(tutorial_04 tutorial/04_font_input/main.c tutorial/common/hud.c tutorial/common/scene.c ${core_src})
add_dependencies(tutorial_04 tutorial_04_shaders copy_gltf_assets)
IF(WIN32)
	target_link_libraries(tutorial_04 glfw3 vulkan-1 winmm)
ELSE()
	target_link_libraries(tutorial_04 pthread glfw vulkan m)
ENDIF(WIN32)

add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/tutorial/05_shadow_map/shaders/scene.vert.spv
           ${CMAKE_BINARY_DIR}/tutorial/05_shadow_map/shaders/scene.frag.spv
           ${CMAKE_BINARY_DIR}/tutorial/05_shadow_map/shaders/shadow.vert.spv
           ${CMAKE_BINARY_DIR}/tutorial/05_shadow_map/shaders/shadow.frag.spv
           ${CMAKE_BINARY_DIR}/images/bkpview_wallpaper2.png
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/tutorial/05_shadow_map/shaders
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/images
    COMMAND ${GLSLC} ${CMAKE_SOURCE_DIR}/tutorial/05_shadow_map/shaders/scene.vert  -o ${CMAKE_BINARY_DIR}/tutorial/05_shadow_map/shaders/scene.vert.spv
    COMMAND ${GLSLC} ${CMAKE_SOURCE_DIR}/tutorial/05_shadow_map/shaders/scene.frag  -o ${CMAKE_BINARY_DIR}/tutorial/05_shadow_map/shaders/scene.frag.spv
    COMMAND ${GLSLC} ${CMAKE_SOURCE_DIR}/tutorial/05_shadow_map/shaders/shadow.vert -o ${CMAKE_BINARY_DIR}/tutorial/05_shadow_map/shaders/shadow.vert.spv
    COMMAND ${GLSLC} ${CMAKE_SOURCE_DIR}/tutorial/05_shadow_map/shaders/shadow.frag -o ${CMAKE_BINARY_DIR}/tutorial/05_shadow_map/shaders/shadow.frag.spv
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/images/bkpview_wallpaper2.png
        ${CMAKE_BINARY_DIR}/images/bkpview_wallpaper2.png
    DEPENDS ${CMAKE_SOURCE_DIR}/tutorial/05_shadow_map/shaders/scene.vert
            ${CMAKE_SOURCE_DIR}/tutorial/05_shadow_map/shaders/scene.frag
            ${CMAKE_SOURCE_DIR}/tutorial/05_shadow_map/shaders/shadow.vert
            ${CMAKE_SOURCE_DIR}/tutorial/05_shadow_map/shaders/shadow.frag
            ${CMAKE_SOURCE_DIR}/images/bkpview_wallpaper2.png
)
add_custom_target(tutorial_05_shaders DEPENDS
    ${CMAKE_BINARY_DIR}/tutorial/05_shadow_map/shaders/scene.vert.spv
    ${CMAKE_BINARY_DIR}/tutorial/05_shadow_map/shaders/scene.frag.spv
    ${CMAKE_BINARY_DIR}/tutorial/05_shadow_map/shaders/shadow.vert.spv
    ${CMAKE_BINARY_DIR}/tutorial/05_shadow_map/shaders/shadow.frag.spv
    ${CMAKE_BINARY_DIR}/images/bkpview_wallpaper2.png
)

add_custom_target(copy_gltf_assets ALL
    COMMAND ${CMAKE_COMMAND}
            -DSRC=${CMAKE_SOURCE_DIR}/tutorial/gltf
            -DDST=${CMAKE_BINARY_DIR}/tutorial/gltf
            -P ${CMAKE_SOURCE_DIR}/cmake/copy_gltf.cmake
    COMMENT "Syncing tutorial/gltf to build"
)

add_executable(tutorial_05 tutorial/05_shadow_map/main.c tutorial/common/hud.c tutorial/common/scene.c ${core_src})
add_dependencies(tutorial_05 tutorial_05_shaders copy_gltf_assets)
IF(WIN32)
	target_link_libraries(tutorial_05 glfw3 vulkan-1 winmm)
ELSE()
	target_link_libraries(tutorial_05 pthread glfw vulkan m)
ENDIF(WIN32)

add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/tutorial/06_floor_reflection/shaders/reflect.vert.spv
           ${CMAKE_BINARY_DIR}/tutorial/06_floor_reflection/shaders/scene.vert.spv
           ${CMAKE_BINARY_DIR}/tutorial/06_floor_reflection/shaders/scene.frag.spv
           ${CMAKE_BINARY_DIR}/tutorial/06_floor_reflection/shaders/floor.vert.spv
           ${CMAKE_BINARY_DIR}/tutorial/06_floor_reflection/shaders/floor.frag.spv
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/tutorial/06_floor_reflection/shaders
    COMMAND ${GLSLC} ${CMAKE_SOURCE_DIR}/tutorial/06_floor_reflection/shaders/reflect.vert -o ${CMAKE_BINARY_DIR}/tutorial/06_floor_reflection/shaders/reflect.vert.spv
    COMMAND ${GLSLC} ${CMAKE_SOURCE_DIR}/tutorial/06_floor_reflection/shaders/scene.vert   -o ${CMAKE_BINARY_DIR}/tutorial/06_floor_reflection/shaders/scene.vert.spv
    COMMAND ${GLSLC} ${CMAKE_SOURCE_DIR}/tutorial/06_floor_reflection/shaders/scene.frag   -o ${CMAKE_BINARY_DIR}/tutorial/06_floor_reflection/shaders/scene.frag.spv
    COMMAND ${GLSLC} ${CMAKE_SOURCE_DIR}/tutorial/06_floor_reflection/shaders/floor.vert   -o ${CMAKE_BINARY_DIR}/tutorial/06_floor_reflection/shaders/floor.vert.spv
    COMMAND ${GLSLC} ${CMAKE_SOURCE_DIR}/tutorial/06_floor_reflection/shaders/floor.frag   -o ${CMAKE_BINARY_DIR}/tutorial/06_floor_reflection/shaders/floor.frag.spv
    DEPENDS ${CMAKE_SOURCE_DIR}/tutorial/06_floor_reflection/shaders/reflect.vert
            ${CMAKE_SOURCE_DIR}/tutorial/06_floor_reflection/shaders/scene.vert
            ${CMAKE_SOURCE_DIR}/tutorial/06_floor_reflection/shaders/scene.frag
            ${CMAKE_SOURCE_DIR}/tutorial/06_floor_reflection/shaders/floor.vert
            ${CMAKE_SOURCE_DIR}/tutorial/06_floor_reflection/shaders/floor.frag
)
add_custom_target(tutorial_06_shaders DEPENDS
    ${CMAKE_BINARY_DIR}/tutorial/06_floor_reflection/shaders/reflect.vert.spv
    ${CMAKE_BINARY_DIR}/tutorial/06_floor_reflection/shaders/scene.vert.spv
    ${CMAKE_BINARY_DIR}/tutorial/06_floor_reflection/shaders/scene.frag.spv
    ${CMAKE_BINARY_DIR}/tutorial/06_floor_reflection/shaders/floor.vert.spv
    ${CMAKE_BINARY_DIR}/tutorial/06_floor_reflection/shaders/floor.frag.spv
)

add_executable(tutorial_06 tutorial/06_floor_reflection/main.c tutorial/common/hud.c tutorial/common/scene.c ${core_src})
add_dependencies(tutorial_06 tutorial_06_shaders copy_gltf_assets)
IF(WIN32)
	target_link_libraries(tutorial_06 glfw3 vulkan-1 winmm)
ELSE()
	target_link_libraries(tutorial_06 pthread glfw vulkan m)
ENDIF(WIN32)

add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/tutorial/07_animations/shaders/reflect.vert.spv
           ${CMAKE_BINARY_DIR}/tutorial/07_animations/shaders/scene.vert.spv
           ${CMAKE_BINARY_DIR}/tutorial/07_animations/shaders/scene.frag.spv
           ${CMAKE_BINARY_DIR}/tutorial/07_animations/shaders/floor.vert.spv
           ${CMAKE_BINARY_DIR}/tutorial/07_animations/shaders/floor.frag.spv
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/tutorial/07_animations/shaders
    COMMAND ${GLSLC} ${CMAKE_SOURCE_DIR}/tutorial/07_animations/shaders/reflect.vert -o ${CMAKE_BINARY_DIR}/tutorial/07_animations/shaders/reflect.vert.spv
    COMMAND ${GLSLC} ${CMAKE_SOURCE_DIR}/tutorial/07_animations/shaders/scene.vert   -o ${CMAKE_BINARY_DIR}/tutorial/07_animations/shaders/scene.vert.spv
    COMMAND ${GLSLC} ${CMAKE_SOURCE_DIR}/tutorial/07_animations/shaders/scene.frag   -o ${CMAKE_BINARY_DIR}/tutorial/07_animations/shaders/scene.frag.spv
    COMMAND ${GLSLC} ${CMAKE_SOURCE_DIR}/tutorial/07_animations/shaders/floor.vert   -o ${CMAKE_BINARY_DIR}/tutorial/07_animations/shaders/floor.vert.spv
    COMMAND ${GLSLC} ${CMAKE_SOURCE_DIR}/tutorial/07_animations/shaders/floor.frag   -o ${CMAKE_BINARY_DIR}/tutorial/07_animations/shaders/floor.frag.spv
    DEPENDS ${CMAKE_SOURCE_DIR}/tutorial/07_animations/shaders/reflect.vert
            ${CMAKE_SOURCE_DIR}/tutorial/07_animations/shaders/scene.vert
            ${CMAKE_SOURCE_DIR}/tutorial/07_animations/shaders/scene.frag
            ${CMAKE_SOURCE_DIR}/tutorial/07_animations/shaders/floor.vert
            ${CMAKE_SOURCE_DIR}/tutorial/07_animations/shaders/floor.frag
)
add_custom_target(tutorial_07_shaders DEPENDS
    ${CMAKE_BINARY_DIR}/tutorial/07_animations/shaders/reflect.vert.spv
    ${CMAKE_BINARY_DIR}/tutorial/07_animations/shaders/scene.vert.spv
    ${CMAKE_BINARY_DIR}/tutorial/07_animations/shaders/scene.frag.spv
    ${CMAKE_BINARY_DIR}/tutorial/07_animations/shaders/floor.vert.spv
    ${CMAKE_BINARY_DIR}/tutorial/07_animations/shaders/floor.frag.spv
)

add_executable(tutorial_07 tutorial/07_animations/main.c tutorial/common/hud.c tutorial/common/scene.c ${core_src})
add_dependencies(tutorial_07 tutorial_07_shaders copy_gltf_assets)
IF(WIN32)
	target_link_libraries(tutorial_07 glfw3 vulkan-1 winmm)
ELSE()
	target_link_libraries(tutorial_07 pthread glfw vulkan m)
ENDIF(WIN32)

add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/tutorial/08_deformation/shaders/reflect.vert.spv
           ${CMAKE_BINARY_DIR}/tutorial/08_deformation/shaders/scene.vert.spv
           ${CMAKE_BINARY_DIR}/tutorial/08_deformation/shaders/scene.frag.spv
           ${CMAKE_BINARY_DIR}/tutorial/08_deformation/shaders/floor.vert.spv
           ${CMAKE_BINARY_DIR}/tutorial/08_deformation/shaders/floor.frag.spv
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/tutorial/08_deformation/shaders
    COMMAND ${GLSLC} ${CMAKE_SOURCE_DIR}/tutorial/08_deformation/shaders/reflect.vert -o ${CMAKE_BINARY_DIR}/tutorial/08_deformation/shaders/reflect.vert.spv
    COMMAND ${GLSLC} ${CMAKE_SOURCE_DIR}/tutorial/08_deformation/shaders/scene.vert   -o ${CMAKE_BINARY_DIR}/tutorial/08_deformation/shaders/scene.vert.spv
    COMMAND ${GLSLC} ${CMAKE_SOURCE_DIR}/tutorial/08_deformation/shaders/scene.frag   -o ${CMAKE_BINARY_DIR}/tutorial/08_deformation/shaders/scene.frag.spv
    COMMAND ${GLSLC} ${CMAKE_SOURCE_DIR}/tutorial/08_deformation/shaders/floor.vert   -o ${CMAKE_BINARY_DIR}/tutorial/08_deformation/shaders/floor.vert.spv
    COMMAND ${GLSLC} ${CMAKE_SOURCE_DIR}/tutorial/08_deformation/shaders/floor.frag   -o ${CMAKE_BINARY_DIR}/tutorial/08_deformation/shaders/floor.frag.spv
    DEPENDS ${CMAKE_SOURCE_DIR}/tutorial/08_deformation/shaders/reflect.vert
            ${CMAKE_SOURCE_DIR}/tutorial/08_deformation/shaders/scene.vert
            ${CMAKE_SOURCE_DIR}/tutorial/08_deformation/shaders/scene.frag
            ${CMAKE_SOURCE_DIR}/tutorial/08_deformation/shaders/floor.vert
            ${CMAKE_SOURCE_DIR}/tutorial/08_deformation/shaders/floor.frag
)
add_custom_target(tutorial_08_shaders DEPENDS
    ${CMAKE_BINARY_DIR}/tutorial/08_deformation/shaders/reflect.vert.spv
    ${CMAKE_BINARY_DIR}/tutorial/08_deformation/shaders/scene.vert.spv
    ${CMAKE_BINARY_DIR}/tutorial/08_deformation/shaders/scene.frag.spv
    ${CMAKE_BINARY_DIR}/tutorial/08_deformation/shaders/floor.vert.spv
    ${CMAKE_BINARY_DIR}/tutorial/08_deformation/shaders/floor.frag.spv
)

add_executable(tutorial_08 tutorial/08_deformation/main.c tutorial/common/hud.c tutorial/common/scene.c ${core_src})
add_dependencies(tutorial_08 tutorial_08_shaders copy_gltf_assets)
IF(WIN32)
	target_link_libraries(tutorial_08 glfw3 vulkan-1 winmm)
ELSE()
	target_link_libraries(tutorial_08 pthread glfw vulkan m)
ENDIF(WIN32)

add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/tutorial/09_destruction/shaders/reflect.vert.spv
           ${CMAKE_BINARY_DIR}/tutorial/09_destruction/shaders/scene.vert.spv
           ${CMAKE_BINARY_DIR}/tutorial/09_destruction/shaders/scene.frag.spv
           ${CMAKE_BINARY_DIR}/tutorial/09_destruction/shaders/floor.vert.spv
           ${CMAKE_BINARY_DIR}/tutorial/09_destruction/shaders/floor.frag.spv
           ${CMAKE_BINARY_DIR}/tutorial/09_destruction/shaders/explosion.comp.spv
           ${CMAKE_BINARY_DIR}/tutorial/09_destruction/shaders/particle.vert.spv
           ${CMAKE_BINARY_DIR}/tutorial/09_destruction/shaders/particle.frag.spv
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/tutorial/09_destruction/shaders
    COMMAND ${GLSLC} ${CMAKE_SOURCE_DIR}/tutorial/09_destruction/shaders/reflect.vert   -o ${CMAKE_BINARY_DIR}/tutorial/09_destruction/shaders/reflect.vert.spv
    COMMAND ${GLSLC} ${CMAKE_SOURCE_DIR}/tutorial/09_destruction/shaders/scene.vert     -o ${CMAKE_BINARY_DIR}/tutorial/09_destruction/shaders/scene.vert.spv
    COMMAND ${GLSLC} ${CMAKE_SOURCE_DIR}/tutorial/09_destruction/shaders/scene.frag     -o ${CMAKE_BINARY_DIR}/tutorial/09_destruction/shaders/scene.frag.spv
    COMMAND ${GLSLC} ${CMAKE_SOURCE_DIR}/tutorial/09_destruction/shaders/floor.vert     -o ${CMAKE_BINARY_DIR}/tutorial/09_destruction/shaders/floor.vert.spv
    COMMAND ${GLSLC} ${CMAKE_SOURCE_DIR}/tutorial/09_destruction/shaders/floor.frag     -o ${CMAKE_BINARY_DIR}/tutorial/09_destruction/shaders/floor.frag.spv
    COMMAND ${GLSLC} ${CMAKE_SOURCE_DIR}/tutorial/09_destruction/shaders/explosion.comp -o ${CMAKE_BINARY_DIR}/tutorial/09_destruction/shaders/explosion.comp.spv
    COMMAND ${GLSLC} ${CMAKE_SOURCE_DIR}/tutorial/09_destruction/shaders/particle.vert  -o ${CMAKE_BINARY_DIR}/tutorial/09_destruction/shaders/particle.vert.spv
    COMMAND ${GLSLC} ${CMAKE_SOURCE_DIR}/tutorial/09_destruction/shaders/particle.frag  -o ${CMAKE_BINARY_DIR}/tutorial/09_destruction/shaders/particle.frag.spv
    DEPENDS ${CMAKE_SOURCE_DIR}/tutorial/09_destruction/shaders/reflect.vert
            ${CMAKE_SOURCE_DIR}/tutorial/09_destruction/shaders/scene.vert
            ${CMAKE_SOURCE_DIR}/tutorial/09_destruction/shaders/scene.frag
            ${CMAKE_SOURCE_DIR}/tutorial/09_destruction/shaders/floor.vert
            ${CMAKE_SOURCE_DIR}/tutorial/09_destruction/shaders/floor.frag
            ${CMAKE_SOURCE_DIR}/tutorial/09_destruction/shaders/explosion.comp
            ${CMAKE_SOURCE_DIR}/tutorial/09_destruction/shaders/particle.vert
            ${CMAKE_SOURCE_DIR}/tutorial/09_destruction/shaders/particle.frag
)
add_custom_target(tutorial_09_shaders DEPENDS
    ${CMAKE_BINARY_DIR}/tutorial/09_destruction/shaders/reflect.vert.spv
    ${CMAKE_BINARY_DIR}/tutorial/09_destruction/shaders/scene.vert.spv
    ${CMAKE_BINARY_DIR}/tutorial/09_destruction/shaders/scene.frag.spv
    ${CMAKE_BINARY_DIR}/tutorial/09_destruction/shaders/floor.vert.spv
    ${CMAKE_BINARY_DIR}/tutorial/09_destruction/shaders/floor.frag.spv
    ${CMAKE_BINARY_DIR}/tutorial/09_destruction/shaders/explosion.comp.spv
    ${CMAKE_BINARY_DIR}/tutorial/09_destruction/shaders/particle.vert.spv
    ${CMAKE_BINARY_DIR}/tutorial/09_destruction/shaders/particle.frag.spv
)

add_executable(tutorial_09 tutorial/09_destruction/main.c tutorial/common/hud.c tutorial/common/scene.c ${core_src})
add_dependencies(tutorial_09 tutorial_09_shaders copy_gltf_assets)
IF(WIN32)
	target_link_libraries(tutorial_09 glfw3 vulkan-1 winmm)
ELSE()
	target_link_libraries(tutorial_09 pthread glfw vulkan m)
ENDIF(WIN32)

