
#pragma once

#include <BoxO3DE/BoxO3DETypeIds.h>

#include <AzCore/EBus/EBus.h>
#include <AzCore/Interface/Interface.h>

namespace B3
{
    class BoxO3DERequests
    {
    public:
        AZ_RTTI(BoxO3DERequests, BoxO3DERequestsTypeId);
        virtual ~BoxO3DERequests() = default;
        // Put your public methods here
    };

    class BoxO3DEBusTraits
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////
    };

    using BoxO3DERequestBus = AZ::EBus<BoxO3DERequests, BoxO3DEBusTraits>;
    using BoxO3DEInterface = AZ::Interface<BoxO3DERequests>;

} // namespace B3
