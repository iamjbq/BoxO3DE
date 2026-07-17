
#pragma once

#include <Clients/BaseColliderComponent.h>
#include <AzCore/Component/Component.h>

namespace B3
{
    /// Component that provides capsule shape collider.
    /// May be used in conjunction with a Box3D Rigid Body Component to create a dynamic rigid body.
    class CapsuleColliderComponent
        : public BaseColliderComponent
    {
    public:
        using Configuration = Physics::CapsuleShapeConfiguration;
        AZ_COMPONENT(CapsuleColliderComponent, "{8E55107C-8FCF-43B9-88FC-F7E4F6B85CC0}", BaseColliderComponent);
        static void Reflect(AZ::ReflectContext* context);

        CapsuleColliderComponent() = default;

        // BaseColliderComponent
        void UpdateScaleForShapeConfigs() override;
    };
}
