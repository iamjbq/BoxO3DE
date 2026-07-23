
#pragma once

#include <AzCore/Component/TickBus.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzFramework/Physics/PhysicsSystem.h>
#include <AzFramework/Physics/Configuration/SystemConfiguration.h>

#include <Configuration/Box3DSettingsRegistryManager.h>
#include <Debug/Box3DDebug.h>
#include <Scene/Box3DSceneInterface.h>
// #include <System/PhysXAllocator.h>
// #include <System/PhysXSdkCallbacks.h>

#include <BoxO3DE/Configuration/Box3DConfiguration.h>
// #include <System/PhysXJointInterface.h>

namespace AZ::Debug
{
    class PerformanceCollector;
}

namespace B3
{
    class Box3DSystem
        : public AZ::Interface<AzPhysics::SystemInterface>::Registrar
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL;
        AZ_RTTI(Box3DSystem, "{D930B5E8-1D44-4FA9-9F22-D13CE73B7B82}", AzPhysics::SystemInterface);

        Box3DSystem(AZStd::unique_ptr<Box3DSettingsRegistryManager> registryManager);
        ~Box3DSystem();

        // SystemInterface interface ...
        void Initialize(const AzPhysics::SystemConfiguration* config) override;
        void Reinitialize() override;
        void Shutdown() override;
        void Simulate(float deltaTime) override;
        AzPhysics::SceneHandle AddScene(const AzPhysics::SceneConfiguration& config) override;
        AzPhysics::SceneHandleList AddScenes(const AzPhysics::SceneConfigurationList& configs) override;
        AzPhysics::SceneHandle GetSceneHandle(const AZStd::string& sceneName) override;
        AzPhysics::Scene* GetScene(AzPhysics::SceneHandle handle) override;
        AzPhysics::SceneList GetScenes(const AzPhysics::SceneHandleList& handles) override;
        AzPhysics::SceneList& GetAllScenes() override;
        void RemoveScene(AzPhysics::SceneHandle handle) override;
        void RemoveScenes(const AzPhysics::SceneHandleList& handles) override;
        void RemoveAllScenes() override;
        AZStd::pair<AzPhysics::SceneHandle, AzPhysics::SimulatedBodyHandle> FindAttachedBodyHandleFromEntityId(AZ::EntityId entityId) override;
        const AzPhysics::SystemConfiguration* GetConfiguration() const override;
        void UpdateConfiguration(const AzPhysics::SystemConfiguration* newConfig, bool forceReinitialization = false) override;
        void UpdateDefaultSceneConfiguration(const AzPhysics::SceneConfiguration& sceneConfiguration) override;
        const AzPhysics::SceneConfiguration& GetDefaultSceneConfiguration() const override;

        //! Accessor to get the current Box3D configuration data.
        const Box3DSystemConfiguration& GetBox3DConfiguration() const;

        //! Accessor to get the Settings Registry Manager.
        const Box3DSettingsRegistryManager& GetSettingsRegistryManager() const;

        //TEMP -- until these are fully moved over here
        // physx::PxPhysics* GetPxPhysics() { return m_physXSdk.m_physics; }
        // physx::PxCooking* GetPxCooking() { return m_physXSdk.m_cooking; }
        // physx::PxCpuDispatcher* GetPxCpuDispathcher()
        // {
        //     AZ_Assert(m_cpuDispatcher, "PhysX CPU dispatcher was not created");
        //     return m_cpuDispatcher;
        // }
        void SetCollisionLayerName(int index, const AZStd::string& layerName);
        void CreateCollisionGroup(const AZStd::string& groupName, const AzPhysics::CollisionGroup& group);
        //TEMP -- until these are fully moved over here

        AZ::Debug::PerformanceCollector* GetPerformanceCollector();

    private:
        //! Initializes the PhysX SDK.
        //! This sets up the PhysX Foundation, Cooking, and other PhysX sub-systems.
        //! @param cookingParams The cooking params to use when setting up PhysX cooking interface.
        // void InitializePhysXSdk(const physx::PxCookingParams& cookingParams);
        // void ShutdownPhysXSdk();

        void InitializePerformanceCollector();

        Box3DSystemConfiguration m_systemConfig;
        AzPhysics::SceneConfiguration m_defaultSceneConfiguration;
        AzPhysics::SceneList m_sceneList;
        AZStd::queue<AzPhysics::SceneIndex> m_freeSceneSlots; //when a scene is removed cache its index here to be used for the next add.

        float m_accumulatedTime = 0.0f;

        // struct PhysXSdk
        // {
        //     physx::PxFoundation* m_foundation = nullptr;
        //     physx::PxPhysics* m_physics = nullptr;
        //     physx::PxCooking* m_cooking = nullptr;
        // };
        // PhysXSdk m_physXSdk;
        // PxAzAllocatorCallback m_physXAllocatorCallback;
        // PxAzErrorCallback m_physXErrorCallback;
        // PxAzProfilerCallback m_pxAzProfilerCallback;
        //
        // physx::PxCpuDispatcher* m_cpuDispatcher = nullptr;

        enum class State : AZ::u8
        {
            Uninitialized = 0,
            Initialized,
            Shutdown
        };
        State m_state = State::Uninitialized;

        Debug::Box3DDebug m_box3DDebug; //! Handler for the Box3DDebug Interface.
        AZStd::unique_ptr<Box3DSettingsRegistryManager> m_registryManager; //! Handles all settings registry interactions.
        Box3DSceneInterface m_sceneInterface; //! Implemented the Scene Az::Interface.
        // PhysXJointHelpersInterface m_jointHelperInterface; //! Implementation of the JointHelpersInterface.

        static constexpr AZStd::string_view PerformanceLogCategory = "Box3D";
        static constexpr AZStd::string_view PerformanceSpecBox3DSimulationTime = "Box3D Simulation Time";

        AZStd::unique_ptr<AZ::Debug::PerformanceCollector> m_performanceCollector;
    };

    //! Helper function for getting the Box3D System interface from inside the Box3D gem.
    Box3DSystem* GetBox3DSystem();
}
