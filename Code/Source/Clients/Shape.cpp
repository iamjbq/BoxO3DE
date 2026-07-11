
#include <Clients/Shape.h>

#include <AzFramework/Physics/Common/PhysicsSceneQueries.h>
#include <AzFramework/Physics/Material/PhysicsMaterial.h>
#include <AzFramework/Physics/ShapeConfiguration.h>

// #include <Common/PhysXSceneQueryHelpers.h>
#include <BoxO3DE//Utils.h>
#include <BoxO3DE/Material/Box3DMaterial.h>
// #include <Source/Collision.h>
// #include <Source/Utils.h>
#include <BoxO3DE/MathConversions.h>

namespace B3
{
    namespace ShapeConstants
    {
        // 48 is the number of stacks/slices used when generating mesh geo for spheres in legacy physics
        // we default to these values for consistency
        constexpr size_t NumStacks = 48;
        constexpr size_t NumSlices = 48;
    }

    Shape::Shape(Shape&& shape)
        // : m_box3DShapePtr(AZStd::move(shape.m_box3DShapePtr))
        : m_shapeId(AZStd::move(shape.m_shapeId))
        , m_materials(AZStd::move(shape.m_materials))
        , m_collisionLayer(AZStd::move(shape.m_collisionLayer))
        , m_collisionGroup(AZStd::move(shape.m_collisionGroup))
    {
        if (B3_IS_NON_NULL(m_shapeId))
        {
            b3Shape_SetUserData(m_shapeId, this);
        }
    }

    Shape& Shape::operator=(Shape&& shape)
    {
        // m_box3DShapePtr = AZStd::move(shape.m_box3DShapePtr);
        m_shapeId = AZStd::move(shape.m_shapeId);
        m_materials = AZStd::move(shape.m_materials);
        m_collisionLayer = AZStd::move(shape.m_collisionLayer);
        m_collisionGroup = AZStd::move(shape.m_collisionGroup);

        if (B3_IS_NON_NULL(m_shapeId))
        {
            b3Shape_SetUserData(m_shapeId, this);
        }

        return *this;
    }

    // void Shape::ReleasePxShape(physx::PxShape* shape)
    // {
    //     if (shape != nullptr)
    //     {
    //         PHYSX_SCENE_WRITE_LOCK(GetScene());
    //         shape->userData = nullptr;
    //         shape->release();
    //     }
    // }

    Shape::Shape(const Physics::ColliderConfiguration& colliderConfiguration, const Physics::ShapeConfiguration& shapeConfiguration)
        : m_collisionLayer(colliderConfiguration.m_collisionLayer)
    {
        m_shapeConfiguration = shapeConfiguration.Clone();
        if (physx::PxShape* newShape = Utils::CreatePxShapeFromConfig(colliderConfiguration, shapeConfiguration, m_collisionGroup)) // TODO: the biggie
        {
            // m_box3DShapePtr = Box3DShapeUniquePtr(newShape, AZStd::bind(&Shape::ReleasePxShape, this, newShape));

            ExtractMaterialsFromBox3DShape();

            m_tag = AZ::Crc32(colliderConfiguration.m_tag);
        }
    }

    Shape::Shape(b3ShapeId nativeShape)
    {
        // m_box3DShapePtr = Box3DShapeUniquePtr(nativeShape, AZStd::bind(&Shape::ReleasePxShape, this, nativeShape));

        ExtractMaterialsFromBox3DShape();
    }

    Shape::~Shape()
    {
        // Shapes in Box3D are automatically deleted when the body is.
        // m_box3DShapePtr.reset();
        // m_box3DShapePtr = nullptr;
        
        if (B3_IS_NON_NULL(m_shapeId))
        {
            b3Shape_SetUserData(m_shapeId, nullptr);
        }
        m_attachedBody = b3_nullBodyId;
    }

    // physx::PxShape* Shape::GetPxShape()
    // {
    //     if (m_box3DShapePtr)
    //     {
    //         return m_box3DShapePtr.get();
    //     }
    //     return nullptr;
    // }

