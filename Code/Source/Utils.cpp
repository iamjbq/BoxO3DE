#include <Utils.h>

#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Component/NonUniformScaleBus.h>
#include <AzCore/Casting/lossy_cast.h>
#include <AzCore/EBus/Results.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/SimdMath.h>
#include <AzCore/Math/MathStringConversions.h>
#include <AzFramework/Physics/ShapeConfiguration.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Physics/Collision/CollisionGroups.h>
#include <AzFramework/Physics/Collision/CollisionLayers.h>
#include <AzFramework/Physics/Configuration/RigidBodyConfiguration.h>
#include <AzFramework/Physics/Configuration/StaticRigidBodyConfiguration.h>
#include <AzFramework/Physics/PhysicsScene.h>
#include <AzFramework/Physics/PhysicsSystem.h>
#include <AzFramework/Physics/SimulatedBodies/StaticRigidBody.h>
#include <AzFramework/Physics/HeightfieldProviderBus.h>

#include <Clients/BoxO3DESystemComponent.h>
#include <Clients/Shape.h>
#include <BoxO3DE/Material/Box3DMaterialConfiguration.h>
#include <BoxO3DE/MathConversions.h>
#include <System/Box3DSystem.h>

namespace B3
{
    namespace Utils
    {
        b3ShapeId CreateBox3DShapeFromConfig(const Physics::ColliderConfiguration& colliderConfiguration,
            const Physics::ShapeConfiguration& shapeConfiguration, AzPhysics::CollisionGroup& assignedCollisionGroup)
        {
            AZ_UNUSED_3(colliderConfiguration, shapeConfiguration, assignedCollisionGroup);
            // b3ShapeDef shapeDef = b3DefaultShapeDef();
            return b3_nullShapeId;
            // if (!Utils::CreatePxGeometryFromConfig(shapeConfiguration, pxGeomHolder))
            // {
            //     return nullptr;
            // }
            //
            // AZStd::vector<AZStd::shared_ptr<Material>> materials = Material::FindOrCreateMaterials(colliderConfiguration.m_materialSlots);
            // AZStd::vector<const physx::PxMaterial*> pxMaterials(materials.size(), nullptr);
            // for (size_t materialIndex = 0; materialIndex < materials.size(); ++materialIndex)
            // {
            //     pxMaterials[materialIndex] = materials[materialIndex]->GetPxMaterial();
            // }
            //
            // physx::PxShape* shape = PxGetPhysics().createShape(
            //     pxGeomHolder.any(),
            //     const_cast<physx::PxMaterial**>(pxMaterials.data()),
            //     static_cast<physx::PxU16>(pxMaterials.size()),
            //     colliderConfiguration.m_isExclusive);
            // if (!shape)
            // {
            //     AZ_Error("PhysX Rigid Body", false, "Failed to create shape.");
            //     return nullptr;
            // }
            //
            // AzPhysics::CollisionGroup collisionGroup;
            // Physics::CollisionRequestBus::BroadcastResult(collisionGroup, &Physics::CollisionRequests::GetCollisionGroupById, colliderConfiguration.m_collisionGroupId);
            //
            // physx::PxFilterData filterData = PhysX::Collision::CreateFilterData(colliderConfiguration.m_collisionLayer, collisionGroup);
            // shape->setSimulationFilterData(filterData);
            // shape->setQueryFilterData(filterData);
            //
            // // Do custom logic for specific shape types
            // if (pxGeomHolder.getType() == physx::PxGeometryType::eCAPSULE)
            // {
            //     // PhysX capsules are oriented around x by default.
            //     physx::PxQuat pxQuat(AZ::Constants::HalfPi, physx::PxVec3(0.0f, 1.0f, 0.0f));
            //     shape->setLocalPose(physx::PxTransform(pxQuat));
            // }
            // else if (pxGeomHolder.getType() == physx::PxGeometryType::eHEIGHTFIELD)
            // {
            //     const Physics::HeightfieldShapeConfiguration& heightfieldConfig =
            //         static_cast<const Physics::HeightfieldShapeConfiguration&>(shapeConfiguration);
            //
            //     // PhysX heightfields have the origin at the corner, not the center, so add an offset to the passed-in transform
            //     // to account for this difference.
            //     const AZ::Vector2 gridSpacing = heightfieldConfig.GetGridResolution();
            //     AZ::Vector3 offset(
            //         -(gridSpacing.GetX() * heightfieldConfig.GetNumColumnSquares() / 2.0f),
            //         -(gridSpacing.GetY() * heightfieldConfig.GetNumRowSquares() / 2.0f),
            //         0.0f);
            //
            //     // PhysX heightfields are always defined to have the height in the Y direction, not the Z direction, so we need
            //     // to provide additional rotations to make it Z-up.
            //     physx::PxQuat pxQuat = PxMathConvert(
            //         AZ::Quaternion::CreateFromEulerAnglesRadians(AZ::Vector3(AZ::Constants::HalfPi, AZ::Constants::HalfPi, 0.0f)));
            //     physx::PxTransform pxHeightfieldTransform = physx::PxTransform(PxMathConvert(offset), pxQuat);
            //     shape->setLocalPose(pxHeightfieldTransform);
            // }
            //
            // // Handle a possible misconfiguration when a shape is set to be both simulated & trigger. This is illegal in PhysX.
            // shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, colliderConfiguration.m_isSimulated && !colliderConfiguration.m_isTrigger);
            // shape->setFlag(physx::PxShapeFlag::eSCENE_QUERY_SHAPE, colliderConfiguration.m_isInSceneQueries);
            // shape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, colliderConfiguration.m_isTrigger);
            //
            // shape->setRestOffset(colliderConfiguration.m_restOffset);
            // shape->setContactOffset(colliderConfiguration.m_contactOffset);
            //
            // physx::PxTransform pxShapeTransform = PxMathConvert(colliderConfiguration.m_position, colliderConfiguration.m_rotation);
            // shape->setLocalPose(pxShapeTransform * shape->getLocalPose());
            //
            // assignedCollisionGroup = collisionGroup;
            // return shape;
        }
    }
}
