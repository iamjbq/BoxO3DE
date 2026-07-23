
#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzFramework/Physics/Configuration/SystemConfiguration.h>

#include <BoxO3DE/Debug/Box3DDebugConfiguration.h>

namespace AZ
{
    class ReflectContext;
}

namespace B3
{
    //! Box3D body, shape, and collision limits.
    class CapacityConfiguration
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL
        AZ_TYPE_INFO(B3::CapacityConfiguration, "{89C9559A-4714-47D4-A860-96F714591CBF}");
        static void Reflect(AZ::ReflectContext* context);
        
        int m_maxStaticBodies = 4096; //!< Expected max static bodies in a scene.

        int m_maxDynamicBodies = 512; //!< Expected max dynamic bodies in a scene.

        int m_maxStaticShapes = 4096; //!< Expected max static shapes in a scene.

        int m_maxDynamicShapes = 512; //!< Expected max dynamic shapes in a scene.
        
        int m_maxContacts = 16384; //!< Expected max body contact points in a scene.

        bool operator==(const CapacityConfiguration& other) const;
        bool operator!=(const CapacityConfiguration& other) const;
    };

    //! Contains global physics settings.
    //! Used to initialize the Physics System.
    struct Box3DSystemConfiguration : public AzPhysics::SystemConfiguration
    {
        AZ_CLASS_ALLOCATOR_DECL;
        AZ_RTTI(B3::Box3DSystemConfiguration, "{BDB35142-E64B-4D39-9108-CE4B1ECED921}");
        static void Reflect(AZ::ReflectContext* context);

        static Box3DSystemConfiguration CreateDefault();
        
        CapacityConfiguration m_capacityConfiguration; //!< Capacity configuration for Box3D.

        bool operator==(const Box3DSystemConfiguration& other) const;
        bool operator!=(const Box3DSystemConfiguration& other) const;
    };
}