    void Shape::SetMaterial(const AZStd::shared_ptr<Physics::Material>& material)
    {
        if (auto materialWrapper = AZStd::rtti_pointer_cast<B3::Material>(material))
        {
            m_materials.clear();
            m_materials.emplace_back(materialWrapper);

            BindMaterialsWithBox3DShape();
        }
        else
        {
            AZ_Warning("Box3D Shape", false, "Trying to assign material of unknown type");
        }
    }

    AZStd::shared_ptr<Physics::Material> Shape::GetMaterial() const
    {
        if (!m_materials.empty())
        {
            return m_materials[0];
        }
        return nullptr;
    }

    Physics::MaterialId Shape::GetMaterialId() const
    {
        if (!m_materials.empty())
        {
            return m_materials[0]->GetId();
        }

        return {};
    }

    void Shape::SetBox3DMaterials(const AZStd::vector<AZStd::shared_ptr<B3::Material>>& materials)
    {
        m_materials = materials;

        BindMaterialsWithBox3DShape();
    }

    const AZStd::vector<AZStd::shared_ptr<B3::Material>>& Shape::GetBox3DMaterials()
    {
        return m_materials;
    }

    void Shape::BindMaterialsWithBox3DShape()
    {
        // if (m_box3DShapePtr)
        // {
        //     AZStd::vector<const physx::PxMaterial*> pxMaterials;
        //     pxMaterials.reserve(m_materials.size());
        //
        //     for (const auto& material : m_materials)
        //     {
        //         pxMaterials.emplace_back(material->GetPxMaterial());
        //     }
        //
        //     AZ_Warning("Box3D Shape", m_materials.size() < std::numeric_limits<AZ::u16>::max(), "Trying to assign too many materials, cutting down");
        //     size_t materialsCount = AZStd::GetMin(m_materials.size(), static_cast<size_t>(std::numeric_limits<AZ::u16>::max()));
        //
        //     {
        //         PHYSX_SCENE_WRITE_LOCK(GetScene());
        //         m_box3DShapePtr->setMaterials(const_cast<physx::PxMaterial**>(pxMaterials.data()), static_cast<physx::PxU16>(materialsCount));
        //     }
        // }
    }

    void Shape::ExtractMaterialsFromBox3DShape()
    {
        // if (m_box3DShapePtr == nullptr)
        // {
        //     return;
        // }
        // const int BufferSize = 100;
        //
        // AZ_Warning("Box3D Shape", m_box3DShapePtr->getNbMaterials() < BufferSize, "Shape has too many materials, consider increasing the buffer");
        //
        // physx::PxMaterial* assignedMaterials[BufferSize];
        // int materialsCount = m_box3DShapePtr->getMaterials(assignedMaterials, BufferSize, 0);

        // m_materials.clear();
        // m_materials.reserve(materialsCount);
        //
        // for (int i = 0; i < materialsCount; ++i)
        // {
        //     if (assignedMaterials[i]->userData == nullptr)
        //     {
        //         AZ_Error("Box3D Shape", false, "Trying to assign material with no user data. Make sure you are creating materials using MaterialManager");
        //         continue;
        //     }
        //
        //     AZStd::shared_ptr<B3::Material> physxMaterial = static_cast<B3::Material*>(B3::Utils::GetUserData(assignedMaterials[i]))->shared_from_this();
        //     if (!physxMaterial)
        //     {
        //         AZ_Error("Box3D Shape", false, "Invalid user data of a physx material. Make sure you are creating materials using MaterialManager");
        //         continue;
        //     }
        //
        //     m_materials.push_back(physxMaterial);
        // }
    }

    void Shape::SetCollisionLayer(const AzPhysics::CollisionLayer& layer)
    {
        m_collisionLayer = layer;

        // physx::PxFilterData filterData = m_box3DShapePtr->getSimulationFilterData();
        // Collision::SetLayer(layer, filterData);
        // m_box3DShapePtr->setSimulationFilterData(filterData);
        // m_box3DShapePtr->setQueryFilterData(filterData);
    }

