
#pragma once

#include <Utils.h>
#include <AzFramework/Physics/SimulatedBodies/RigidBody.h>
#include <AzFramework/Physics/Common/PhysicsTypes.h>
#include <BoxO3DE/UserDataTypes.h>

#include <box3d/box3d.h>

namespace AzPhysics
{
    struct RigidBodyConfiguration;
}

namespace B3
{
    class RigidBodyComponent;
    class Shape;

    //! Box3D-specific settings which are not generic enough to be stored in the AzPhysics rigid body configuration.
    class RigidBodyConfiguration
    {
    public:
        AZ_CLASS_ALLOCATOR(RigidBodyConfiguration, AZ::SystemAllocator);
        AZ_RTTI(B3::RigidBodyConfiguration, "{EFF86F72-E5D4-43C4-8078-2165C97ABD61}");

        RigidBodyConfiguration() = default;
        virtual ~RigidBodyConfiguration() = default;

        static void Reflect(AZ::ReflectContext* context);
        //
        // AZ::u8 m_solverPositionIterations = 4; //!< Higher values can improve stability at the cost of performance.
        // AZ::u8 m_solverVelocityIterations = 1; //!< Higher values can improve stability at the cost of performance.
    };

    AZ_PUSH_DISABLE_WARNING(4996, "-Wdeprecated-declarations")

    /// Box3D specific implementation of generic physics API RigidBody class.
    class RigidBody
        : public AzPhysics::RigidBody
    {
    public:
        friend class RigidBodyComponent;

        AZ_CLASS_ALLOCATOR(RigidBody, AZ::SystemAllocator);
        AZ_RTTI(B3::RigidBody, "{240E1881-841E-4F33-9178-B773122EC754}", AzPhysics::RigidBody);

        RigidBody() = default;
        RigidBody(const AzPhysics::RigidBodyConfiguration& configuration, b3WorldId& worldId);
        ~RigidBody();

        static void Reflect(AZ::ReflectContext* context);

        AZ::u32 GetShapeCount() const override;
        AZStd::shared_ptr<Physics::Shape> GetShape(AZ::u32 index) override;
        AZStd::shared_ptr<const Physics::Shape> GetShape(AZ::u32 index) const override;

        AZ::Vector3 GetCenterOfMassWorld() const override;
        AZ::Vector3 GetCenterOfMassLocal() const override;

        AZ::Matrix3x3 GetInertiaWorld() const override;
        AZ::Matrix3x3 GetInertiaLocal() const override;
        AZ::Matrix3x3 GetInverseInertiaWorld() const override;
        AZ::Matrix3x3 GetInverseInertiaLocal() const override;

        float GetMass() const override;
        float GetInverseMass() const override;
        void SetMass(float mass) override;
        void SetCenterOfMassOffset(const AZ::Vector3& comOffset) override;
        AZ::Vector3 GetLinearVelocity() const override;
        void SetLinearVelocity(const AZ::Vector3& velocity) override;
        AZ::Vector3 GetAngularVelocity() const override;
        void SetAngularVelocity(const AZ::Vector3& angularVelocity) override;
        AZ::Vector3 GetLinearVelocityAtWorldPoint(const AZ::Vector3& worldPoint) const override;
        void ApplyLinearImpulse(const AZ::Vector3& impulse) override;
        void ApplyLinearImpulseAtWorldPoint(const AZ::Vector3& impulse, const AZ::Vector3& worldPoint) override;
        void ApplyAngularImpulse(const AZ::Vector3& angularImpulse) override;

        bool IsKinematic() const override;
        void SetKinematic(bool isKinematic) override;
        void SetKinematicTarget(const AZ::Transform& targetPosition) override;

        bool IsGravityEnabled() const override;
        void SetGravityEnabled(bool enabled) override;
        void SetSimulationEnabled(bool enabled) override;
        void SetCCDEnabled(bool enabled) override;

        // AzPhysics::SimulatedBody
        AZ::Transform GetTransform() const override;
        void SetTransform(const AZ::Transform& transform) override;
        AZ::Vector3 GetPosition() const override;
        AZ::Quaternion GetOrientation() const override;
        AZ::Aabb GetAabb() const override; //!< This will return empty if the body has no shapes attached.
        AZ::EntityId GetEntityId() const override;

        AzPhysics::SceneQueryHit RayCast(const AzPhysics::RayCastRequest& request) override;

        // Physics::ReferenceBase
        AZ::Crc32 GetNativeType() const override;
        void* GetNativePointer() const override;

        // Not in API but needed to support PhysicsComponentBus
        float GetLinearDamping() const override;
        void SetLinearDamping(float damping) override;
        float GetAngularDamping() const override;
        void SetAngularDamping(float damping) override;

        //sleeping
        bool IsAwake() const override;
        void ForceAsleep() override; //!< Warning, this will put the whole island of bodies touching this body to sleep and is expensive.
        void ForceAwake() override; //!< Warning, this will wake the whole island of bodies touching this body.
        float GetSleepThreshold() const override;
        void SetSleepThreshold(float threshold) override;

        bool ShouldStartAsleep() const { return m_startAsleep; }

        void SetName(const AZStd::string& entityName);
        const AZStd::string& GetName() const;

        void AddShape(AZStd::shared_ptr<Physics::Shape> shape) override;
        void RemoveShape(AZStd::shared_ptr<Physics::Shape> shape) override;

        void UpdateMassProperties(AzPhysics::MassComputeFlags flags = AzPhysics::MassComputeFlags::DEFAULT,
            const AZ::Vector3& centerOfMassOffsetOverride = AZ::Vector3::CreateZero(),
            const AZ::Matrix3x3& inertiaTensorOverride = AZ::Matrix3x3::CreateIdentity(),
            const float massOverride = 1.0f) override;

    private:
        void CreateBox3DBody(const AzPhysics::RigidBodyConfiguration& configuration);

        void UpdateCenterOfMass(bool includeAllShapesInMassCalculation);
        void SetInertia(const AZ::Matrix3x3& inertia);

        b3BodyId m_bodyId = b3_nullBodyId;
        b3WorldId m_worldId = b3_nullWorldId;
        b3BodyDef m_bodyDef;
        AZStd::vector<AZStd::shared_ptr<B3::Shape>> m_shapes;
        AZStd::string m_name;
        B3::BodyData m_bodyUserData;
        bool m_startAsleep = false;
    };

    AZ_POP_DISABLE_WARNING
} // namespace B3
