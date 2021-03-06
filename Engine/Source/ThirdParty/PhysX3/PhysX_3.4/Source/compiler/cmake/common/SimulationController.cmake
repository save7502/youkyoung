#
# Build SimulationController common
#

SET(SIMULATIONCONTROLLER_BASE_DIR ${PHYSX_ROOT_DIR}/Source/SimulationController)
SET(SIMULATIONCONTROLLER_HEADERS		
	${SIMULATIONCONTROLLER_BASE_DIR}/include/ScActorCore.h
	${SIMULATIONCONTROLLER_BASE_DIR}/include/ScArticulationCore.h
	${SIMULATIONCONTROLLER_BASE_DIR}/include/ScArticulationJointCore.h
	${SIMULATIONCONTROLLER_BASE_DIR}/include/ScBodyCore.h
	${SIMULATIONCONTROLLER_BASE_DIR}/include/ScClothCore.h
	${SIMULATIONCONTROLLER_BASE_DIR}/include/ScClothFabricCore.h
	${SIMULATIONCONTROLLER_BASE_DIR}/include/ScConstraintCore.h
	${SIMULATIONCONTROLLER_BASE_DIR}/include/ScIterators.h
	${SIMULATIONCONTROLLER_BASE_DIR}/include/ScMaterialCore.h
	${SIMULATIONCONTROLLER_BASE_DIR}/include/ScParticleSystemCore.h
	${SIMULATIONCONTROLLER_BASE_DIR}/include/ScPhysics.h
	${SIMULATIONCONTROLLER_BASE_DIR}/include/ScRigidCore.h
	${SIMULATIONCONTROLLER_BASE_DIR}/include/ScScene.h
	${SIMULATIONCONTROLLER_BASE_DIR}/include/ScShapeCore.h
	${SIMULATIONCONTROLLER_BASE_DIR}/include/ScStaticCore.h
)
SOURCE_GROUP(include FILES ${SIMULATIONCONTROLLER_HEADERS})

