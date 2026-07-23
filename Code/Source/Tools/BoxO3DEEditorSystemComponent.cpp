#include <Tools/BoxO3DEEditorSystemComponent.h>

#include <AzCore/Interface/Interface.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Physics/Collision/CollisionEvents.h>
#include <AzFramework/Physics/Common/PhysicsSimulatedBody.h>

#include <IEditor.h>

#include <BoxO3DE/BoxO3DETypeIds.h>
#include <Tools/System/EditorWindow.h>
#include <Tools/System/PropertyTypes.h>
#include <Tools/System/Box3DEditorMaterialAsset.h>
#include <System/Box3DSystem.h>

#include "System/ColliderComponentMode.h"

namespace B3
{
    AZ_COMPONENT_IMPL(BoxO3DEEditorSystemComponent, "BoxO3DEEditorSystemComponent",
        BoxO3DEEditorSystemComponentTypeId);

    void BoxO3DEEditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        ColliderComponentMode::Reflect(context);
        EditorMaterialAsset::Reflect(context);
        
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<BoxO3DEEditorSystemComponent, AZ::Component>()
                ->Version(0)
                ->Attribute(AZ::Edit::Attributes::SystemComponentTags, AZStd::vector<AZ::Crc32>({ AZ_CRC_CE("AssetBuilder") }));
        }
    }

    void BoxO3DEEditorSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("PhysicsEditorService"));
    }

    void BoxO3DEEditorSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("PhysicsEditorService"));
    }

    void BoxO3DEEditorSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("PhysicsService"));
    }

    void BoxO3DEEditorSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        dependent.push_back(AZ_CRC_CE("AssetDatabaseService"));
        dependent.push_back(AZ_CRC_CE("AssetCatalogService"));
        dependent.push_back(AZ_CRC_CE("PhysicsMaterialService"));
    }

    void BoxO3DEEditorSystemComponent::Activate()
    {
        Physics::EditorWorldBus::Handler::BusConnect();

        // Register Box3D Material Asset
        auto* materialAsset = aznew AzFramework::GenericAssetHandler<B3::EditorMaterialAsset>("Box3D Material", Physics::MaterialAsset::AssetGroup, EditorMaterialAsset::FileExtension);
        materialAsset->Register();
        m_assetHandlers.emplace_back(materialAsset);

        // Register Box3D Material Asset Builder
        AssetBuilderSDK::AssetBuilderDesc materialAssetBuilderDescriptor;
        materialAssetBuilderDescriptor.m_name = "Box3D Material Asset Builder";
        materialAssetBuilderDescriptor.m_version = 1; // bump this to rebuild all box3dmaterial files
        materialAssetBuilderDescriptor.m_patterns.push_back(AssetBuilderSDK::AssetBuilderPattern(AZStd::string::format("*.%s", EditorMaterialAsset::FileExtension), AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
        materialAssetBuilderDescriptor.m_busId = azrtti_typeid<EditorMaterialAssetBuilder>();
        materialAssetBuilderDescriptor.m_createJobFunction = [this](const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response)
        {
            m_materialAssetBuilder.CreateJobs(request, response);
        };
        materialAssetBuilderDescriptor.m_processJobFunction = [this](const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response)
        {
            m_materialAssetBuilder.ProcessJob(request, response);
        };
        m_materialAssetBuilder.BusConnect(materialAssetBuilderDescriptor.m_busId);
        AssetBuilderSDK::AssetBuilderBus::Broadcast(&AssetBuilderSDK::AssetBuilderBus::Handler::RegisterBuilderInformation, materialAssetBuilderDescriptor);

        if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
        {
            AzPhysics::SceneConfiguration editorWorldConfiguration = physicsSystem->GetDefaultSceneConfiguration();
            editorWorldConfiguration.m_sceneName = AzPhysics::EditorPhysicsSceneName;
            m_editorWorldSceneHandle = physicsSystem->AddScene(editorWorldConfiguration);
        }

        B3::Editor::RegisterPropertyTypes(); // for registering custom UIs and property handlers

        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
        AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusConnect();
        AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler::BusConnect();
    }

    void BoxO3DEEditorSystemComponent::Deactivate()
    {
        AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
        Physics::EditorWorldBus::Handler::BusDisconnect();

        if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
        {
            physicsSystem->RemoveScene(m_editorWorldSceneHandle);
        }
        m_editorWorldSceneHandle = AzPhysics::InvalidSceneHandle;

        m_materialAssetBuilder.BusDisconnect();
        
        for (auto& assetHandler : m_assetHandlers)
        {
            if (auto editorMaterialAssetHandler = azrtti_cast<AzFramework::GenericAssetHandler<B3::EditorMaterialAsset>*>(assetHandler.get());
                editorMaterialAssetHandler != nullptr)
            {
                editorMaterialAssetHandler->Unregister();
            }
        }
        m_assetHandlers.clear();
    }

    AzPhysics::SceneHandle BoxO3DEEditorSystemComponent::GetEditorSceneHandle() const
    {
        return m_editorWorldSceneHandle;
    }

    void BoxO3DEEditorSystemComponent::OnActionRegistrationHook()
    {
        // ColliderComponentMode::RegisterActions();
        // JointsComponentMode::RegisterActions();
    }

    void BoxO3DEEditorSystemComponent::OnActionContextModeBindingHook()
    {
        // ColliderComponentMode::BindActionsToModes();
        // JointsComponentMode::BindActionsToModes();
    }

    void BoxO3DEEditorSystemComponent::OnMenuBindingHook()
    {
        // ColliderComponentMode::BindActionsToMenus();
        // JointsComponentMode::BindActionsToMenus();
    }

    void BoxO3DEEditorSystemComponent::OnStartPlayInEditorBegin()
    {
        if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
        {
            if (AzPhysics::Scene* scene = physicsSystem->GetScene(m_editorWorldSceneHandle))
            {
                scene->SetEnabled(false);
            }
        }
    }

    void BoxO3DEEditorSystemComponent::OnStopPlayInEditor()
    {
        if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
        {
            if (AzPhysics::Scene* scene = physicsSystem->GetScene(m_editorWorldSceneHandle))
            {
                scene->SetEnabled(true);
            }
        }
    }

    void BoxO3DEEditorSystemComponent::NotifyRegisterViews()
    {
        B3::Editor::EditorWindow::RegisterViewClass(); // for adding configuration widget to tools menu
    }
} // namespace B3
