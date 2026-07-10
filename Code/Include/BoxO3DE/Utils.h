
#pragma once

#include <BoxO3DE/UserDataTypes.h>
#include <AzFramework/Physics/Shape.h>
#include <AzFramework/Physics/Common/PhysicsSceneQueries.h>

namespace AzPhysics
{
    class CollisionLayer;
    class CollisionGroup;
    class Scene;
}

namespace Physics
{
    class Material;
    class Shape;
}

namespace B3
{
    // class Shape;

    namespace Utils
    {
        BodyData* GetUserData(const b3BodyId bodyId);
        // Physics::Material* GetUserData(const physx::PxMaterial* material);
        Physics::Shape* GetUserData(const b3ShapeId shapeId);
        AzPhysics::Scene* GetUserData(b3WorldId worldId);

        namespace Collision
        {
            // AZ::u64 Combine(AZ::u32 word0, AZ::u32 word1);
            // void SetLayer(const AzPhysics::CollisionLayer& layer, physx::PxFilterData& filterData);
            // void SetGroup(const AzPhysics::CollisionGroup& group, physx::PxFilterData& filterData);
            // void SetCollisionLayerAndGroup(physx::PxShape* shape, const AzPhysics::CollisionLayer& layer, const AzPhysics::CollisionGroup& group);
            // bool ShouldCollide(const physx::PxFilterData& filterData0, const physx::PxFilterData& filterData1);
        }
    }
}

#include <BoxO3DE/Utils.inl>