SET(SIMULATIONCONTROLLER_SOURCE
	${SIMULATIONCONTROLLER_BASE_DIR}/src/ScActorCore.cpp
	${SIMULATIONCONTROLLER_BASE_DIR}/src/ScActorElementPair.h
	${SIMULATIONCONTROLLER_BASE_DIR}/src/ScActorInteraction.h
	${SIMULATIONCONTROLLER_BASE_DIR}/src/ScActorPair.h
	${SIMULATIONCONTROLLER_BASE_DIR}/src/ScActorSim.cpp
	${SIMULATIONCONTROLLER_BASE_DIR}/src/ScActorSim.h
	${SIMULATIONCONTROLLER_BASE_DIR}/src/ScArticulationCore.cpp
	${SIMULATIONCONTROLLER_BASE_DIR}/src/ScArticulationJointCore.cpp
	${SIMULATIONCONTROLLER_BASE_DIR}/src/ScArticulationJointSim.cpp
	${SIMULATIONCONTROLLER_BASE_DIR}/src/ScArticulationJointSim.h
	${SIMULATIONCONTROLLER_BASE_DIR}/src/ScArticulationSim.cpp
	${SIMULATIONCONTROLLER_BASE_DIR}/src/ScArticulationSim.h
	${SIMULATIONCONTROLLER_BASE_DIR}/src/ScBodyCore.cpp
	${SIMULATIONCONTROLLER_BASE_DIR}/src/ScBodyCoreKinematic.cpp
	${SIMULATIONCONTROLLER_BASE_DIR}/src/ScBodySim.cpp
	${SIMULATIONCONTROLLER_BASE_DIR}/src/ScBodySim.h
	${SIMULATIONCONTROLLER_BASE_DIR}/src/ScClient.h
	${SIMULATIONCONTROLLER_BASE_DIR}/src/ScConstraintCore.cpp
	${SIMULATIONCONTROLLER_BASE_DIR}/src/ScConstraintGroupNode.cpp
	${SIMULATIONCONTROLLER_BASE_DIR}/src/ScConstraintGroupNode.h
	${SIMULATIONCONTROLLER_BASE_DIR}/src/ScConstraintInteraction.cpp
	${SIMULATIONCONTROLLER_BASE_DIR}/src/ScConstraintInteraction.h
	${SIMULATIONCONTROLLER_BASE_DIR}/src/ScConstraintProjectionManager.cpp
	${SIMULATIONCONTROLLER_BASE_DIR}/src/ScConstraintProjectionManager.h
	${SIMULATIONCONTROLLER_BASE_DIR}/src/ScConstraintProjectionTree.cpp
	${SIMULATIONCONTROLLER_BASE_DIR}/src/ScConstraintProjectionTree.h
	${SIMULATIONCONTROLLER_BASE_DIR}/src/ScConstraintSim.cpp
	${SIMULATIONCONTROLLER_BASE_DIR}/src/ScConstraintSim.h
	${SIMULATIONCONTROLLER_BASE_DIR}/src/ScContactReportBuffer.h
	${SIMULATIONCONTROLLER_BASE_DIR}/src/ScContactStream.h
	${SIMULATIONCONTROLLER_BASE_DIR}/src/ScElementInteractionMarker.cpp
	${SIMULATIONCONTROLLER_BASE_DIR}/src/ScElementInteractionMarker.h
	${SIMULATIONCONTROLLER_BASE_DIR}/src/ScElementSim.cpp
	${SIMULATIONCONTROLLER_BASE_DIR}/src/ScElementSim.h
	${SIMULATIONCONTROLLER_BASE_DIR}/src/ScElementSimInteraction.h
	${SIMULATIONCONTROLLER_BASE_DIR}/src/ScInteraction.cpp
	${SIMULATIONCONTROLLER_BASE_DIR}/src/ScInteraction.h
	${SIMULATIONCONTROLLER_BASE_DIR}/src/ScInteractionFlags.h
	${SIMULATIONCONTROLLER_BASE_DIR}/src/ScIterators.cpp
	${SIMULATIONCONTROLLER_BASE_DIR}/src/ScMaterialCore.cpp
	${SIMULATIONCONTROLLER_BASE_DIR}/src/ScMetaData.cpp
	${SIMULATIONCONTROLLER_BASE_DIR}/src/ScNPhaseCore.cpp
	${SIMULATIONCONTROLLER_BASE_DIR}/src/ScNPhaseCore.h
	${SIMULATIONCONTROLLER_BASE_DIR}/src/ScObjectIDTracker.h
	${SIMULATIONCONTROLLER_BASE_DIR}/src/ScPhysics.cpp
	${SIMULATIONCONTROLLER_BASE_DIR}/src/ScRbElementInteraction.h
	${SIMULATIONCONTROLLER_BASE_DIR}/src/ScRigidCore.cpp
	${SIMULATIONCONTROLLER_BASE_DIR}/src/ScRigidSim.cpp
	${SIMULATIONCONTROLLER_BASE_DIR}/src/ScRigidSim.h
	${SIMULATIONCONTROLLER_BASE_DIR}/src/ScScene.cpp
	${SIMULATIONCONTROLLER_BASE_DIR}/src/ScShapeCore.cpp
	${SIMULATIONCONTROLLER_BASE_DIR}/src/ScShapeInteraction.cpp
	${SIMULATIONCONTROLLER_BASE_DIR}/src/ScShapeInteraction.h
	${SIMULATIONCONTROLLER_BASE_DIR}/src/ScShapeIterator.h
	${SIMULATIONCONTROLLER_BASE_DIR}/src/ScShapeSim.cpp
	${SIMULATIONCONTROLLER_BASE_DIR}/src/ScShapeSim.h
	${SIMULATIONCONTROLLER_BASE_DIR}/src/ScSimStateData.h
	${SIMULATIONCONTROLLER_BASE_DIR}/src/ScSimStats.cpp
	${SIMULATIONCONTROLLER_BASE_DIR}/src/ScSimStats.h
	${SIMULATIONCONTROLLER_BASE_DIR}/src/ScSimulationController.cpp
	${SIMULATIONCONTROLLER_BASE_DIR}/src/ScSimulationController.h
	${SIMULATIONCONTROLLER_BASE_DIR}/src/ScSqBoundsManager.cpp
	${SIMULATIONCONTROLLER_BASE_DIR}/src/ScSqBoundsManager.h
	${SIMULATIONCONTROLLER_BASE_DIR}/src/ScStaticCore.cpp
	${SIMULATIONCONTROLLER_BASE_DIR}/src/ScStaticSim.cpp
	${SIMULATIONCONTROLLER_BASE_DIR}/src/ScStaticSim.h
	${SIMULATIONCONTROLLER_BASE_DIR}/src/ScTriggerInteraction.cpp
	${SIMULATIONCONTROLLER_BASE_DIR}/src/ScTriggerInteraction.h
	${SIMULATIONCONTROLLER_BASE_DIR}/src/ScTriggerPairs.h
)
SOURCE_GROUP(src FILES ${SIMULATIONCONTROLLER_SOURCE})

