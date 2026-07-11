#include <Common/Box3DSceneQueryHelpers.h>

#include <AzCore/std/limits.h>

#include <Clients/Shape.h>

namespace B3
{
    namespace SceneQueryHelpers
    {
        AzPhysics::SceneQueryHit GetHitFromPxHit(const b3CastOutput& box3DHit, const b3BodyId& bodyId,
            const b3ShapeId& shapeId)
        {
            AzPhysics::SceneQueryHit hit;
            return hit;
        }
    }
}
