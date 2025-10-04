set (MAINSRC ${CMAKE_CURRENT_SOURCE_DIR}/../Source)

# Helper function to create a test
function(add_unit_test TEST_NAME)
    # Usage 1 (legacy): add_unit_test(NAME src1 src2 ...)
    # Usage 2: add_unit_test(NAME SOURCES s1 s2 ... INCLUDE_DIRS d1 d2 ... COMPILE_DEFINITIONS DEF1 DEF2 ...)
    set(options)
    set(oneValueArgs)
    set(multiValueArgs SOURCES INCLUDE_DIRS COMPILE_DEFINITIONS INCLUDE_LIBS)
    cmake_parse_arguments(UT "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(TEST_SOURCES ${UT_SOURCES})

    set(TEST_TARGET "test_${TEST_NAME}")
    add_executable(${TEST_TARGET} ${TEST_SOURCES})

    # Base include dirs + optionnels
    target_include_directories(${TEST_TARGET} PRIVATE ${UT_INCLUDE_DIRS})

    if (UT_COMPILE_DEFINITIONS)
        target_compile_definitions(${TEST_TARGET} PRIVATE ${UT_COMPILE_DEFINITIONS})
    endif ()

    target_link_libraries(${TEST_TARGET} PRIVATE Catch2::Catch2WithMain ${UT_INCLUDE_LIBS})
    add_test(NAME ${TEST_TARGET} COMMAND ${TEST_TARGET} WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endfunction()

add_subdirectory (ThirdParty/catch2)

add_unit_test(
    "SplinePath2D"
    SOURCES
        Components/splinepath2d_test.cpp
        ${MAINSRC}/Components/SplinePath2D.cpp
    INCLUDE_DIRS
        ${URHO3D_INCLUDE_DIRS}
        ${MAINSRC}/Components
    INCLUDE_LIBS
        Urho3D
)

# Tests de Matches (logique de base)
add_unit_test(
    "Matches"
    SOURCES
        Managers/test_matches.cpp
        ${MAINSRC}/Objects/DelayAction.cpp
        ${MAINSRC}/ObjectsCore/DelayInformer.cpp
        ${MAINSRC}/ObjectsCore/TimerSimple.cpp
        ${MAINSRC}/ObjectsCore/TimerRemover.cpp
        ${MAINSRC}/ObjectsCore/TextMessage.cpp
        ${MAINSRC}/GameAttributes.cpp
        ${MAINSRC}/GameStatics.cpp
        ${MAINSRC}/GameHelpers.cpp
        ${MAINSRC}/GameRand.cpp
        ${MAINSRC}/GameUI.cpp
        ${MAINSRC}/GameStateManager.cpp
        ${MAINSRC}/Graphics2D/AnimatedSprite.cpp
        ${MAINSRC}/Graphics2D/AnimationSet2D.cpp            
        ${MAINSRC}/Graphics2D/AnimatedSprite2D.cpp
        ${MAINSRC}/Graphics2D/Drawable2D.cpp        
        ${MAINSRC}/Graphics2D/Graphics2D.cpp
        ${MAINSRC}/Graphics2D/ParticleEffect2D.cpp
        ${MAINSRC}/Graphics2D/ParticleEmitter2D.cpp
        ${MAINSRC}/Graphics2D/Renderer2D.cpp
        ${MAINSRC}/Graphics2D/Sprite2D.cpp
        ${MAINSRC}/Graphics2D/SpriterData2D.cpp
        ${MAINSRC}/Graphics2D/SpriterInstance2D.cpp
        ${MAINSRC}/Graphics2D/SpriteSheet2D.cpp
        ${MAINSRC}/Graphics2D/StaticSprite2D.cpp
        ${MAINSRC}/Graphics2D/Text2D.cpp
        ${MAINSRC}/Managers/Matches.cpp
    INCLUDE_DIRS
        ${URHO3D_INCLUDE_DIRS}
        ${MAINSRC}
        ${MAINSRC}/Managers
        ${MAINSRC}/ObjectsCore
        ${MAINSRC}/Objects
        ${MAINSRC}/Graphics2D
    COMPILE_DEFINITIONS
        ACTIVE_CUSTOM_URHO
        ACTIVE_TESTMODE
        TESTMODE
        TESTMODE_STARTZONE=1
    INCLUDE_LIBS
        Urho3D
)

# add_unit_test(
#     "Network"
#     Network/network_test.cpp
#     ${MAINSRC}/Network/Network.cpp
#     ${MAINSRC}/Network/NetworkUtils.cpp
# )
