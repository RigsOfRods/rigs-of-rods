rigsofrods_test_suite(
    NAME
        "SimControllerTestSuite"
    SOURCES
        "${CMAKE_SOURCE_DIR}/source/main/gameplay/RoRFrameListener.cpp"         # SimController
        "${CMAKE_SOURCE_DIR}/source/main/physics/Beam.cpp"                      # Actor
        "${CMAKE_SOURCE_DIR}/source/main/physics/BeamForcesEuler.cpp"           # Actor
        "${CMAKE_SOURCE_DIR}/source/main/physics/BeamSlideNode.cpp"             # Actor
        "${CMAKE_SOURCE_DIR}/source/main/physics/BeamFactory.cpp"               # ActorManager
        "${CMAKE_SOURCE_DIR}/source/main/physics/RigSpawner.cpp"                # ActorSpawner
        "${CMAKE_SOURCE_DIR}/source/main/physics/RigSpawner_ProcessControl.cpp" # ActorSpawner
        "${CMAKE_SOURCE_DIR}/source/main/resources/rig_def_fileformat/RigDef_Node.cpp"
)
