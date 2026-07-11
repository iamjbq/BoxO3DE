
#pragma once

#include<AzFramework/Physics/Common/PhysicsSceneQueries.h>

#include <box3d/box3d.h>

namespace B3
{
    class Shape;

    namespace SceneQueryHelpers
    {
        //! Helper function to convert from Box3D hit to AZ.
        AzPhysics::SceneQueryHit GetHitFromPxHit(const b3CastOutput& box3DHit, const b3BodyId& bodyId, const b3ShapeId& shapeId);
    }
}
