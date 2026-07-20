
#pragma once

#include <AzFramework/Physics/Shape.h>
#include <AzFramework/Physics/ShapeConfiguration.h>
#include <AzCore/std/smart_ptr/enable_shared_from_this.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

#include <AzFramework/Physics/Collision/CollisionGroups.h>
#include <AzFramework/Physics/Collision/CollisionLayers.h>

#include <box3d/box3d.h>

namespace Physics
{
    class Material;
}

namespace B3
{
    class Material;
    
    class CylinderShapeConfiguration : public Physics::ShapeConfiguration
    {
    public:
        AZ_CLASS_ALLOCATOR(CylinderShapeConfiguration, AZ::SystemAllocator);
        AZ_RTTI(CylinderShapeConfiguration, "{527F10D5-5C8B-49D2-A20E-6B97E994D6F6}", ShapeConfiguration);
        static void Reflect(AZ::ReflectContext* context);
        CylinderShapeConfiguration(
            float height = Physics::ShapeConstants::DefaultCylinderHeight,
            float radius = Physics::ShapeConstants::DefaultCylinderRadius,
            const AZ::Vector3& scale = Physics::ShapeConstants::DefaultScale,
            AZ::u8 subdivisionCount = Physics::ShapeConstants::DefaultCylinderSubdivisionCount
            );
            

        Physics::ShapeType GetShapeType() const override { return Physics::ShapeType::Cylinder; }
        AZStd::shared_ptr<ShapeConfiguration> Clone() const override
        {
            return AZStd::make_shared<CylinderShapeConfiguration>(*this);
        }
        
        // AZ::Capsule ToCylinder(const AZ::Transform& transform = AZ::Transform::CreateIdentity()) const;
        
        float m_height = Physics::ShapeConstants::DefaultCapsuleHeight;
        float m_radius = Physics::ShapeConstants::DefaultCapsuleRadius;
        AZ::u8 m_subdivisionCount = Physics::ShapeConstants::DefaultCylinderSubdivisionCount;

    // private:
    //     void OnHeightChanged();
    //     void OnRadiusChanged();
    };

    class Shape
        : public Physics::Shape
        , public AZStd::enable_shared_from_this<Shape>
    {
    public:
        AZ_CLASS_ALLOCATOR(Shape, AZ::SystemAllocator);
        AZ_RTTI(Shape, "{7A79E4C1-2FD4-4709-9838-CFBA7CD94FEA}", Physics::Shape);

        Shape(const Physics::ColliderConfiguration& colliderConfiguration, const Physics::ShapeConfiguration& configuration);
        virtual ~Shape();

        Shape(Shape&& shape);
        Shape& operator=(Shape&& shape);
        Shape(const Shape& shape) = delete;
        Shape& operator=(const Shape& shape) = delete;

        // Physics::Shape overrides...
        void SetMaterial(const AZStd::shared_ptr<Physics::Material>& material) override;
        AZStd::shared_ptr<Physics::Material> GetMaterial() const override;
        Physics::MaterialId GetMaterialId() const override;
        void SetCollisionLayer(const AzPhysics::CollisionLayer& layer) override;
        AzPhysics::CollisionLayer GetCollisionLayer() const override;
        void SetCollisionGroup(const AzPhysics::CollisionGroup& group) override;
        AzPhysics::CollisionGroup GetCollisionGroup() const override;
        void SetName(const char* name) override;
        void SetLocalPose(const AZ::Vector3& offset, const AZ::Quaternion& rotation) override;
        AZStd::pair<AZ::Vector3, AZ::Quaternion> GetLocalPose() const override;
        float GetRestOffset() const override;
        float GetContactOffset() const override;
        void SetRestOffset(float restOffset) override;
        void SetContactOffset(float contactOffset) override;
        void* GetNativePointer() override;
        const void* GetNativePointer() const override;
        AZ::Crc32 GetTag() const override;
        void AttachedToActor(void* attachedBody) override; //!< Constructs final shape and attaches to provided body
        void DetachedFromActor() override; //!< Deletes the shape from the body
        AzPhysics::SceneQueryHit RayCast(const AzPhysics::RayCastRequest& worldSpaceRequest, const AZ::Transform& worldTransform) override;
        AzPhysics::SceneQueryHit RayCastLocal(const AzPhysics::RayCastRequest& localSpaceRequest) override;
        AZ::Aabb GetAabb(const AZ::Transform& worldTransform) const override;
        AZ::Aabb GetAabbLocal() const override;
        AZStd::shared_ptr<Physics::ShapeConfiguration> GetShapeConfiguration() const override;
        void GetGeometry(AZStd::vector<AZ::Vector3>& vertices, AZStd::vector<AZ::u32>& indices,
            const AZ::Aabb* optionalBounds = nullptr) const override;

        // physx::PxShape* GetPxShape();
        b3ShapeId GetShapeId() const;

        void SetBox3DMaterials(const AZStd::vector<AZStd::shared_ptr<B3::Material>>& materials);
        const AZStd::vector<AZStd::shared_ptr<B3::Material>>& GetBox3DMaterials();

        bool IsTrigger() const;

    private:
        void BindMaterialsWithBox3DShape();
        void ExtractMaterialsFromBox3DShape();
        // physx::PxScene* GetScene() const;
        AzPhysics::SceneQueryHit RayCastInternal(const AzPhysics::RayCastRequest& worldSpaceRequest, const b3WorldTransform& pose);

        // using Box3DShapeUniquePtr = AZStd::unique_ptr<b3ShapeId, AZStd::function<void(b3ShapeId*)>>;
        Shape() = default;

        AZStd::unique_ptr<b3ShapeId> m_box3DShapePtr;
        b3ShapeId m_shapeId = b3_nullShapeId;
        b3ShapeDef m_shapeDef;
        AZStd::vector<AZStd::shared_ptr<B3::Material>> m_materials;
        AzPhysics::CollisionLayer m_collisionLayer;
        AzPhysics::CollisionGroup m_collisionGroup;
        AZStd::shared_ptr<Physics::ShapeConfiguration> m_shapeConfiguration;
        AZStd::shared_ptr<Physics::ColliderConfiguration> m_colliderConfiguration;
        AZ::Crc32 m_tag;
        AZStd::string m_name;
        b3BodyId m_attachedBody = b3_nullBodyId;
    };
}
