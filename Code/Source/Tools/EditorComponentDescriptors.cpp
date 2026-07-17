#include <Tools/EditorComponentDescriptors.h>
#include <Tools/BoxO3DEEditorSystemComponent.h>
#include <Tools/System/Box3DEditorSettingsRegistryManager.h>
#include <Tools/EditorStaticRigidBodyComponent.h>
#include <Tools/EditorRigidBodyComponent.h>

namespace B3
{
    AZStd::list<AZ::ComponentDescriptor*> GetEditorDescriptors()
    {
        AZStd::list<AZ::ComponentDescriptor*> descriptors =
        {
            BoxO3DEEditorSystemComponent::CreateDescriptor(),
            BoxO3DESystemComponent::CreateDescriptor(),
            EditorStaticRigidBodyComponent::CreateDescriptor(),
            EditorRigidBodyComponent::CreateDescriptor()
        };

        return descriptors;
    }
}
