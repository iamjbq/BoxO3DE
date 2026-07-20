
#include <Clients/Shape.h>

#include <AzFramework/Physics/Common/PhysicsSceneQueries.h>
#include <AzFramework/Physics/Material/PhysicsMaterial.h>
#include <AzFramework/Physics/ShapeConfiguration.h>
#include <AzFramework/Physics/Material/PhysicsMaterialManager.h>
#include <AzFramework/Physics/Collision/CollisionGroups.h>
#include <AzFramework/Physics/Collision/CollisionLayers.h>

#include <Common/Box3DSceneQueryHelpers.h>
#include <BoxO3DE//Utils.h>
#include <BoxO3DE/Material/Box3DMaterial.h>
// #include <Source/Collision.h>
#include <Utils.h>
#include <AzFramework/Physics/CollisionBus.h>
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

    void CylinderShapeConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext
                ->RegisterGenericType<AZStd::shared_ptr<CylinderShapeConfiguration>>();
            
            serializeContext->Class<CylinderShapeConfiguration>()
                ->Version(1)
                ->Field("Subdivision", &CylinderShapeConfiguration::m_subdivisionCount)
                ->Field("Height", &CylinderShapeConfiguration::m_height)
                ->Field("Radius", &CylinderShapeConfiguration::m_radius)
            ;

            if (auto* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<CylinderShapeConfiguration>("CylinderShapeConfiguration", "Configuration for cylinder collider")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &CylinderShapeConfiguration::m_subdivisionCount,
                        "Subdivision", "Cylinder subdivision count.")
                        ->Attribute(AZ::Edit::Attributes::Min, Utils::MinFrustumSubdivisions)
                        ->Attribute(AZ::Edit::Attributes::Max, Utils::MaxFrustumSubdivisions)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &CylinderShapeConfiguration::m_height, "Height", "Cylinder height.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &CylinderShapeConfiguration::m_radius, "Radius", "Cylinder radius.")
                    ;
            }
        }
    }

    CylinderShapeConfiguration::CylinderShapeConfiguration(float height, float radius, const AZ::Vector3& scale,
        AZ::u8 subdivisionCount)
            : m_height(height)
            , m_radius(radius)
            , ShapeConfiguration(scale)
            , m_subdivisionCount(subdivisionCount)
    
    {
    }

    // AZ::Capsule CylinderShapeConfiguration::ToCylinder(const AZ::Transform& transform) const
    // {
    // }

    Shape::Shape(Shape&& shape)
        // : m_box3DShapePtr(AZStd::move(shape.m_box3DShapePtr))
        : m_shapeId(AZStd::move(shape.m_shapeId))
        , m_materials(AZStd::move(shape.m_materials))
        , m_collisionLayer(AZStd::move(shape.m_collisionLayer))
        , m_collisionGroup(AZStd::move(shape.m_collisionGroup))
    {
        if (b3Shape_IsValid(m_shapeId))
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

        if (b3Shape_IsValid(m_shapeId))
        {
            b3Shape_SetUserData(m_shapeId, this);
        }

        return *this;
    }

    Shape::Shape(const Physics::ColliderConfiguration& colliderConfiguration, const Physics::ShapeConfiguration& shapeConfiguration)
        : m_collisionLayer(colliderConfiguration.m_collisionLayer)
    {
        m_shapeConfiguration = shapeConfiguration.Clone();
        m_colliderConfiguration = AZStd::make_shared<Physics::ColliderConfiguration>(colliderConfiguration);
        m_tag = AZ::Crc32(colliderConfiguration.m_tag);
        
        Physics::CollisionRequestBus::BroadcastResult(m_collisionGroup, &Physics::CollisionRequests::GetCollisionGroupById, colliderConfiguration.m_collisionGroupId);
        
        AZStd::vector<AZStd::shared_ptr<Material>> materials = Material::FindOrCreateMaterials(colliderConfiguration.m_materialSlots); // Have access to materialId here
        AZStd::vector<b3SurfaceMaterial> box3DMaterials(materials.size());
        for (size_t materialIndex = 0; materialIndex < materials.size(); ++materialIndex)
        {
            box3DMaterials[materialIndex] = materials[materialIndex]->GetNativeMaterial();
        }
        
        b3Filter collisionFilter = b3DefaultFilter();
        collisionFilter.categoryBits = colliderConfiguration.m_collisionLayer.GetMask();
        collisionFilter.maskBits = m_collisionGroup.GetMask();
        
        b3ShapeDef newShapeDef = b3DefaultShapeDef();
        newShapeDef.userData = this;
        if (shapeConfiguration.GetShapeType() == Physics::ShapeType::TriangleMesh || shapeConfiguration.GetShapeType() == Physics::ShapeType::CookedMesh)
        {
            newShapeDef.materials = box3DMaterials.data();
            newShapeDef.materialCount = static_cast<int>(box3DMaterials.size());
        }
        else
        {
            newShapeDef.baseMaterial = box3DMaterials.front();
            newShapeDef.density = materials.front()->GetDensity();
        }
        newShapeDef.explosionScale = 1.0f;
        newShapeDef.filter = collisionFilter;
        newShapeDef.isSensor = colliderConfiguration.m_isTrigger;
        newShapeDef.enableContactEvents = true; // TODO: future configuration on component
        newShapeDef.enableSensorEvents = true;
        newShapeDef.enableHitEvents = true;
        newShapeDef.updateBodyMass = true;
        
        m_shapeDef = newShapeDef;
    }

    Shape::~Shape()
    {
        // Shapes in Box3D are automatically deleted when the body is.
        if (b3Shape_IsValid(m_shapeId))
        {
            b3Shape_SetUserData(m_shapeId, nullptr);
        }
        m_attachedBody = b3_nullBodyId;
        m_colliderConfiguration.reset();
        m_shapeConfiguration.reset();
    }

    b3ShapeId Shape::GetShapeId() const
    {
        return m_shapeId;
    }

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
        b3Filter filter = b3Shape_GetFilter(m_shapeId);
        filter.categoryBits = layer.GetMask();
        b3Shape_SetFilter(m_shapeId, filter, true);
    }

    AzPhysics::CollisionLayer Shape::GetCollisionLayer() const
    {
        return m_collisionLayer;
    }

    void Shape::SetCollisionGroup(const AzPhysics::CollisionGroup& group)
    {
        m_collisionGroup = group;
        b3Filter filter = b3Shape_GetFilter(m_shapeId);
        filter.maskBits = group.GetMask();
        b3Shape_SetFilter(m_shapeId, filter, true);
    }

    AzPhysics::CollisionGroup Shape::GetCollisionGroup() const
    {
        return m_collisionGroup;
    }

    void Shape::SetName(const char* name)
    {
        m_name = name;
    }

    void Shape::SetLocalPose(const AZ::Vector3& offset, const AZ::Quaternion& rotation)
    {
        switch (b3Shape_GetType(m_shapeId))
        {
        case b3ShapeType::b3_sphereShape:
            {
                b3Sphere sphere = b3Shape_GetSphere(m_shapeId);
                sphere.center = Box3DMathConvert(offset); // rotation doesn't change a sphere
                b3Shape_SetSphere(m_shapeId, &sphere);
                break;
            }
        case b3ShapeType::b3_capsuleShape:
            {
                b3Capsule capsule = b3Shape_GetCapsule(m_shapeId);
                float halfHeight = b3Distance(capsule.center1, capsule.center2) * 0.5f;
                AZ::Vector3 axis = rotation.TransformVector(AZ::Vector3::CreateAxisZ());
                capsule.center1 = Box3DMathConvert(offset - axis * halfHeight);
                capsule.center2 = Box3DMathConvert(offset + axis * halfHeight);
                b3Shape_SetCapsule(m_shapeId, &capsule);
                break;
            }
        case b3ShapeType::b3_hullShape:
            {
                b3HullData* hull = b3CloneAndTransformHull(b3Shape_GetHull(m_shapeId), Box3DMathConvert(offset, rotation), b3Vec3_one);
                b3Shape_SetHull(m_shapeId, hull);
                break;
            }
        default:
            {
                AZ_Warning("Physics::Shape", false, "Cannot set local pose. Unknown or unsupported shape type.");
                break;
            }
        }
        
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
        switch (b3Shape_GetType(m_shapeId))
        {
        case b3ShapeType::b3_sphereShape:
            {
                return { Box3DMathConvert(b3Shape_GetSphere(m_shapeId).center), AZ::Quaternion::CreateIdentity() };
            }
        case b3ShapeType::b3_capsuleShape:
            {
                b3Capsule capsule = b3Shape_GetCapsule(m_shapeId);
                AZ::Vector3 center = Box3DMathConvert(capsule.center1 + capsule.center2) * 0.5f;
                AZ::Vector3 axis = Box3DMathConvert(capsule.center1 - capsule.center2).GetNormalized();
                return { center, AZ::Quaternion::CreateShortestArc(AZ::Vector3::CreateAxisZ(), axis) };
            }
        case b3ShapeType::b3_hullShape:
            {
                // const b3HullData* hull = b3Shape_GetHull(m_shapeId);
                // TODO: not sure yet how to get it
                return { AZ::Vector3::CreateZero(), AZ::Quaternion::CreateIdentity() };
            }
        default:
            {
                AZ_Warning("Physics::Shape", false, "Cannot set local pose. Unknown or unsupported shape type.");
                return { AZ::Vector3::CreateZero(), AZ::Quaternion::CreateIdentity() };
            }
        }
        // physx::PxTransform pose = m_box3DShapePtr->getLocalPose();
        // if (m_box3DShapePtr->getGeometry().getType() == physx::PxGeometryType::eCAPSULE)
        // {
        //     physx::PxQuat PxTolyRotation(-AZ::Constants::HalfPi, physx::PxVec3(0.0f, 1.0f, 0.0f));
        //     pose.q *= PxTolyRotation;
        // }
        // return { PxMathConvert(pose.p), PxMathConvert(pose.q) };
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
        return m_box3DShapePtr.get();
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
        if (b3Shape_IsValid(m_shapeId))
        {
            return b3Shape_IsSensor(m_shapeId);
        }
        return false;
    }

    void Shape::
    AttachedToActor(void* attachedBody)
    {
        // This is where the shape is actually created
        b3BodyId* bodyId = static_cast<b3BodyId*>(attachedBody);
        if (b3Body_IsValid(*bodyId))
        {
            m_attachedBody = *bodyId;

            m_shapeId = Utils::CreateBox3DShapeFromConfig(*m_colliderConfiguration, *m_shapeConfiguration, m_shapeDef, m_attachedBody);
            m_box3DShapePtr = AZStd::make_unique<b3ShapeId>(m_shapeId);
        }
    }

    void Shape::DetachedFromActor()
    {
        b3DestroyShape(m_shapeId, true);
        m_box3DShapePtr.reset();
    }

    AzPhysics::SceneQueryHit Shape::RayCastInternal(const AzPhysics::RayCastRequest& worldSpaceRequest, const b3WorldTransform& pose)
    {
        AZ_UNUSED(pose)
        if (const bool shouldCollide = worldSpaceRequest.m_collisionGroup.GetMask() & m_collisionLayer.GetMask();
            !shouldCollide)
        {
            return AzPhysics::SceneQueryHit();
        }

        const b3Vec3 start = Box3DMathConvert(worldSpaceRequest.m_start);
        const b3Vec3 translation = Box3DMathConvert(worldSpaceRequest.m_direction) * worldSpaceRequest.m_distance;
        // const AZ::u32 maxHits = 1;
        // const physx::PxHitFlags hitFlags = SceneQueryHelpers::GetPxHitFlags(worldSpaceRequest.m_hitFlags);
        
        // b3QueryFilter filter = b3DefaultQueryFilter();
        // b3RayResult result = b3World_CastRayClosest(b3Shape_GetWorld(m_shapeId), start, translation, filter);
        b3WorldCastOutput result = b3Shape_RayCast(m_shapeId, start, translation);
        
        if (result.hit)
        {
            AzPhysics::SceneQueryHit hit;
            
            hit.m_distance = b3Distance(result.point, start);
            hit.m_resultFlags |= AzPhysics::SceneQuery::ResultFlags::Distance;
            
            hit.m_position = Box3DMathConvert(result.point);
            hit.m_resultFlags |= AzPhysics::SceneQuery::ResultFlags::Position;
            
            hit.m_normal = Box3DMathConvert(result.normal);
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
            
            hit.m_shape = Utils::GetUserData(m_shapeId);
            if (hit.m_shape != nullptr)
            {
                hit.m_resultFlags |= AzPhysics::SceneQuery::ResultFlags::Shape;
            }
            
            b3SurfaceMaterial material = b3Shape_GetMeshSurfaceMaterial(m_shapeId, result.triangleIndex);
            
            if (material.userMaterialId != 0)
            {
                
                // AZ::Interface<Physics::MaterialManager>::Get()->GetMaterial(material.userMaterialId);
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

    AZ::Aabb Shape::GetAabb([[maybe_unused]] const AZ::Transform& worldTransform) const
    {
        return Box3DMathConvert(b3Shape_GetAABB(m_shapeId));
    }

    AZ::Aabb Shape::GetAabbLocal() const
    {
        // TODO: Some way to convert to lcoal
        return Box3DMathConvert(b3Shape_GetAABB(m_shapeId));
    }

    AZStd::shared_ptr<Physics::ShapeConfiguration> Shape::GetShapeConfiguration() const
    {
        return m_shapeConfiguration;
    }

    void Shape::GetGeometry(AZStd::vector<AZ::Vector3>& vertices, AZStd::vector<AZ::u32>& indices, const AZ::Aabb* optionalBounds) const
    {
        AZ_UNUSED_3(vertices, indices, optionalBounds);
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
