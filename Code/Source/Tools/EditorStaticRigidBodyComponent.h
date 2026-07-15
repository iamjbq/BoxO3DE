
#pragma once

#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

namespace B3
{
    //! Class for in-editor Box3D Static Rigid Body Component.
    class EditorStaticRigidBodyComponent : public AzToolsFramework::Components::EditorComponentBase
    {
    public:
        AZ_EDITOR_COMPONENT(
            EditorStaticRigidBodyComponent, "{795B1DE7-1FDF-4716-B868-9A1050A06F11}", AzToolsFramework::Components::EditorComponentBase);
        static void Reflect(AZ::ReflectContext* context);

        EditorStaticRigidBodyComponent() = default;
        ~EditorStaticRigidBodyComponent() = default;

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        // EditorComponentBase
        void BuildGameEntity(AZ::Entity* gameEntity) override;
    };
} // namespace B3
