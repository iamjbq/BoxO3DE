
#pragma once

#include <AzCore/std/containers/list.h>

namespace AZ
{
    class ComponentDescriptor;
}

namespace B3
{
    AZStd::list<AZ::ComponentDescriptor*> GetDescriptors();
} // namespace B3