SET(SIMULATIONCONTROLLER_CLOTH_SOURCE
	${SIMULATIONCONTROLLER_BASE_DIR}/src/cloth/ScClothCore.cpp
	${SIMULATIONCONTROLLER_BASE_DIR}/src/cloth/ScClothFabricCore.cpp
	${SIMULATIONCONTROLLER_BASE_DIR}/src/cloth/ScClothShape.cpp
	${SIMULATIONCONTROLLER_BASE_DIR}/src/cloth/ScClothShape.h
	${SIMULATIONCONTROLLER_BASE_DIR}/src/cloth/ScClothSim.cpp
	${SIMULATIONCONTROLLER_BASE_DIR}/src/cloth/ScClothSim.h
)
SOURCE_GROUP(src\\cloth FILES ${SIMULATIONCONTROLLER_CLOTH_SOURCE})

SET(SIMULATIONCONTROLLER_PARTICLES_SOURCE	
	${SIMULATIONCONTROLLER_BASE_DIR}/src/particles/ScParticleBodyInteraction.cpp
	${SIMULATIONCONTROLLER_BASE_DIR}/src/particles/ScParticleBodyInteraction.h
	${SIMULATIONCONTROLLER_BASE_DIR}/src/particles/ScParticlePacketShape.cpp
	${SIMULATIONCONTROLLER_BASE_DIR}/src/particles/ScParticlePacketShape.h
	${SIMULATIONCONTROLLER_BASE_DIR}/src/particles/ScParticleSystemCore.cpp
	${SIMULATIONCONTROLLER_BASE_DIR}/src/particles/ScParticleSystemSim.cpp
	${SIMULATIONCONTROLLER_BASE_DIR}/src/particles/ScParticleSystemSim.h
)
SOURCE_GROUP(src\\particles FILES ${SIMULATIONCONTROLLER_PARTICLES_SOURCE})

ADD_LIBRARY(SimulationController ${SIMULATIONCONTROLLER_LIBTYPE}
	${SIMULATIONCONTROLLER_HEADERS}
	${SIMULATIONCONTROLLER_SOURCE}
	
	${SIMULATIONCONTROLLER_CLOTH_SOURCE}
	${SIMULATIONCONTROLLER_PARTICLES_SOURCE}
)

