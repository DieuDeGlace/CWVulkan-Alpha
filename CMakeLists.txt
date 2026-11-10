cmake_minimum_required(VERSION 3.20)
project(AlphaVulkan C CXX)

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "AlphaVulkan requires a 64-bit toolchain. The Alpha extractor is built separately.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Every dependency is fetched as source at a pinned version (the same pins as CubeVulkan), so
# no Vulkan SDK is required: volk loads the system vulkan-1.dll and glslang is built here to
# compile the shaders.
include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

FetchContent_Declare(vulkan_headers GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Headers.git
                     GIT_TAG v1.4.304 GIT_SHALLOW TRUE SOURCE_SUBDIR none)
FetchContent_Declare(volk GIT_REPOSITORY https://github.com/zeux/volk.git
                     GIT_TAG 1.4.304 GIT_SHALLOW TRUE SOURCE_SUBDIR none)
FetchContent_Declare(glm GIT_REPOSITORY https://github.com/g-truc/glm.git
                     GIT_TAG 1.0.1 GIT_SHALLOW TRUE SOURCE_SUBDIR none)
FetchContent_Declare(miniaudio GIT_REPOSITORY https://github.com/mackron/miniaudio.git
                     GIT_TAG 0.11.21 GIT_SHALLOW TRUE SOURCE_SUBDIR none)
FetchContent_Declare(stb GIT_REPOSITORY https://github.com/nothings/stb.git
                     GIT_TAG a7d4c2175630e1ea5b6f8dd00ab96fed0b97b96d SOURCE_SUBDIR none)
FetchContent_Declare(glfw GIT_REPOSITORY https://github.com/glfw/glfw.git
                     GIT_TAG 3.4 GIT_SHALLOW TRUE)
FetchContent_Declare(glslang GIT_REPOSITORY https://github.com/KhronosGroup/glslang.git
                     GIT_TAG 15.0.0 GIT_SHALLOW TRUE)
# SQLite (public domain), read-only: Alpha keeps its content in SQLite stores (data1..4.db).
FetchContent_Declare(sqlite URL https://www.sqlite.org/2024/sqlite-amalgamation-3460100.zip
                     URL_HASH SHA256=77823cb110929c2bcb0f5d48e4833b5c59a8a6e40cdea3936b99e199dbbe5784
                     DOWNLOAD_EXTRACT_TIMESTAMP TRUE SOURCE_SUBDIR none)
# zlib (zlib License): Alpha compresses its character saves and network updates with it.
FetchContent_Declare(zlib URL https://github.com/madler/zlib/releases/download/v1.3.1/zlib-1.3.1.tar.gz
                     URL_HASH SHA256=9a93b2b7dfdac77ceba5a558a580e74667dd6fede4585b91eefb60f03b72df23
                     DOWNLOAD_EXTRACT_TIMESTAMP TRUE SOURCE_SUBDIR none)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