    AzPhysics::CollisionLayer Shape::GetCollisionLayer() const
    {
        return m_collisionLayer;
    }

    void Shape::SetCollisionGroup(const AzPhysics::CollisionGroup& group)
    {
        m_collisionGroup = group;
        
        // physx::PxFilterData filterData = m_box3DShapePtr->getSimulationFilterData();
        // Collision::SetGroup(m_collisionGroup, filterData);
        // m_box3DShapePtr->setSimulationFilterData(filterData);
        // m_box3DShapePtr->setQueryFilterData(filterData);
    }

    AzPhysics::CollisionGroup Shape::GetCollisionGroup() const
    {
        return m_collisionGroup;
    }

    void Shape::SetName(const char* name)
    {
        
    }

    void Shape::SetLocalPose(const AZ::Vector3& offset, const AZ::Quaternion& rotation)
    {
        // physx::PxTransform pxShapeTransform = PxMathConvert(offset, rotation);
        // AZ_Warning("Physics::Shape", m_box3DShapePtr->isExclusive(), "Non-exclusive shapes are not mutable after they're attached to a body.");
        // if (m_box3DShapePtr->getGeometry().getType() == physx::PxGeometryType::eCAPSULE)
        // {
        //     physx::PxQuat lyToPxRotation(AZ::Constants::HalfPi, physx::PxVec3(0.0f, 1.0f, 0.0f));
        //     pxShapeTransform.q *= lyToPxRotation;
        // }
        // m_box3DShapePtr->setLocalPose(pxShapeTransform);
    }

    AZStd::pair<AZ::Vector3, AZ::Quaternion> Shape::GetLocalPose() const
    {
        // physx::PxTransform pose = m_box3DShapePtr->getLocalPose();
        // if (m_box3DShapePtr->getGeometry().getType() == physx::PxGeometryType::eCAPSULE)
        // {
        //     physx::PxQuat PxTolyRotation(-AZ::Constants::HalfPi, physx::PxVec3(0.0f, 1.0f, 0.0f));
        //     pose.q *= PxTolyRotation;
        // }
        // return { PxMathConvert(pose.p), PxMathConvert(pose.q) };
        return { AZ::Vector3::CreateZero(), AZ::Quaternion::CreateIdentity() };
    }

    float Shape::GetRestOffset() const
    {
        return 0.0f;
    }

    float Shape::GetContactOffset() const
    {
        return 0.0f;
    }

    void Shape::SetRestOffset(float restOffset)
    {
        float contactOffset = GetContactOffset();
        if (restOffset >= contactOffset)
        {
            AZ_Error("Box3D Shape", false, "Requested rest offset (%e) must be less than contact offset (%e).",
                restOffset, contactOffset);
            return;
        }
        // m_box3DShapePtr->setRestOffset(restOffset);
    }

    void Shape::SetContactOffset(float contactOffset)
    {
        if (contactOffset <= 0.0f)
        {
            AZ_Error("Box3D Shape", false, "Requested contact offset (%e) must exceed 0.");
            return;
        }

        float restOffset = GetRestOffset();
        if (contactOffset <= restOffset)
        {
            AZ_Error("Box3D Shape", false, "Requested contact offset (%e) must exceed rest offset (%e).",
                contactOffset, restOffset);
            return;
        }
        // m_box3DShapePtr->setContactOffset(contactOffset);
    }

    void* Shape::GetNativePointer()
    {
        return nullptr;
    }

    const void* Shape::GetNativePointer() const
    {
        return nullptr;
    }

    AZ::Crc32 Shape::GetTag() const
    {
        return m_tag;
    }

    bool Shape::IsTrigger() const
    {
        if (B3_IS_NON_NULL(m_shapeId))
        {
            return b3Shape_IsSensor(m_shapeId);
        }
        return false;
    }

