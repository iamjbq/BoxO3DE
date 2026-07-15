#include <Clients/ComponentDescriptors.h>
#include <Clients/BoxO3DESystemComponent.h>
#include <Clients/StaticRigidBodyComponent.h>

namespace B3
{
    AZStd::list<AZ::ComponentDescriptor*> GetDescriptors()
    {
        AZStd::list<AZ::ComponentDescriptor*> descriptors =
        {
            BoxO3DESystemComponent::CreateDescriptor(),
            StaticRigidBodyComponent::CreateDescriptor()
        };

        return descriptors;
    }
}