set(ENABLE_OPT OFF CACHE BOOL "" FORCE)
set(ENABLE_HLSL OFF CACHE BOOL "" FORCE)
set(ENABLE_SPVREMAPPER OFF CACHE BOOL "" FORCE)
set(ENABLE_CTEST OFF CACHE BOOL "" FORCE)
set(GLSLANG_TESTS OFF CACHE BOOL "" FORCE)
set(GLSLANG_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
set(ENABLE_GLSLANG_BINARIES OFF CACHE BOOL "" FORCE)
set(BUILD_EXTERNAL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(vulkan_headers volk glm miniaudio stb glfw glslang sqlite zlib)

add_executable(spirv_compile tools/spirv_compile.cpp)
target_link_libraries(spirv_compile PRIVATE glslang glslang-default-resource-limits)

# Shaders: GLSL -> SPIR-V headers (uint32_t arrays) in the build tree.
set(SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
set(SHADER_OUT ${CMAKE_CURRENT_BINARY_DIR}/shaders)
file(MAKE_DIRECTORY ${SHADER_OUT})
set(SHADER_HEADERS)
foreach(shader ui.vert ui.frag model.vert model.frag water.frag shadow.vert cloud.vert cloud.frag sky.vert sky.frag sprite.vert sprite.frag effect.vert effect.frag minimap.vert minimap.frag item.vert post.vert copy.frag taa.frag
        smaa_edge.vert smaa_edge.frag smaa_weight.vert smaa_weight.frag smaa_blend.vert smaa_blend.frag)
    string(REPLACE "." "_" symbol ${shader})
    set(out ${SHADER_OUT}/${symbol}.h)
    add_custom_command(OUTPUT ${out}
        COMMAND spirv_compile ${SHADER_DIR}/${shader} ${out} spv_${symbol} ${SHADER_DIR}
        DEPENDS ${SHADER_DIR}/${shader} ${SHADER_DIR}/frame.glsl ${SHADER_DIR}/smaa_common.glsl
                ${SHADER_DIR}/water_surface.glsl
                ${CMAKE_CURRENT_SOURCE_DIR}/third_party/smaa/SMAA.hlsl spirv_compile
        COMMENT "Compiling ${shader}")
    list(APPEND SHADER_HEADERS ${out})
endforeach()
add_custom_target(shaders DEPENDS ${SHADER_HEADERS})
# The quest icons (assets/quests, the owner's MIT-licensed picture), built into the program.
set(QUEST_ICONS ${CMAKE_CURRENT_SOURCE_DIR}/assets/quests/quest-icons.png)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${QUEST_ICONS})
file(READ ${QUEST_ICONS} QUEST_ICONS_HEX HEX)
string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," QUEST_ICONS_BYTES "${QUEST_ICONS_HEX}")
file(CONFIGURE OUTPUT ${SHADER_OUT}/quest_icons.h
    CONTENT "static const unsigned char QUEST_ICONS_PNG[] = {${QUEST_ICONS_BYTES}};\n" @ONLY)

set(ZLIB_SOURCES)
foreach(file adler32 compress crc32 deflate infback inffast inflate inftrees trees uncompr zutil)
    list(APPEND ZLIB_SOURCES ${zlib_SOURCE_DIR}/${file}.c)
endforeach()
if(MSVC)
    set_source_files_properties(${ZLIB_SOURCES} PROPERTIES COMPILE_OPTIONS /w) # (third-party code, built as it comes)
endif()
add_library(deps STATIC
    ${volk_SOURCE_DIR}/volk.c
    ${sqlite_SOURCE_DIR}/sqlite3.c
    ${ZLIB_SOURCES}
    src/third_party.c)
target_include_directories(deps PUBLIC
    ${vulkan_headers_SOURCE_DIR}/include ${volk_SOURCE_DIR} ${glm_SOURCE_DIR}
    ${miniaudio_SOURCE_DIR} ${stb_SOURCE_DIR} ${sqlite_SOURCE_DIR} ${zlib_SOURCE_DIR})
target_compile_definitions(deps PUBLIC VK_NO_PROTOTYPES GLM_FORCE_DEPTH_ZERO_TO_ONE
    GLM_FORCE_RADIANS NOMINMAX WIN32_LEAN_AND_MEAN)

# The engine core is a library so the tests link the same code as the game.
add_library(engine STATIC
    src/Config.cpp
    src/Install.cpp
    src/FileHash.cpp
    src/OwnerContent.cpp
    src/GenerationProfile.cpp
    src/DataStore.cpp
    src/CubModel.cpp
    src/AlphaLook.cpp
    src/TerrainLights.cpp
    src/SectorLights.cpp
    src/CreatureLights.cpp
    src/CachedLights.cpp
    src/Audio.cpp
    src/Dictionary.cpp
    src/ItemNames.cpp
    src/WorldItems.cpp
    src/StaticStates.cpp
    src/BlockEdits.cpp
    src/Missions.cpp
    src/Quests.cpp
    src/Loot.cpp
    src/Speech.cpp
    src/NewCharacter.cpp
    src/GameOptions.cpp
    src/Frame.cpp
    src/Shadows.cpp
    src/Motion.cpp
    src/Moon.cpp
    src/Mods.cpp
    src/ModFingerprint.cpp
    src/EngineModules.cpp
    src/worldgen/Random.cpp
    src/worldgen/NoiseMath.cpp
    src/worldgen/Regions.cpp
    src/worldgen/Locations.cpp
    src/worldgen/Terrain.cpp
    src/worldgen/PostPasses.cpp
    src/worldgen/Structures.cpp
    src/worldgen/Builders.cpp
    src/worldgen/Dungeon.cpp
    src/worldgen/Town.cpp
    src/worldgen/Prefab.cpp
    src/worldgen/Items.cpp
    src/worldgen/LocationCreatures.cpp
    src/worldgen/Wildlife.cpp
    src/worldgen/MapTile.cpp
    src/creatures/CreatureTables.cpp
    src/creatures/Appearance.cpp
    src/creatures/Pose.cpp
    src/creatures/Particles.cpp
    src/creatures/Physics.cpp
    src/creatures/FollowCamera.cpp
    src/creatures/Combat.cpp
    src/creatures/Hits.cpp
    src/creatures/Buffs.cpp
    src/creatures/Relationships.cpp
    src/creatures/Credits.cpp
    src/creatures/Strike.cpp
    src/creatures/Inventory.cpp
    src/creatures/FatalRecipients.cpp
    src/creatures/Crafting.cpp
    src/creatures/Skills.cpp
    src/creatures/Spirits.cpp
    src/creatures/Projectiles.cpp
    src/creatures/Abilities.cpp
    src/creatures/Npcs.cpp
    src/creatures/Pets.cpp
    src/creatures/Paths.cpp
    src/creatures/Names.cpp
    src/worldgen/Zone.cpp
    src/worldgen/WorldMap.cpp
    src/worldgen/EdgeLight.cpp
    src/worldgen/WaterFeatures.cpp
    src/worldgen/Clouds.cpp
    src/worldgen/Trees.cpp
    src/ChunkMesher.cpp
    src/ZlibStream.cpp
    src/Saves.cpp
    src/WorldSession.cpp
    src/ui/Plasma.cpp
    src/ui/Font.cpp
    src/ui/Painter.cpp
    src/ui/Screens.cpp
    src/ui/LandNames.cpp
    src/ui/Interaction.cpp
    src/ui/InventoryScreen.cpp
    src/ui/CraftingScreen.cpp
    src/ui/SpeechBubbles.cpp
    src/ui/VendorScreen.cpp
    src/ui/ServicesScreen.cpp
    src/ui/SkillScreen.cpp
    src/ui/FrontScreens.cpp
    src/ui/BenchScreen.cpp
    src/net/Protocol.cpp
    src/net/RemoteFighter.cpp
    src/net/Socket.cpp
    src/net/Server.cpp
    src/net/Client.cpp)
target_include_directories(engine PUBLIC src sdk)
target_include_directories(engine PRIVATE ${SHADER_OUT}) # quest_icons.h
target_link_libraries(engine PUBLIC deps ws2_32 bcrypt)

# Source modules are part of the engine library, including the dedicated server.
# They remain inactive until named in [Mods] Load. Explicit registry references
# keep static-library dead stripping from silently dropping a module.
function(alpha_add_engine_module id)
    cmake_parse_arguments(module "" "VERSION" "" ${ARGN})
    if(NOT module_VERSION)
        set(module_VERSION "1")
    endif()
    if(NOT module_VERSION MATCHES "^[A-Za-z0-9_.+-]+$")
        message(FATAL_ERROR "Invalid engine module version: ${module_VERSION}")
    endif()
    if(NOT id MATCHES "^[A-Za-z][A-Za-z0-9_]*$")
        message(FATAL_ERROR "Invalid engine module ID: ${id}")
    endif()
    get_property(ids GLOBAL PROPERTY ALPHA_MODULE_IDS)
    if(id IN_LIST ids)
        message(FATAL_ERROR "Duplicate engine module ID: ${id}")
    endif()
    add_library(alpha_module_${id} OBJECT ${module_UNPARSED_ARGUMENTS})
    target_include_directories(alpha_module_${id} PRIVATE ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/sdk)
    target_compile_definitions(alpha_module_${id} PRIVATE AV_MODULE_ENTRY=avBuiltin_${id})
    target_link_libraries(alpha_module_${id} PRIVATE deps)
    if(MSVC)
        target_compile_options(alpha_module_${id} PRIVATE /W4 /permissive-)
    endif()
    target_sources(engine PRIVATE $<TARGET_OBJECTS:alpha_module_${id}>)
    set_property(GLOBAL APPEND PROPERTY ALPHA_MODULE_IDS ${id})
    set_property(GLOBAL PROPERTY ALPHA_MODULE_VERSION_${id} "${module_VERSION}")
    # Fingerprint declared inputs without checkout-specific absolute paths.
    # Listing headers here also makes Ninja reconfigure when they change.
    set(moduleInputs ${module_UNPARSED_ARGUMENTS}
        "${PROJECT_SOURCE_DIR}/sdk/alphavulkan_mod.h"
        "${PROJECT_SOURCE_DIR}/sdk/alphavulkan_engine_module.h")
    set(moduleManifest "AlphaVulkan source inputs 1\n${CMAKE_CXX_COMPILER_ID}\n${CMAKE_CXX_COMPILER_VERSION}\n")
    foreach(input IN LISTS moduleInputs)
        get_filename_component(inputPath "${input}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
        if(NOT EXISTS "${inputPath}" OR IS_DIRECTORY "${inputPath}")
            message(FATAL_ERROR "Module ${id} requires an existing source/header input: ${input}")
        endif()
        set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${inputPath}")
        get_filename_component(inputName "${inputPath}" NAME)
        file(READ "${inputPath}" inputText)
        string(REPLACE "\r\n" "\n" inputText "${inputText}")
        string(SHA256 inputHash "${inputText}")
        string(LENGTH "${inputName}" inputNameLength)
        string(APPEND moduleManifest "${inputNameLength}:${inputName}${inputHash}\n")
    endforeach()
    string(SHA256 moduleFingerprint "${moduleManifest}")
    set_property(GLOBAL PROPERTY ALPHA_MODULE_FINGERPRINT_${id} "${moduleFingerprint}")
endfunction()

option(ALPHAVULKAN_EXAMPLE_MODULE "Compile the opt-in StoneTint source-module example" ON)
if(ALPHAVULKAN_EXAMPLE_MODULE)
    alpha_add_engine_module(StoneTint examples/modules/StoneTint/StoneTint.cpp)
endif()
set(ALPHAVULKAN_MODULE_DIRS "" CACHE STRING "Semicolon-separated source-module CMake directories")
set(moduleIndex 0)
foreach(moduleDir IN LISTS ALPHAVULKAN_MODULE_DIRS)
    math(EXPR moduleIndex "${moduleIndex}+1")
    add_subdirectory("${moduleDir}" "source-modules/${moduleIndex}")
endforeach()
get_property(moduleIds GLOBAL PROPERTY ALPHA_MODULE_IDS)
list(LENGTH moduleIds moduleCount)
set(moduleRegistry "#include \"BuiltinModules.h\"\n#include <array>\n")
foreach(id IN LISTS moduleIds)
    string(APPEND moduleRegistry "extern \"C\" int avBuiltin_${id}(const AvHost *, AvMod *);\n")
endforeach()
string(APPEND moduleRegistry "std::span<const BuiltinModule> builtinModules() {\n    static const std::array<BuiltinModule, ${moduleCount}> modules{{\n")
foreach(id IN LISTS moduleIds)
    get_property(moduleVersion GLOBAL PROPERTY ALPHA_MODULE_VERSION_${id})
    get_property(moduleFingerprint GLOBAL PROPERTY ALPHA_MODULE_FINGERPRINT_${id})
    string(APPEND moduleRegistry "        {\"${id}\", avBuiltin_${id}, \"${moduleVersion}\", \"${moduleFingerprint}\"},\n")
endforeach()
string(APPEND moduleRegistry "    }};\n    return modules;\n}\n")
file(CONFIGURE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/BuiltinModules.cpp" CONTENT "${moduleRegistry}" @ONLY)
target_sources(engine PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/BuiltinModules.cpp")

add_library(renderer STATIC src/Renderer.cpp src/RendererWater.cpp src/RendererPost.cpp src/RendererUi.cpp)
add_dependencies(renderer shaders)
target_include_directories(renderer PRIVATE ${SHADER_OUT})
target_link_libraries(renderer PUBLIC engine glfw)

add_executable(AlphaVulkan src/Main.cpp src/Gamepad.cpp src/WorldView.cpp src/CreatureView.cpp src/Player.cpp)
target_link_libraries(AlphaVulkan PRIVATE renderer)

# Explicit GPU diagnostic, using only procedural geometry and synthetic lights.
# Kept out of automatic CTest and player packages (requires a Vulkan window).
add_executable(TerrainLightRenderProbe tools/TerrainLightRenderProbe.cpp)
target_link_libraries(TerrainLightRenderProbe PRIVATE renderer)

add_executable(StoreTests tests/StoreTests.cpp)
target_link_libraries(StoreTests PRIVATE engine)
# The example code mod (examples/mods/TimeLapse), built as a mod folder under build/Mods.
add_library(TimeLapse SHARED examples/mods/TimeLapse/TimeLapse.c)
target_include_directories(TimeLapse PRIVATE sdk)
set_target_properties(TimeLapse PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Mods/TimeLapse
                                           LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Mods/TimeLapse)
configure_file(examples/mods/TimeLapse/mod.ini ${CMAKE_CURRENT_BINARY_DIR}/Mods/TimeLapse/mod.ini COPYONLY)

add_executable(ModTests tests/ModTests.cpp)
add_library(ModStartupProbe SHARED tests/ModStartupProbe.c)
target_include_directories(ModStartupProbe PRIVATE sdk)
set_target_properties(ModStartupProbe PROPERTIES PREFIX "" RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test-plugins")
add_dependencies(ModTests ModStartupProbe)
target_link_libraries(ModTests PRIVATE engine)
add_dependencies(ModTests TimeLapse)
add_executable(WorldgenTests tests/WorldgenTests.cpp)
target_link_libraries(WorldgenTests PRIVATE engine)
add_executable(SpatialScalarTests tests/SpatialScalarTests.cpp)
target_link_libraries(SpatialScalarTests PRIVATE engine)
add_executable(TerrainLightTests tests/TerrainLightTests.cpp)
target_link_libraries(TerrainLightTests PRIVATE engine)
add_executable(SectorLightTests tests/SectorLightTests.cpp)
target_link_libraries(SectorLightTests PRIVATE engine)
add_executable(CreatureLightTests tests/CreatureLightTests.cpp)
target_link_libraries(CreatureLightTests PRIVATE engine)
add_executable(CachedLightTests tests/CachedLightTests.cpp)
target_link_libraries(CachedLightTests PRIVATE engine)
add_executable(RemoteFighterTests tests/RemoteFighterTests.cpp)
target_link_libraries(RemoteFighterTests PRIVATE engine)
# The dedicated server.
add_executable(AlphaServer src/ServerMain.cpp)
target_link_libraries(AlphaServer PRIVATE engine)

add_executable(NetTests tests/NetTests.cpp)
target_link_libraries(NetTests PRIVATE engine)
add_executable(SaveTests tests/SaveTests.cpp)
target_link_libraries(SaveTests PRIVATE engine)
add_executable(ContentTests tests/ContentTests.cpp)
target_link_libraries(ContentTests PRIVATE engine)
add_executable(LookTests tests/LookTests.cpp)
add_executable(UiTests tests/UiTests.cpp)
target_link_libraries(UiTests PRIVATE engine)
add_executable(CreatureTests tests/CreatureTests.cpp)
target_link_libraries(CreatureTests PRIVATE engine)
add_executable(PlxDump tools/PlxDump.cpp)
target_link_libraries(PlxDump PRIVATE engine)
add_executable(ZoneDump tools/ZoneDump.cpp)
target_link_libraries(ZoneDump PRIVATE engine)
add_executable(ZoneCompare tools/ZoneCompare.cpp)
target_link_libraries(ZoneCompare PRIVATE engine)
add_executable(CreatureCompare tools/CreatureCompare.cpp)
target_link_libraries(CreatureCompare PRIVATE engine)
target_link_libraries(LookTests PRIVATE engine)

if(MSVC)
    set_source_files_properties(src/worldgen/NoiseMath.cpp src/creatures/Combat.cpp src/creatures/Strike.cpp
        src/creatures/FatalRecipients.cpp
        src/creatures/Projectiles.cpp
        src/creatures/Physics.cpp
        src/AlphaLook.cpp
        src/worldgen/Regions.cpp src/worldgen/Random.cpp
        src/TerrainLights.cpp
        src/SectorLights.cpp
        src/CreatureLights.cpp
        src/CachedLights.cpp
        src/creatures/Spirits.cpp
        src/Frame.cpp
        src/Missions.cpp
        tests/SpatialScalarTests.cpp
        PROPERTIES COMPILE_OPTIONS /fp:strict)
    target_compile_options(engine PRIVATE /W4 /permissive-)
    target_compile_options(renderer PRIVATE /W4 /permissive-)
    target_compile_options(AlphaVulkan PRIVATE /W4 /permissive-)
endif()
add_executable(DictDump tools/DictDump.cpp)
target_link_libraries(DictDump PRIVATE engine)
add_executable(MakeCharacter tools/MakeCharacter.cpp)
target_link_libraries(MakeCharacter PRIVATE engine)
add_executable(AlphaSetup tools/AlphaSetup.cpp)
target_link_libraries(AlphaSetup PRIVATE engine)
add_executable(OwnerContentTests tests/OwnerContentTests.cpp)
target_link_libraries(OwnerContentTests PRIVATE engine)
add_executable(EngineModuleTests tests/EngineModuleTests.cpp)
target_link_libraries(EngineModuleTests PRIVATE engine)
add_executable(GenerationProfileTests tests/GenerationProfileTests.cpp)
target_link_libraries(GenerationProfileTests PRIVATE engine)
add_executable(NoiseMathTests tests/NoiseMathTests.cpp)
target_link_libraries(NoiseMathTests PRIVATE engine)
add_executable(ActionPhaseTests tests/ActionPhaseTests.cpp)
target_link_libraries(ActionPhaseTests PRIVATE engine)
add_executable(BuffTests tests/BuffTests.cpp)
target_link_libraries(BuffTests PRIVATE engine)
add_executable(HitRecordTests tests/HitRecordTests.cpp)
target_link_libraries(HitRecordTests PRIVATE engine)
add_executable(HitApplicationTests tests/HitApplicationTests.cpp)
target_link_libraries(HitApplicationTests PRIVATE engine)
add_executable(PeriodicHitTests tests/PeriodicHitTests.cpp)
target_link_libraries(PeriodicHitTests PRIVATE engine)
add_executable(SimulationFrameTests tests/SimulationFrameTests.cpp)
target_link_libraries(SimulationFrameTests PRIVATE engine)
add_executable(ActorOrderTests tests/ActorOrderTests.cpp)
target_link_libraries(ActorOrderTests PRIVATE engine)
add_executable(WorldHitDeliveryTests tests/WorldHitDeliveryTests.cpp)
target_link_libraries(WorldHitDeliveryTests PRIVATE engine)
add_executable(RelationshipTests tests/RelationshipTests.cpp)
target_link_libraries(RelationshipTests PRIVATE engine)
add_executable(FatalContextTests tests/FatalContextTests.cpp)
target_link_libraries(FatalContextTests PRIVATE engine)
add_executable(FatalLootTests tests/FatalLootTests.cpp)
target_link_libraries(FatalLootTests PRIVATE engine)
add_executable(FatalObjectTests tests/FatalObjectTests.cpp)
target_link_libraries(FatalObjectTests PRIVATE engine)
add_executable(CreditTests tests/CreditTests.cpp)
target_link_libraries(CreditTests PRIVATE engine)
add_executable(RegionProgressTests tests/RegionProgressTests.cpp)
target_link_libraries(RegionProgressTests PRIVATE engine)
add_executable(ItemIdentityTests tests/ItemIdentityTests.cpp)
target_link_libraries(ItemIdentityTests PRIVATE engine)
add_executable(LinkedItemTests tests/LinkedItemTests.cpp)
target_link_libraries(LinkedItemTests PRIVATE engine)
add_executable(FatalRecipientTests tests/FatalRecipientTests.cpp)
target_link_libraries(FatalRecipientTests PRIVATE engine)
add_executable(SourceAuxiliaryTests tests/SourceAuxiliaryTests.cpp)
target_link_libraries(SourceAuxiliaryTests PRIVATE engine)
add_executable(IncomingHitTests tests/IncomingHitTests.cpp)
target_link_libraries(IncomingHitTests PRIVATE engine)
add_executable(LightingTests tests/LightingTests.cpp)
target_link_libraries(LightingTests PRIVATE engine)
add_executable(WaterTests tests/WaterTests.cpp)
target_link_libraries(WaterTests PRIVATE engine)
add_executable(RangedSchedulingTests tests/RangedSchedulingTests.cpp)
target_link_libraries(RangedSchedulingTests PRIVATE engine)

# Run the existing checks against the same libraries used by the client/server.
# Owner-content checks accept an explicit installation and otherwise use discovery.
include(CTest)
set(ALPHAVULKAN_TEST_GAME_PATH "" CACHE PATH "Owned Alpha installation used by content/save/UI checks")
if(BUILD_TESTING)
    foreach(testTarget StoreTests ModTests WorldgenTests NetTests SaveTests ContentTests LookTests UiTests CreatureTests LightingTests WaterTests RangedSchedulingTests OwnerContentTests EngineModuleTests GenerationProfileTests NoiseMathTests ActionPhaseTests BuffTests HitRecordTests HitApplicationTests PeriodicHitTests SimulationFrameTests ActorOrderTests WorldHitDeliveryTests RelationshipTests FatalContextTests FatalLootTests FatalObjectTests CreditTests RegionProgressTests ItemIdentityTests LinkedItemTests FatalRecipientTests SourceAuxiliaryTests IncomingHitTests SpatialScalarTests TerrainLightTests SectorLightTests CreatureLightTests CachedLightTests RemoteFighterTests)
        set(testArguments)
        if(ALPHAVULKAN_TEST_GAME_PATH AND testTarget MATCHES "^(Store|Save|Content|Ui|OwnerContent)Tests$")
            list(APPEND testArguments "${ALPHAVULKAN_TEST_GAME_PATH}")
        endif()
        add_test(NAME ${testTarget} COMMAND ${testTarget} ${testArguments})
        set_tests_properties(${testTarget} PROPERTIES
            WORKING_DIRECTORY "$<TARGET_FILE_DIR:${testTarget}>"
            TIMEOUT 300)
    endforeach()
    add_test(NAME ZoneCompareTests COMMAND powershell -NoProfile -ExecutionPolicy Bypass
        -File "${CMAKE_CURRENT_SOURCE_DIR}/tests/ZoneCompareTests.ps1"
        -ZoneCompare "$<TARGET_FILE:ZoneCompare>")
    set_tests_properties(ZoneCompareTests PROPERTIES
        WORKING_DIRECTORY "$<TARGET_FILE_DIR:ZoneCompare>" TIMEOUT 300)
endif()
