
#pragma once

#include <Clients/BaseColliderComponent.h>
#include <AzCore/Component/Component.h>
#include <AzFramework/Physics/ShapeConfiguration.h>

namespace B3
{
    /// Component that provides box shape collider used with a Box3D body.
    class BoxColliderComponent
        : public BaseColliderComponent
    {
    public:
        using Configuration = Physics::BoxShapeConfiguration;
        AZ_COMPONENT(BoxColliderComponent, "{1630B5F1-A3C8-42D6-B172-468EA09C40B1}", BaseColliderComponent);
        static void Reflect(AZ::ReflectContext* context);

        BoxColliderComponent() = default;

        // BaseColliderComponent
        void UpdateScaleForShapeConfigs() override;
    };
}
