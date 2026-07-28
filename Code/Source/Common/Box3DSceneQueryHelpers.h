
#pragma once

#include<AzFramework/Physics/Common/PhysicsSceneQueries.h>

#include <box3d/box3d.h>

// namespace AzPhysics
// {
//     struct RayCastRequest;
// }

namespace B3
{
    class Shape;

    namespace SceneQueryHelpers
    {
        //! Helper function to convert from Box3D hit to AZ.
        AzPhysics::SceneQueryHit GetHitFromBodyCast(const b3CastOutput& box3DHit, const b3BodyId& bodyId, const b3ShapeId& shapeId);
        AzPhysics::SceneQueryHit GetHitFromShapeRayCast(const b3CastOutput& box3DHit, const b3BodyId& bodyId, const b3ShapeId& shapeId);
        
        //! Helper class to bring in filtering and hit results to cast callback functions.
        //! Box3D requires inline filtering during the cast of each hit depending on request flags.
        class WorldCastResultContext
        {
        public:
            WorldCastResultContext(const AzPhysics::RayCastRequest* raycastRequest,
            const AZ::u32 sceneMaxResults,
            AzPhysics::SceneQueryHits& hits);
            
            const AzPhysics::RayCastRequest* m_raycastRequest;
            const AZ::u32 m_sceneMaxResults;
            AzPhysics::SceneQueryHits& m_hits;
            AZ::u32 m_currentResultCount = 0;
        };
    }
}
