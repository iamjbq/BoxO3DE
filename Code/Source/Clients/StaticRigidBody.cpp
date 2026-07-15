
#include <Clients/StaticRigidBody.h>

#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/utility/as_const.h>
#include <AzFramework/Physics/Configuration/StaticRigidBodyConfiguration.h>
#include <Utils.h>
#include <BoxO3DE/Utils.h>
#include <Clients/Shape.h>
#include <BoxO3DE/NativeTypeIdentifiers.h>
#include <Common/Box3DSceneQueryHelpers.h>
#include <BoxO3DE/MathConversions.h>

namespace B3
{
    AZ_CLASS_ALLOCATOR_IMPL(B3::StaticRigidBody, AZ::SystemAllocator);

    StaticRigidBody::StaticRigidBody(const AzPhysics::StaticRigidBodyConfiguration& configuration, b3WorldId& worldId)
        :m_worldId(worldId)
    {
        CreateBox3DBody(configuration);
    }

    StaticRigidBody::~StaticRigidBody()
    {
        //clean up the body, attached shapes, and userdata
        if(b3Body_IsValid(m_bodyId))
        {
            // Invalidate user data so it sets m_bodyIdPtr user data to nullptr.
            // It's appropriate to do this as m_bodyIdPtr is a shared pointer, and
            // technically it could survive m_bodyUserData life's spam.
            m_bodyUserData.Invalidate();
            
            b3DestroyBody(m_bodyId); // Shapes and joints are automatically destroyed with the body
        }
        m_shapes.clear();
    }

    void StaticRigidBody::CreateBox3DBody(const AzPhysics::StaticRigidBodyConfiguration& configuration)
    {
        if (b3Body_IsValid(m_bodyId))
        {
            AZ_Warning("Box3D Rigid Body", false, "Trying to create Box3D rigid actor when it's already created");
            return;
        }
        
        b3BodyDef newBodyDef = b3DefaultBodyDef();
        
        newBodyDef.type = b3BodyType::b3_staticBody;
        newBodyDef.position = Box3DMathConvert(configuration.m_position);
        newBodyDef.rotation = Box3DMathConvert(configuration.m_orientation.GetNormalized());
        newBodyDef.name = configuration.m_debugName.c_str();
        newBodyDef.isEnabled = configuration.m_startSimulationEnabled;
        
        m_bodyDef = newBodyDef; // Not sure if this should be cached
        
        m_bodyId = b3CreateBody(m_worldId, &m_bodyDef);

        if (b3Body_IsValid(m_bodyId))
        {
            m_bodyUserData = BodyData(m_bodyId);
            m_bodyUserData.SetRigidBodyStatic(this);
            m_bodyUserData.SetEntityId(configuration.m_entityId);
            m_debugName = configuration.m_debugName;

            if (configuration.m_customUserData)
            {
                SetUserData(configuration.m_customUserData);
            }
        }
    }

    void StaticRigidBody::AddShape(AZStd::shared_ptr<Physics::Shape> shape)
    {
        auto box3DShape = AZStd::rtti_pointer_cast<B3::Shape>(shape);
        
        if (!box3DShape)
        {
            AZ_Error("Box3D Rigid Body", false, "Trying to add a shape of unknown type. Name: %s", GetName().c_str());
            return;
        }
        
        box3DShape->AttachedToActor(&m_bodyId);
        m_shapes.push_back(box3DShape);
    }

    AZStd::shared_ptr<Physics::Shape> StaticRigidBody::GetShape(AZ::u32 index)
    {
        AZStd::shared_ptr<const Physics::Shape> constShape = AZStd::as_const(*this).GetShape(index);
        return AZStd::const_pointer_cast<Physics::Shape>(constShape);
    }

    AZStd::shared_ptr<const Physics::Shape> StaticRigidBody::GetShape(AZ::u32 index) const
    {
        if (index >= m_shapes.size())
        {
            return nullptr;
        }
        return m_shapes[index];
    }

    AZ::u32 StaticRigidBody::GetShapeCount() const
    {
        return static_cast<AZ::u32>(m_shapes.size());
    }

    AZ::Transform StaticRigidBody::GetTransform() const
    {
        if(b3Body_IsValid(m_bodyId))
        {
            return Box3DMathConvert(b3Body_GetTransform(m_bodyId));
        }
        return AZ::Transform::CreateIdentity();
    }

    void StaticRigidBody::SetTransform(const AZ::Transform & transform)
    {
        if(b3Body_IsValid(m_bodyId))
        {
            b3Body_SetTransform(
                m_bodyId,
                Box3DMathConvert(transform.GetTranslation()),
                Box3DMathConvert(transform.GetRotation())
                );
        }
    }

    AZ::Vector3 StaticRigidBody::GetPosition() const
    {
        if(b3Body_IsValid(m_bodyId))
        {
            return Box3DMathConvert(b3Body_GetPosition(m_bodyId));
        }
        return AZ::Vector3::CreateZero();
    }

    AZ::Quaternion StaticRigidBody::GetOrientation() const
    {
        if(b3Body_IsValid(m_bodyId))
        {
            return Box3DMathConvert(b3Body_GetRotation(m_bodyId));
        }
        return  AZ::Quaternion::CreateZero();
    }

    AZ::Aabb StaticRigidBody::GetAabb() const
    {
        if(b3Body_IsValid(m_bodyId))
        {
            return Box3DMathConvert(b3Body_ComputeAABB(m_bodyId));
        }
        return  AZ::Aabb::CreateNull();
    }

    AzPhysics::SceneQueryHit StaticRigidBody::RayCast(const AzPhysics::RayCastRequest& request)
    {
        AZ_UNUSED(request);
        // return B3::SceneQueryHelpers::ClosestRayHitAgainstShapes(request, m_shapes, GetTransform());
        return AzPhysics::SceneQueryHit();
    }

    AZ::EntityId StaticRigidBody::GetEntityId() const
    {
        return m_bodyUserData.GetEntityId();
    }

    AZ::Crc32 StaticRigidBody::GetNativeType() const
    {
        return B3::NativeTypeIdentifiers::RigidBodyStatic;
    }

    void* StaticRigidBody::GetNativePointer() const
    {
        return nullptr;
    }

    void StaticRigidBody::SetName(const AZStd::string& entityName)
    {
        m_debugName = entityName;

        if(b3Body_IsValid(m_bodyId))
        {
            b3Body_SetName(m_bodyId, m_debugName.c_str());
        }
    }

    const AZStd::string& StaticRigidBody::GetName() const
    {
        return m_debugName;
    }
}