    void Shape::AttachedToActor(void* actor)
    {
        b3BodyId* bodyId = static_cast<b3BodyId*>(actor);
        if (b3Body_IsValid(*bodyId))
        {
            m_attachedBody = *bodyId;
        }
    }

    void Shape::DetachedFromActor()
    {
        m_attachedBody = b3_nullBodyId;
    }

    AzPhysics::SceneQueryHit Shape::RayCastInternal(const AzPhysics::RayCastRequest& worldSpaceRequest, const b3WorldTransform& pose)
    {
        if (const bool shouldCollide = worldSpaceRequest.m_collisionGroup.GetMask() & m_collisionLayer.GetMask();
            !shouldCollide)
        {
            return AzPhysics::SceneQueryHit();
        }

        const b3Vec3 start = Box3DMathConvert(worldSpaceRequest.m_start);
        const b3Vec3 translation = Box3DMathConvert(worldSpaceRequest.m_direction) * worldSpaceRequest.m_distance;
        // const AZ::u32 maxHits = 1;
        // const physx::PxHitFlags hitFlags = SceneQueryHelpers::GetPxHitFlags(worldSpaceRequest.m_hitFlags);
        
        b3QueryFilter filter = b3DefaultQueryFilter();
        b3BodyCastResult output = b3Body_CastRay(m_attachedBody, start, translation, filter, 1.0f, pose);
        
        if (output.hit)
        {
            AzPhysics::SceneQueryHit hit;
            
            hit.m_distance = b3Distance(output.point, start);
            hit.m_resultFlags |= AzPhysics::SceneQuery::ResultFlags::Distance;
            
            hit.m_position = Box3DMathConvert(output.point);
            hit.m_resultFlags |= AzPhysics::SceneQuery::ResultFlags::Position;
            
            hit.m_normal = Box3DMathConvert(output.normal);
            hit.m_resultFlags |= AzPhysics::SceneQuery::ResultFlags::Normal;
            
            const BodyData* bodyData = Utils::GetUserData(m_attachedBody);
            hit.m_bodyHandle = bodyData->GetBodyHandle();
            if (hit.m_bodyHandle != AzPhysics::InvalidSimulatedBodyHandle)
            {
                hit.m_resultFlags |= AzPhysics::SceneQuery::ResultFlags::BodyHandle;
            }
            
            hit.m_entityId = bodyData->GetEntityId();
            if (hit.m_entityId.IsValid())
            {
                hit.m_resultFlags |= AzPhysics::SceneQuery::ResultFlags::EntityId;
            }
            
            hit.m_shape = Utils::GetUserData(output.shapeId);
            if (hit.m_shape != nullptr)
            {
                hit.m_resultFlags |= AzPhysics::SceneQuery::ResultFlags::Shape;
            }
            
            // TODO: how materials are accessed depends on shape type (mesh, heightfield, or simple)
            if (output.userMaterialId != 0)
            {
                // somehow get Box3D material by id on b3SurfaceMaterial
            }
            else if (hit.m_shape != nullptr)
            {
                hit.m_physicsMaterialId = hit.m_shape->GetMaterialId();
            }
            else
            {
                hit.m_resultFlags |= AzPhysics::SceneQuery::ResultFlags::Material;
            }
            
            return hit;
        }
        return AzPhysics::SceneQueryHit();
    }

    AzPhysics::SceneQueryHit Shape::RayCast(const AzPhysics::RayCastRequest& worldSpaceRequest, const AZ::Transform& worldTransform)
    {
        return RayCastInternal(worldSpaceRequest, Box3DMathConvert(worldTransform));
    }

    AzPhysics::SceneQueryHit Shape::RayCastLocal(const AzPhysics::RayCastRequest& localSpaceRequest)
    {
        b3MassData data = b3Shape_ComputeMassData(m_shapeId);
        b3WorldTransform worldTransform = b3Body_GetTransform(m_attachedBody);
        
        b3Pos position = b3TransformWorldPoint(worldTransform, data.center);
        return RayCastInternal(localSpaceRequest, worldTransform); // TODO: properly get local pose
    }

