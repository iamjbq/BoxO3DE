
#include <Clients/BoxO3DESystemComponent.h>

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/Physics/Material/PhysicsMaterialAsset.h>

#include <Material/Box3DMaterialManager.h>
#include <System/Box3DSystem.h>
#include <BoxO3DE/Configuration/Box3DConfiguration.h>
#include <BoxO3DE/BoxO3DETypeIds.h>

#include "BoxO3DE/Material/Box3DMaterialConfiguration.h"

namespace B3
{
    AZ_COMPONENT_IMPL(BoxO3DESystemComponent, "BoxO3DESystemComponent",
        BoxO3DESystemComponentTypeId);

    void BoxO3DESystemComponent::Reflect(AZ::ReflectContext* context)
    {
        Box3DSystemConfiguration::Reflect(context);
        Debug::DebugConfiguration::Reflect(context);
        MaterialConfiguration::Reflect(context);
        
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<BoxO3DESystemComponent, AZ::Component>()
                ->Version(0)
                ->Attribute(AZ::Edit::Attributes::SystemComponentTags, AZStd::vector<AZ::Crc32>({ AZ_CRC_CE("AssetBuilder") }))
                ->Field("Enabled", &BoxO3DESystemComponent::m_enabled);
            
            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<BoxO3DESystemComponent>("Box3D", "Global Box3D physics configuration.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Box3D")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &BoxO3DESystemComponent::m_enabled,
                    "Enabled", "Enables the Box3D system component.")
                ;
            }
        }
    }

    void BoxO3DESystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("PhysicsService"));
    }

    void BoxO3DESystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("PhysicsService"));
    }

    void BoxO3DESystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void BoxO3DESystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        dependent.push_back(AZ_CRC_CE("AssetDatabaseService"));
        dependent.push_back(AZ_CRC_CE("AssetCatalogService"));
    }

    BoxO3DESystemComponent::BoxO3DESystemComponent()
    : m_enabled(true)
        , m_onSystemInitializedHandler(
            [this](const AzPhysics::SystemConfiguration* config)
            {
                EnableAutoManagedPhysicsTick(config->m_autoManageSimulationUpdate);
            })
        , m_onSystemConfigChangedHandler(
            [this](const AzPhysics::SystemConfiguration* config)
            {
                EnableAutoManagedPhysicsTick(config->m_autoManageSimulationUpdate);
            })
    {
    }

    BoxO3DESystemComponent::~BoxO3DESystemComponent()
    {
        if (m_physicsSystem.Get() == this)
        {
            m_physicsSystem.Unregister(this);
        }
    }

    void BoxO3DESystemComponent::Init()
    {
        if (m_physicsSystem.Get() == nullptr)
        {
            m_physicsSystem.Register(this);
        }
    }
    
    template<typename AssetHandlerT, typename AssetT>
    void RegisterAsset(AZStd::vector<AZStd::unique_ptr<AZ::Data::AssetHandler>>& assetHandlers)
    {
        AssetHandlerT* handler = aznew AssetHandlerT();
        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequests::EnableCatalogForAsset, AZ::AzTypeInfo<AssetT>::Uuid());
        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequests::AddExtension, AssetHandlerT::s_assetFileExtension);
        assetHandlers.emplace_back(handler);
    }

    void BoxO3DESystemComponent::Activate()
    {
        if (!m_enabled)
        {
            return;
        }

        m_defaultWorldComponent.Activate();

        Physics::SystemRequestBus::Handler::BusConnect();
        Physics::CollisionRequestBus::Handler::BusConnect();

        ActivateSimulation();
    }

    void BoxO3DESystemComponent::Deactivate()
    {
        AZ::TickBus::Handler::BusDisconnect();
        Physics::CollisionRequestBus::Handler::BusDisconnect();
        Physics::SystemRequestBus::Handler::BusDisconnect();

        m_defaultWorldComponent.Deactivate();

        m_materialManager.reset();

        m_onSystemInitializedHandler.Disconnect();
        m_onSystemConfigChangedHandler.Disconnect();

        if (m_box3DSystem != nullptr)
        {
            m_box3DSystem->Shutdown();
            m_box3DSystem = nullptr;
        }
    }

    void BoxO3DESystemComponent::OnTick(float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        if (m_box3DSystem)
        {
            m_box3DSystem->Simulate(deltaTime);
        }
    }

    int BoxO3DESystemComponent::GetTickOrder()
    {
        return AZ::ComponentTickBus::TICK_PHYSICS_SYSTEM;
    }

    AZStd::shared_ptr<Physics::Shape> BoxO3DESystemComponent::CreateShape(
        const Physics::ColliderConfiguration& colliderConfiguration, const Physics::ShapeConfiguration& configuration)
    {
        auto shapePtr = AZStd::make_shared<B3::Shape>(colliderConfiguration, configuration);
        
        if (shapePtr)
        {
            return shapePtr;
        }

        AZ_Error("Box3D", false, "SystemComponent::CreateShape error. Unable to create a shape from configuration.")

        return nullptr;
    }

    void BoxO3DESystemComponent::ReleaseNativeMeshObject([[maybe_unused]] void* nativeMeshObject)
    {
    }

    void BoxO3DESystemComponent::ReleaseNativeHeightfieldObject([[maybe_unused]] void* nativeHeightfieldObject)
    {
    }

    bool BoxO3DESystemComponent::CookConvexMeshToFile([[maybe_unused]] const AZStd::string& filePath, [[maybe_unused]] const AZ::Vector3* vertices,
        [[maybe_unused]] AZ::u32 vertexCount)
    {
        return false;
    }

    bool BoxO3DESystemComponent::CookConvexMeshToMemory([[maybe_unused]] const AZ::Vector3* vertices, [[maybe_unused]] AZ::u32 vertexCount,
        [[maybe_unused]] AZStd::vector<AZ::u8>& result)
    {
        return false;
    }

    bool BoxO3DESystemComponent::CookTriangleMeshToFile([[maybe_unused]] const AZStd::string& filePath, [[maybe_unused]] const AZ::Vector3* vertices,
        [[maybe_unused]] AZ::u32 vertexCount, [[maybe_unused]] const AZ::u32* indices, [[maybe_unused]] AZ::u32 indexCount)
    {
        return false;
    }

    bool BoxO3DESystemComponent::CookTriangleMeshToMemory([[maybe_unused]] const AZ::Vector3* vertices, [[maybe_unused]] AZ::u32 vertexCount,
        [[maybe_unused]] const AZ::u32* indices, [[maybe_unused]] AZ::u32 indexCount, [[maybe_unused]] AZStd::vector<AZ::u8>& result)
    {
        return false;
    }

    AzPhysics::CollisionLayer BoxO3DESystemComponent::GetCollisionLayerByName(const AZStd::string& layerName)
    {
        return m_box3DSystem->GetBox3DConfiguration().m_collisionConfig.m_collisionLayers.GetLayer(layerName);
    }

    AZStd::string BoxO3DESystemComponent::GetCollisionLayerName(const AzPhysics::CollisionLayer& layer)
    {
        return m_box3DSystem->GetBox3DConfiguration().m_collisionConfig.m_collisionLayers.GetName(layer);
    }

    bool BoxO3DESystemComponent::TryGetCollisionLayerByName(const AZStd::string& layerName,
        AzPhysics::CollisionLayer& layer)
    {
        return m_box3DSystem->GetBox3DConfiguration().m_collisionConfig.m_collisionLayers.TryGetLayer(layerName, layer);
    }

    AzPhysics::CollisionGroup BoxO3DESystemComponent::GetCollisionGroupByName(const AZStd::string& groupName)
    {
        return m_box3DSystem->GetBox3DConfiguration().m_collisionConfig.m_collisionGroups.FindGroupByName(groupName);
    }

    bool BoxO3DESystemComponent::TryGetCollisionGroupByName(const AZStd::string& layerName,
        AzPhysics::CollisionGroup& group)
    {
        return m_box3DSystem->GetBox3DConfiguration().m_collisionConfig.m_collisionGroups.TryFindGroupByName(layerName, group);
    }

    AZStd::string BoxO3DESystemComponent::GetCollisionGroupName(const AzPhysics::CollisionGroup& collisionGroup)
    {
        AZStd::string groupName;
        for (const auto& group : m_box3DSystem->GetBox3DConfiguration().m_collisionConfig.m_collisionGroups.GetPresets())
        {
            if (group.m_group.GetMask() == collisionGroup.GetMask())
            {
                groupName = group.m_name;
                break;
            }
        }
        return groupName;
    }

    AzPhysics::CollisionGroup BoxO3DESystemComponent::GetCollisionGroupById(
        const AzPhysics::CollisionGroups::Id& groupId)
    {
        auto groups = m_box3DSystem->GetBox3DConfiguration().m_collisionConfig.m_collisionGroups;
        return groups.FindGroupById(groupId);
    }

    void BoxO3DESystemComponent::SetCollisionLayerName(int index, const AZStd::string& layerName)
    {
        m_box3DSystem->SetCollisionLayerName(index, layerName);
    }

    void BoxO3DESystemComponent::CreateCollisionGroup(const AZStd::string& groupName,
        const AzPhysics::CollisionGroup& group)
    {
        m_box3DSystem->CreateCollisionGroup(groupName, group);
    }

    bool BoxO3DESystemComponent::ShouldCollide(const Physics::ColliderConfiguration& colliderConfigurationA,
        const Physics::ColliderConfiguration& colliderConfigurationB)
    {
        AZ_UNUSED_2(colliderConfigurationA, colliderConfigurationB);
        AZ_Info("BoxO3DESystemComponent::ShouldCollide","Not implemented")
        return false;
    }

    void BoxO3DESystemComponent::EnableAutoManagedPhysicsTick(bool shouldTick)
    {
        if (shouldTick && !m_isTickingPhysics)
        {
            AZ::TickBus::Handler::BusConnect();
        }
        else if (!shouldTick && m_isTickingPhysics)
        {
            AZ::TickBus::Handler::BusDisconnect();
        }
        m_isTickingPhysics = shouldTick;
    }

    void BoxO3DESystemComponent::ActivateSimulation()
    {
        m_box3DSystem = GetBox3DSystem();
        
        if (m_box3DSystem)
        {
            m_box3DSystem->RegisterSystemInitializedEvent(m_onSystemInitializedHandler);
            m_box3DSystem->RegisterSystemConfigurationChangedEvent(m_onSystemConfigChangedHandler);

            const Box3DSettingsRegistryManager& registryManager = m_box3DSystem->GetSettingsRegistryManager();
            if (AZStd::optional<Box3DSystemConfiguration> config = registryManager.LoadSystemConfiguration();
                config.has_value())
            {
                m_box3DSystem->Initialize(&(*config));
            }
            else //load defaults if there is no config
            {
                const Box3DSystemConfiguration defaultConfig = Box3DSystemConfiguration::CreateDefault();
                m_box3DSystem->Initialize(&defaultConfig);

                auto saveCallback = []([[maybe_unused]] const Box3DSystemConfiguration& config, [[maybe_unused]] Box3DSettingsRegistryManager::Result result)
                {
                    AZ_Warning("Box3D", result == Box3DSettingsRegistryManager::Result::Success,
                        "Unable to save the default Box3D configuration.");
                };
                registryManager.SaveSystemConfiguration(defaultConfig, saveCallback);
            }

            //Load the DefaultSceneConfig
            if (AZStd::optional<AzPhysics::SceneConfiguration> config = registryManager.LoadDefaultSceneConfiguration();
                config.has_value())
            {
                m_box3DSystem->UpdateDefaultSceneConfiguration((*config));
            }
            else //load defaults if there is no config
            {
                const AzPhysics::SceneConfiguration defaultConfig = AzPhysics::SceneConfiguration::CreateDefault();
                m_box3DSystem->UpdateDefaultSceneConfiguration(defaultConfig);

                auto saveCallback = []([[maybe_unused]] const AzPhysics::SceneConfiguration& config, [[maybe_unused]] Box3DSettingsRegistryManager::Result result)
                {
                    AZ_Warning("Box3D", result == Box3DSettingsRegistryManager::Result::Success,
                        "Unable to save the default Scene configuration.");
                };
                registryManager.SaveDefaultSceneConfiguration(defaultConfig, saveCallback);
            }
            
            // load the debug configuration and initialize the Box3D debug interface
             if (auto* debug = AZ::Interface<Debug::Box3DDebugInterface>::Get())
             {
                 if (AZStd::optional<Debug::DebugConfiguration> config = registryManager.LoadDebugConfiguration();
                     config.has_value())
                 {
                     debug->Initialize(*config);
                 }
                 else //load defaults if there is no config
                 {
                     const Debug::DebugConfiguration defaultConfig = Debug::DebugConfiguration::CreateDefault();
                     debug->Initialize(defaultConfig);
            
                     auto saveCallback = []([[maybe_unused]] const Debug::DebugConfiguration& config, [[maybe_unused]] Box3DSettingsRegistryManager::Result result)
                     {
                         AZ_Warning("Box3D", result == Box3DSettingsRegistryManager::Result::Success,
                             "Unable to save the default Box3D Debug configuration.");
                     };
                     registryManager.SaveDebugConfiguration(defaultConfig, saveCallback);
                 }
             }
        }
        
        m_materialManager = AZStd::make_unique<MaterialManager>();
        m_materialManager->Init();
    }
} // namespace B3
