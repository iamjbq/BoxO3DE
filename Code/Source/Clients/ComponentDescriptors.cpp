#include <Clients/ComponentDescriptors.h>
#include <Clients/BoxO3DESystemComponent.h>
#include <Clients/StaticRigidBodyComponent.h>
#include <Clients/RigidBodyComponent.h>
#include <Clients/BaseColliderComponent.h>
#include <Clients/BoxColliderComponent.h>
#include <Clients/SphereColliderComponent.h>
#include <Clients/CapsuleColliderComponent.h>

namespace B3
{
    AZStd::list<AZ::ComponentDescriptor*> GetDescriptors()
    {
        AZStd::list<AZ::ComponentDescriptor*> descriptors =
        {
            BoxO3DESystemComponent::CreateDescriptor(),
            StaticRigidBodyComponent::CreateDescriptor(),
            RigidBodyComponent::CreateDescriptor(),
            BaseColliderComponent::CreateDescriptor(),
            BoxColliderComponent::CreateDescriptor(),
            SphereColliderComponent::CreateDescriptor(),
            CapsuleColliderComponent::CreateDescriptor()
        };

        return descriptors;
    }
}