TARGET_INCLUDE_DIRECTORIES(SimulationController 
	PRIVATE ${SIMULATIONCONTROLLER_PLATFORM_INCLUDES}

	PRIVATE ${PXSHARED_ROOT_DIR}/include
	PRIVATE ${PXSHARED_ROOT_DIR}/src/foundation/include
	PRIVATE ${PXSHARED_ROOT_DIR}/src/fastxml/include
	PRIVATE ${PXSHARED_ROOT_DIR}/src/pvd/include

	PRIVATE ${PHYSX_ROOT_DIR}/Include
	PRIVATE ${PHYSX_ROOT_DIR}/Include/common
	PRIVATE ${PHYSX_ROOT_DIR}/Include/geometry
	PRIVATE ${PHYSX_ROOT_DIR}/Include/pvd
	PRIVATE ${PHYSX_ROOT_DIR}/Include/particles
	PRIVATE ${PHYSX_ROOT_DIR}/Include/cloth
	PRIVATE ${PHYSX_ROOT_DIR}/Include/GeomUtils
	
	PRIVATE ${PHYSX_SOURCE_DIR}/Common/include
	PRIVATE ${PHYSX_SOURCE_DIR}/Common/src
	
	PRIVATE ${PHYSX_SOURCE_DIR}/PhysXGpu/include
	
	PRIVATE ${PHYSX_SOURCE_DIR}/GeomUtils/headers
	PRIVATE ${PHYSX_SOURCE_DIR}/GeomUtils/src
	PRIVATE ${PHYSX_SOURCE_DIR}/GeomUtils/src/contact
	PRIVATE ${PHYSX_SOURCE_DIR}/GeomUtils/src/common
	PRIVATE ${PHYSX_SOURCE_DIR}/GeomUtils/src/convex
	PRIVATE ${PHYSX_SOURCE_DIR}/GeomUtils/src/distance
	PRIVATE ${PHYSX_SOURCE_DIR}/GeomUtils/src/sweep
	PRIVATE ${PHYSX_SOURCE_DIR}/GeomUtils/src/gjk
	PRIVATE ${PHYSX_SOURCE_DIR}/GeomUtils/src/intersection
	PRIVATE ${PHYSX_SOURCE_DIR}/GeomUtils/src/mesh
	PRIVATE ${PHYSX_SOURCE_DIR}/GeomUtils/src/hf
	PRIVATE ${PHYSX_SOURCE_DIR}/GeomUtils/src/pcm
	PRIVATE ${PHYSX_SOURCE_DIR}/GeomUtils/src/ccd
	
	PRIVATE ${PHYSX_SOURCE_DIR}/SimulationController/include
	PRIVATE ${PHYSX_SOURCE_DIR}/SimulationController/src
	PRIVATE ${PHYSX_SOURCE_DIR}/SimulationController/src/particles
	PRIVATE ${PHYSX_SOURCE_DIR}/SimulationController/src/cloth
	
	PRIVATE ${PHYSX_SOURCE_DIR}/LowLevel/API/include
	PRIVATE ${PHYSX_SOURCE_DIR}/LowLevel/common/include
	PRIVATE ${PHYSX_SOURCE_DIR}/LowLevel/common/include/collision
	PRIVATE ${PHYSX_SOURCE_DIR}/LowLevel/common/include/pipeline
	PRIVATE ${PHYSX_SOURCE_DIR}/LowLevel/common/include/utils
	PRIVATE ${PHYSX_SOURCE_DIR}/LowLevel/software/include

	PRIVATE ${PHYSX_SOURCE_DIR}/LowLevelDynamics/include

	PRIVATE ${PHYSX_SOURCE_DIR}/LowLevelCloth/include
	
	PRIVATE ${PHYSX_SOURCE_DIR}/LowLevelAABB/include
	
	PRIVATE ${PHYSX_SOURCE_DIR}/LowLevelParticles/include
	
)

# No linked libraries

# Use generator expressions to set config specific preprocessor definitions
TARGET_COMPILE_DEFINITIONS(SimulationController 

	# Common to all configurations
	PRIVATE ${SIMULATIONCONTROLLER_COMPILE_DEFS}
)

IF(NOT ${SIMULATIONCONTROLLER_LIBTYPE} STREQUAL "OBJECT")
	SET_TARGET_PROPERTIES(SimulationController PROPERTIES 
		COMPILE_PDB_NAME_DEBUG "SimulationController${CMAKE_DEBUG_POSTFIX}"
		COMPILE_PDB_NAME_CHECKED "SimulationController${CMAKE_CHECKED_POSTFIX}"
		COMPILE_PDB_NAME_PROFILE "SimulationController${CMAKE_PROFILE_POSTFIX}"
		COMPILE_PDB_NAME_RELEASE "SimulationController${CMAKE_RELEASE_POSTFIX}"
	)
ENDIF()