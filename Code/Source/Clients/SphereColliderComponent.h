
#pragma once

#include <Clients/BaseColliderComponent.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Math/Vector3.h>

namespace B3
{
    /// Component that provides sphere shape collider.
    /// May be used in conjunction with a Box3D Rigid Body Component to create a dynamic rigid body.
    class SphereColliderComponent
        : public BaseColliderComponent
    {
    public:
        using Configuration = Physics::SphereShapeConfiguration;
        AZ_COMPONENT(SphereColliderComponent, "{549630A1-66DD-47A4-B0E9-D1E3B7B3C4CD}", BaseColliderComponent);
        static void Reflect(AZ::ReflectContext* context);

        SphereColliderComponent() = default;

        // BaseColliderComponent
        void UpdateScaleForShapeConfigs() override;
    };
}
