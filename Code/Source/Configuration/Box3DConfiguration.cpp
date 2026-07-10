
#include <BoxO3DE/Configuration/Box3DConfiguration.h>

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace B3
{
    namespace B3Internal
    {
        AzPhysics::CollisionConfiguration CreateDefaultCollisionConfiguration()
        {
            AzPhysics::CollisionConfiguration configuration;
            configuration.m_collisionLayers.SetName(AzPhysics::CollisionLayer::Default, "Default");

            configuration.m_collisionGroups.CreateGroup("All", AzPhysics::CollisionGroup::All, AzPhysics::CollisionGroups::Id(), true);
            configuration.m_collisionGroups.CreateGroup("None", AzPhysics::CollisionGroup::None, AzPhysics::CollisionGroups::Id::Create(), true);

            return configuration;
        }

        bool Box3DSystemConfigurationConverter([[maybe_unused]] AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& dataElement)
        {
            if (dataElement.GetVersion() <= 1)
            {
                dataElement.RemoveElementByName(AZ_CRC_CE("DefaultMaterialLibrary"));
                AZ_Warning("Box3DSystemConfigurationConverter", false,
                    "Old version of Box3D Configuration data found. 'DefaultMaterialLibrary' element removed."); 
            }

            return true;
        }
    }
    
    AZ_CLASS_ALLOCATOR_IMPL(CapacityConfiguration, AZ::SystemAllocator);
    AZ_CLASS_ALLOCATOR_IMPL(Box3DSystemConfiguration, AZ::SystemAllocator);
    
    
    /*static*/ void CapacityConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<B3::CapacityConfiguration>()
                ->Version(1)
                ->Field("MaxStaticBodies", &CapacityConfiguration::m_maxStaticBodies)
                ->Field("MaxDynamicBodies", &CapacityConfiguration::m_maxDynamicBodies)
                ->Field("MaxStaticShapes", &CapacityConfiguration::m_maxStaticShapes)
                ->Field("MaxDynamicShapes", &CapacityConfiguration::m_maxDynamicShapes)
                ->Field("MaxContacts", &CapacityConfiguration::m_maxContacts);

            if (AZ::EditContext* editContext = serialize->GetEditContext())
            {
                editContext->Class<B3::CapacityConfiguration>("Capacity Configuration", "Expected simulation limits.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &CapacityConfiguration::m_maxStaticBodies, "Max Static Bodies",
                        "A reasonable value might be 2048-4096 for a more densely populated static world.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &CapacityConfiguration::m_maxDynamicBodies, "Max Dynamic Bodies",
                        "A reasonable value might be 256-512.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &CapacityConfiguration::m_maxStaticShapes, "Max Static Shapes",
                        "A reasonable value might be 2048-4096 for a more densely populated static world.\n"
                        "Static bodies can have compound shapes.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &CapacityConfiguration::m_maxDynamicShapes, "Max Dynamic Shapes",
                        "This should be slightly higher than your max dynamic bodies. Note, not all actors/bodies have shapes. Bodies can have compound shapes.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &CapacityConfiguration::m_maxContacts, "Max Contacts",
                        "This is the maximum number of body contacts");
            }
        }
    }

    bool CapacityConfiguration::operator==(const CapacityConfiguration& other) const
    {
        return m_maxStaticBodies == other.m_maxStaticBodies &&
            m_maxDynamicBodies == other.m_maxDynamicBodies &&
            m_maxStaticShapes == other.m_maxStaticShapes &&
            m_maxDynamicShapes == other.m_maxDynamicShapes &&
            m_maxContacts == other.m_maxContacts;
    }

    bool CapacityConfiguration::operator!=(const CapacityConfiguration& other) const
    {
        return !(*this == other);
    }

    /*static*/ void Box3DSystemConfiguration::Reflect(AZ::ReflectContext* context)
    {
        AzPhysics::SystemConfiguration::Reflect(context);
        CapacityConfiguration::Reflect(context);

        if (auto* serializeContext = azdynamic_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<B3::Box3DSystemConfiguration, AzPhysics::SystemConfiguration>()
                ->Version(1, &B3Internal::Box3DSystemConfigurationConverter)
                ->Field("CapacityConfiguration", &Box3DSystemConfiguration::m_capacityConfiguration);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                // this is needed so the edit context of AzPhysics::SystemConfiguration can be used.
                editContext->Class<B3::Box3DSystemConfiguration>("System Configuration", "Box3D system configuration")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true);
            }
        }
    }

    /*static*/ Box3DSystemConfiguration Box3DSystemConfiguration::CreateDefault()
    {
        Box3DSystemConfiguration systemConfig;
        systemConfig.m_collisionConfig = B3Internal::CreateDefaultCollisionConfiguration();
        return systemConfig;
    }

    bool Box3DSystemConfiguration::operator==(const Box3DSystemConfiguration& other) const
    {
        return AzPhysics::SystemConfiguration::operator==(other) &&
            m_capacityConfiguration == other.m_capacityConfiguration;
    }

    bool Box3DSystemConfiguration::operator!=(const Box3DSystemConfiguration& other) const
    {
        return !(*this == other);
    }
}
