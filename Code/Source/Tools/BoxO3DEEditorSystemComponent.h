
#pragma once

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Component/Component.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzFramework/Physics/Common/PhysicsEvents.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzToolsFramework/ActionManager/ActionManagerRegistrationNotificationBus.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <Tools/System/Box3DEditorMaterialAssetBuilder.h>
#include <Clients/BoxO3DESystemComponent.h>

namespace AzPhysics
{
    struct TriggerEvent;
}

namespace B3
{
    /// System component for BoxO3DE editor
    class BoxO3DEEditorSystemComponent
        : public AZ::Component
        , public Physics::EditorWorldBus::Handler
        , private AzToolsFramework::EditorEntityContextNotificationBus::Handler
        , protected AzToolsFramework::EditorEvents::Bus::Handler
        , public AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler
    {
    public:
        AZ_COMPONENT_DECL(BoxO3DEEditorSystemComponent)

        static void Reflect(AZ::ReflectContext* context);

        BoxO3DEEditorSystemComponent() = default;
        BoxO3DEEditorSystemComponent(const BoxO3DEEditorSystemComponent&) = delete;
        BoxO3DEEditorSystemComponent& operator=(const BoxO3DEEditorSystemComponent&) = delete;

    private:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        // AZ::Component
        void Activate() override;
        void Deactivate() override;
        
        // Physics::EditorWorldBus overrides...
        AzPhysics::SceneHandle GetEditorSceneHandle() const override;
        
        // ActionManagerRegistrationNotificationBus overrides ...
        void OnActionRegistrationHook() override;
        void OnActionContextModeBindingHook() override;
        void OnMenuBindingHook() override;

    private:
        // AzToolsFramework::EditorEntityContextNotificationBus overrides...
        void OnStartPlayInEditorBegin() override;
        void OnStopPlayInEditor() override;

        // AztoolsFramework::EditorEvents overrides...
        void NotifyRegisterViews() override;

        AzPhysics::SceneHandle m_editorWorldSceneHandle = AzPhysics::InvalidSceneHandle;

        // Assets related data
        AZStd::vector<AZStd::unique_ptr<AZ::Data::AssetHandler>> m_assetHandlers;

        // Asset builder for Box3D material asset
        EditorMaterialAssetBuilder m_materialAssetBuilder;

        // Box3DEditorJointHelpersInterface m_editorJointHelpersInterface;
    };
} // namespace B3
