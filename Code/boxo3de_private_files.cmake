
set(FILES
    Include/BoxO3DE/Configuration/Box3DConfiguration.h
    Include/BoxO3DE/MathConversions.h
    Include/BoxO3DE/UserDataTypes.h
    Include/BoxO3DE/UserDataTypes.inl
    Include/BoxO3DE/Utils.h
    Include/BoxO3DE/Utils.inl
    Include/BoxO3DE/NativeTypeIdentifiers.h
    Include/BoxO3DE/ColliderComponentBus.h
    Include/BoxO3DE/ColliderShapeBus.h
    Include/BoxO3DE/EditorColliderComponentRequestBus.h
    Include/BoxO3DE/Material/Box3DMaterial.h
    Include/BoxO3DE/Material/Box3DMaterialConfiguration.h
    Include/BoxO3DE/Debug/Box3DDebugConfiguration.h
    Include/BoxO3DE/Debug/Box3DDebugInterface.h
    
    Source/BoxO3DEModule.cpp
    Source/BoxO3DEModule.h
    Source/Utils.cpp
    Source/Utils.h
        
    Source/Common/Box3DSceneQueryHelpers.cpp
    Source/Common/Box3DSceneQueryHelpers.h
        
    Source/Debug/Box3DDebugConfiguration.cpp
    Source/Debug/Box3DDebug.cpp
    Source/Debug/Box3DDebug.h
        
    Source/Clients/BoxO3DESystemComponent.cpp
    Source/Clients/BoxO3DESystemComponent.h
    Source/Clients/DefaultWorldComponent.cpp
    Source/Clients/DefaultWorldComponent.h
    Source/Clients/Shape.cpp
    Source/Clients/Shape.h
    Source/Clients/RigidBody.cpp
    Source/Clients/RigidBody.h
    Source/Clients/StaticRigidBody.cpp
    Source/Clients/StaticRigidBody.h
    Source/Clients/StaticRigidBodyComponent.cpp
    Source/Clients/StaticRigidBodyComponent.h
    Source/Clients/RigidBodyComponent.cpp
    Source/Clients/RigidBodyComponent.h
    Source/Clients/BaseColliderComponent.cpp
    Source/Clients/BaseColliderComponent.h
    Source/Clients/BoxColliderComponent.cpp
    Source/Clients/BoxColliderComponent.h
    Source/Clients/CapsuleColliderComponent.cpp
    Source/Clients/CapsuleColliderComponent.h
    Source/Clients/SphereColliderComponent.cpp
    Source/Clients/SphereColliderComponent.h
        
    Source/Configuration/Box3DConfiguration.cpp
    Source/Configuration/Box3DSettingsRegistryManager.cpp
    Source/Configuration/Box3DSettingsRegistryManager.h
        
    Source/System/Box3DSystem.cpp
    Source/System/Box3DSystem.h
        
    Source/Scene/Box3DScene.cpp
    Source/Scene/Box3DScene.h
    Source/Scene/Box3DSceneInterface.cpp
    Source/Scene/Box3DSceneInterface.h
        
    Source/Material/Box3DMaterial.cpp
    Source/Material/Box3DMaterialConfiguration.cpp
    Source/Material/Box3DMaterialManager.cpp
    Source/Material/Box3DMaterialManager.h
)
