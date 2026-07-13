#include <Common/Box3DSceneQueryHelpers.h>

#include <AzCore/std/limits.h>

#include <Clients/Shape.h>

namespace B3
{
    namespace SceneQueryHelpers
    {
        AzPhysics::SceneQueryHit GetHitFromBodyCast(const b3BodyCastResult& box3DHit, const b3BodyId& bodyId,
            const b3ShapeId& shapeId)
        {
            AZ_UNUSED_3(box3DHit, bodyId, shapeId)
            AzPhysics::SceneQueryHit hit;
            return hit;
        }

        AzPhysics::SceneQueryHit GetHitFromShapeRayCast(const b3CastOutput& box3DHit, const b3BodyId& bodyId,
            const b3ShapeId& shapeId)
        {
            AZ_UNUSED_3(box3DHit, bodyId, shapeId)
            AzPhysics::SceneQueryHit hit;
            return hit;
        }
    }
}
