
#pragma once

#include <AzFramework/Physics/SimulatedBodies/StaticRigidBody.h>
#include <AzFramework/Physics/Shape.h>
#include <BoxO3DE/UserDataTypes.h>

#include <box3d/box3d.h>

namespace AzPhysics
{
    struct StaticRigidBodyConfiguration;
}

namespace B3
{
    class Shape;

    class StaticRigidBody
        : public AzPhysics::StaticRigidBody
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL;
        AZ_RTTI(B3::StaticRigidBody, "{AFC068DF-DB36-4E08-8725-C5A927E4CDDF}", AzPhysics::StaticRigidBody);

        StaticRigidBody() = default;
        StaticRigidBody(const AzPhysics::StaticRigidBodyConfiguration& configuration, b3WorldId& worldId);
        ~StaticRigidBody();

        // AzPhysics::StaticRigidBody
        void AddShape(AZStd::shared_ptr<Physics::Shape> shape) override;
        AZStd::shared_ptr<Physics::Shape> GetShape(AZ::u32 index) override;
        AZStd::shared_ptr<const Physics::Shape> GetShape(AZ::u32 index) const override;
        AZ::u32 GetShapeCount() const override;

        // AzPhysics::SimulatedBody
        AZ::EntityId GetEntityId() const override;

        AZ::Transform GetTransform() const override;
        void SetTransform(const AZ::Transform& transform) override;

        AZ::Vector3 GetPosition() const override;
        AZ::Quaternion GetOrientation() const override;

        AZ::Aabb GetAabb() const override;
        AzPhysics::SceneQueryHit RayCast(const AzPhysics::RayCastRequest& request) override;

        virtual AZ::Crc32 GetNativeType() const override;
        virtual void* GetNativePointer() const override;
        
        void SetName(const AZStd::string& entityName);
        const AZStd::string& GetName() const;

    private:
        void CreateBox3DBody(const AzPhysics::StaticRigidBodyConfiguration& configuration);

        b3BodyId m_bodyId = b3_nullBodyId;
        AZStd::shared_ptr<b3BodyId> m_bodyIdPtr;
        b3WorldId m_worldId = b3_nullWorldId;
        b3BodyDef m_bodyDef;
        AZStd::vector<AZStd::shared_ptr<B3::Shape>> m_shapes;
        B3::BodyData m_bodyUserData;
        AZStd::string m_debugName;
    };
} // namespace B3