    AZ::Aabb Shape::GetAabb(const AZ::Transform& worldTransform) const
    {
        return Box3DMathConvert(b3Shape_GetAABB(m_shapeId));
    }

    AZ::Aabb Shape::GetAabbLocal() const
    {
        // TODO: Some way to convert to lcoal
        return Box3DMathConvert(b3Shape_GetAABB(m_shapeId));
    }

    // physx::PxScene* Shape::GetScene() const
    // {
    //     if (b3Body_IsValid(m_attachedBody))
    //     {
    //         return m_attachedBody->getScene();
    //     }
    //     return nullptr;
    // }

    AZStd::shared_ptr<Physics::ShapeConfiguration> Shape::GetShapeConfiguration() const
    {
        return m_shapeConfiguration;
    }

    void Shape::GetGeometry(AZStd::vector<AZ::Vector3>& vertices, AZStd::vector<AZ::u32>& indices, const AZ::Aabb* optionalBounds) const
    {
        // if (!m_box3DShapePtr)
        // {
        //     return;
        // }
        //
        // PHYSX_SCENE_READ_LOCK(GetScene());
        //
        // // PhysX 5.6.1 change, only need to get this reference once
        // const physx::PxGeometry& baseGeometry = m_box3DShapePtr->getGeometry();
        //
        // if (baseGeometry.getType() == physx::PxGeometryType::eTRIANGLEMESH)
        // {
        //     const auto& geometry = static_cast<const physx::PxTriangleMeshGeometry&>(baseGeometry);
        //     if (geometry.triangleMesh && geometry.isValid())
        //     {
        //         Utils::Geometry::GetTriangleMeshGeometry(geometry, vertices, indices);
        //     }
        // }
        // else if (baseGeometry.getType() == physx::PxGeometryType::eCONVEXMESH)
        // {
        //     const auto& geometry = static_cast<const physx::PxConvexMeshGeometry&>(baseGeometry);
        //     if (geometry.convexMesh && geometry.isValid())
        //     {
        //         Utils::Geometry::GetConvexMeshGeometry(geometry, vertices, indices);
        //     }
        // }
        // else if (baseGeometry.getType() == physx::PxGeometryType::eHEIGHTFIELD)
        // {
        //     const auto& geometry = static_cast<const physx::PxHeightFieldGeometry&>(baseGeometry);
        //     if (geometry.heightField && geometry.isValid())
        //     {
        //         Utils::Geometry::GetHeightFieldGeometry(geometry, vertices, indices, optionalBounds);
        //     }
        // }
        // else if (baseGeometry.getType() == physx::PxGeometryType::eBOX)
        // {
        //     const auto& geometry = static_cast<const physx::PxBoxGeometry&>(baseGeometry);
        //     if (geometry.isValid())
        //     {
        //         Utils::Geometry::GetBoxGeometry(geometry, vertices, indices);
        //     }
        // }
        // else if (baseGeometry.getType() == physx::PxGeometryType::eSPHERE)
        // {
        //     const auto& geometry = static_cast<const physx::PxSphereGeometry&>(baseGeometry);
        //     if (geometry.isValid())
        //     {
        //         Utils::Geometry::GetSphereGeometry(geometry, vertices, indices, ShapeConstants::NumStacks, ShapeConstants::NumSlices);
        //     }
        // }
        // else if (baseGeometry.getType() == physx::PxGeometryType::eCAPSULE)
        // {
        //     const auto& geometry = static_cast<const physx::PxCapsuleGeometry&>(baseGeometry);
        //     if (geometry.isValid())
        //     {
        //         Utils::Geometry::GetCapsuleGeometry(geometry, vertices, indices, ShapeConstants::NumStacks, ShapeConstants::NumSlices);
        //     }
        // }
        // else
        // {
        //     AZ_TracePrintf("Shape", "GetGeometry for PxGeometryType %d is not supported", static_cast<int>(m_box3DShapePtr->getGeometry().getType()));
        // }
    }
}
