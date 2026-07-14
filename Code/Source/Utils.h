
#pragma once

#include <AzCore/Component/Entity.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Vector3.h>
#include <AzFramework/Physics/Material/PhysicsMaterialSlots.h>
#include <AzFramework/Physics/Shape.h>
#include <AzFramework/Physics/ShapeConfiguration.h>
#include <AzCore/std/optional.h>

#include <box3d/box3d.h>

namespace AzPhysics
{
    class CollisionGroup;
    struct RigidBodyConfiguration;
    struct StaticRigidBodyConfiguration;
    struct StaticRigidBody;
    class Scene;
}

namespace Physics
{
    class RigidBodyConfiguration;
    class ColliderConfiguration;
    class ShapeConfiguration;
}

namespace B3
{
    class Shape;
    class BodyData;

    namespace Utils
    {
        b3ShapeId CreateBox3DShapeFromConfig(
            const Physics::ColliderConfiguration& colliderConfiguration,
            const Physics::ShapeConfiguration& shapeConfiguration,
            const b3ShapeDef& shapeDef, 
            const b3BodyId& bodyId
        );
    }
}
