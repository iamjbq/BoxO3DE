
#include <Clients/RigidBody.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/utility/as_const.h>
#include <AzCore/Math/MathStringConversions.h>
#include <AzFramework/Physics/Utils.h>
#include <AzFramework/Physics/Configuration/RigidBodyConfiguration.h>
#include <BoxO3DE/NativeTypeIdentifiers.h>
#include <BoxO3DE/MathConversions.h>
#include <Utils.h>
#include <BoxO3DE/Utils.h>
#include <Clients/Shape.h>
#include <BoxO3DE/Material/Box3DMaterial.h>
#include <Common/Box3DSceneQueryHelpers.h>

#include "System/Box3DSystem.h"

namespace B3
{
    namespace
    {
        const AZ::Vector3 DefaultCenterOfMass = AZ::Vector3::CreateZero();
        const float DefaultMass = 1.0f;
        const AZ::Matrix3x3 DefaultInertiaTensor = AZ::Matrix3x3::CreateIdentity();

        bool IsSimulationShape(const b3ShapeId& box3DShape)
        {
            return !b3Shape_IsSensor(box3DShape);
        }
 
        bool CanShapeComputeMassProperties(const b3ShapeId& box3DShape)
        {
            b3ShapeType type = b3Shape_GetType(box3DShape);
            return type == b3_sphereShape
                || type == b3_hullShape
                || type == b3_capsuleShape;
        }
    }

    void RigidBodyConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<B3::RigidBodyConfiguration>()
                ->Version(1)
                // ->Field("SolverPositionIterations", &B3::RigidBodyConfiguration::m_solverPositionIterations)
                // ->Field("SolverVelocityIterations", &B3::RigidBodyConfiguration::m_solverVelocityIterations)
                ;

            if (auto* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<B3::RigidBodyConfiguration>("Box3D-specific Rigid Body Configuration",
                    "Additional Rigid Body settings specific to Box3D.")
                    // ->DataElement(
                    //     AZ::Edit::UIHandlers::Default,
                    //     &B3::RigidBodyConfiguration::m_solverPositionIterations,
                    //     "Solver Position Iterations",
                    //     "Higher values can improve stability at the cost of performance.")
                    // ->Attribute(AZ::Edit::Attributes::Min, 1)
                    // ->Attribute(AZ::Edit::Attributes::Max, 255)
                    // ->DataElement(
                    //     AZ::Edit::UIHandlers::Default,
                    //     &B3::RigidBodyConfiguration::m_solverVelocityIterations,
                    //     "Solver Velocity Iterations",
                    //     "Higher values can improve stability at the cost of performance.")
                    // ->Attribute(AZ::Edit::Attributes::Min, 1)
                    // ->Attribute(AZ::Edit::Attributes::Max, 255)
                    ;
            }
        }
    }

    void RigidBody::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<RigidBody>()
                ->Version(1)
            ;
        }
    }

    RigidBody::RigidBody(const AzPhysics::RigidBodyConfiguration& configuration, b3WorldId& worldId)
        : m_worldId(worldId)
        , m_startAsleep(configuration.m_startAsleep)
    {
        CreateBox3DBody(configuration);
    }

    RigidBody::~RigidBody()
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
        m_bodyIdPtr.reset();
    }

    void RigidBody::CreateBox3DBody(const AzPhysics::RigidBodyConfiguration& configuration)
    {
        if (b3Body_IsValid(m_bodyId))
        {
            AZ_Warning("Box3D Rigid Body", false, "Trying to create Box3D rigid actor when it's already created");
            return;
        }
        
        b3BodyDef newBodyDef = b3DefaultBodyDef();
        
        configuration.m_kinematic ? newBodyDef.type = b3BodyType::b3_kinematicBody : newBodyDef.type = b3BodyType::b3_dynamicBody;
        newBodyDef.position = Box3DMathConvert(configuration.m_position);
        newBodyDef.rotation = Box3DMathConvert(configuration.m_orientation.GetNormalized());
        newBodyDef.linearVelocity = Box3DMathConvert(configuration.m_initialLinearVelocity);
        newBodyDef.angularVelocity = Box3DMathConvert(configuration.m_initialAngularVelocity);
        newBodyDef.linearDamping = configuration.m_linearDamping;
        newBodyDef.angularDamping = configuration.m_angularDamping;
        configuration.m_gravityEnabled ? newBodyDef.gravityScale = 1.0f : newBodyDef.gravityScale = 0.0f;
        newBodyDef.sleepThreshold = configuration.m_sleepMinEnergy;
        newBodyDef.name = configuration.m_debugName.c_str();
        newBodyDef.isAwake = !m_startAsleep;
        newBodyDef.isEnabled = configuration.m_startSimulationEnabled;
        
        b3MotionLocks motionLocks = {0};
        motionLocks.linearX = configuration.m_lockLinearX;
        motionLocks.linearY = configuration.m_lockLinearY;
        motionLocks.linearZ = configuration.m_lockLinearZ;
        motionLocks.angularX = configuration.m_lockAngularX;
        motionLocks.angularY = configuration.m_lockAngularY;
        motionLocks.angularZ = configuration.m_lockAngularZ;
        
        newBodyDef.motionLocks = motionLocks;
        
        m_bodyDef = newBodyDef; // Not sure if this should be cached
        
        m_bodyId = b3CreateBody(m_worldId, &m_bodyDef);
        m_bodyIdPtr = AZStd::make_shared<b3BodyId>(m_bodyId);

        if (b3Body_IsValid(m_bodyId))
        {
            if (configuration.m_startSimulationEnabled)
            {
                m_simulating = true;
            }
            
            m_bodyUserData = BodyData(m_bodyId);
            m_bodyUserData.SetRigidBody(this);
            m_bodyUserData.SetEntityId(configuration.m_entityId);
            m_debugName = configuration.m_debugName;

            if (configuration.m_customUserData)
            {
                SetUserData(configuration.m_customUserData);
            }
        }
    }

    void RigidBody::AddShape(AZStd::shared_ptr<Physics::Shape> shape)
    {
        if (!b3Body_IsValid(m_bodyId) || !shape)
        {
            return;
        }

        auto box3DShape = AZStd::rtti_pointer_cast<B3::Shape>(shape);

        if (!box3DShape)
        {
            AZ_Error("Box3D Rigid Body", false, "Trying to add a shape of unknown type. Name: %s", GetName().c_str());
            return;
        }

        if (box3DShape->GetShapeConfiguration()->GetShapeType() == Physics::ShapeType::TriangleMesh) // TODO: || !IsKinematic()
        {
            AZ_Error("Box3D", false, "Cannot use triangle mesh geometry on a dynamic object: %s", GetName().c_str());
            return;
        }

        box3DShape->AttachedToActor(&m_bodyId);
        m_shapes.push_back(box3DShape);
    }

    void RigidBody::RemoveShape(AZStd::shared_ptr<Physics::Shape> shape)
    {
        if (!b3Body_IsValid(m_bodyId))
        {
            AZ_Warning("B3::RigidBody", false, "Trying to remove shape from rigid body with no actor");
            return;
        }

        auto box3DShape = AZStd::rtti_pointer_cast<B3::Shape>(shape);
        if (!box3DShape)
        {
            AZ_Warning("B3::RigidBody", false, "Trying to remove shape of unknown type", GetName().c_str());
            return;
        }

        const auto found = AZStd::find(m_shapes.begin(), m_shapes.end(), shape);
        if (found == m_shapes.end())
        {
            AZ_Warning("B3::RigidBody", false, "Shape has not been attached to this rigid body: %s", GetName().c_str());
            return;
        }
        
        box3DShape->DetachedFromActor();
        m_shapes.erase(found);
    }

    void RigidBody::UpdateMassProperties(AzPhysics::MassComputeFlags flags, const AZ::Vector3& centerOfMassOffsetOverride, const AZ::Matrix3x3& inertiaTensorOverride, const float massOverride)
    {
        AZ_UNUSED_4(flags, centerOfMassOffsetOverride, inertiaTensorOverride, massOverride);
        // const bool computeCenterOfMass = AzPhysics::MassComputeFlags::COMPUTE_COM == (flags & AzPhysics::MassComputeFlags::COMPUTE_COM);
        // const bool computeInertiaTensor = AzPhysics::MassComputeFlags::COMPUTE_INERTIA == (flags & AzPhysics::MassComputeFlags::COMPUTE_INERTIA);
        // const bool computeMass = AzPhysics::MassComputeFlags::COMPUTE_MASS == (flags & AzPhysics::MassComputeFlags::COMPUTE_MASS);
        // const bool needsCompute = computeCenterOfMass || computeInertiaTensor || computeMass;
        // const bool includeAllShapesInMassCalculation = AzPhysics::MassComputeFlags::INCLUDE_ALL_SHAPES == (flags & AzPhysics::MassComputeFlags::INCLUDE_ALL_SHAPES);
        //
        // // Basic case where all properties are set directly.
        // if (!needsCompute)
        // {
        //     SetCenterOfMassOffset(centerOfMassOffsetOverride);
        //     SetMass(massOverride);
        //     SetInertia(inertiaTensorOverride);
        //     return;
        // }
        //
        // // If there are no shapes then set the properties directly without computing anything.
        // if (m_shapes.empty())
        // {
        //     SetCenterOfMassOffset(computeCenterOfMass ? DefaultCenterOfMass  : centerOfMassOffsetOverride);
        //     SetMass(computeMass ? DefaultMass : massOverride);
        //     SetInertia(computeInertiaTensor ? DefaultInertiaTensor : inertiaTensorOverride);
        //     return;
        // }
        //
        // auto cannotComputeMassProperties = [this, includeAllShapesInMassCalculation]
        // {
        //     return AZStd::any_of(m_shapes.cbegin(), m_shapes.cend(),
        //         [includeAllShapesInMassCalculation](const AZStd::shared_ptr<B3::Shape>& shape)
        //         {
        //             const physx::PxShape& pxShape = *shape->GetPxShape();
        //             const bool includeShape = includeAllShapesInMassCalculation || Is;
        //
        //             return includeShape && !CanShapeComputeMassProperties(pxShape);
        //         });
        // };
        //
        // // If contains shapes that cannot compute mass properties (triangle mesh,
        // // plane or heightfield) then default values will be used.
        // if (cannotComputeMassProperties())
        // {
        //     AZ_Warning("RigidBody", !computeCenterOfMass,
        //         "Rigid body '%s' cannot compute COM because it contains triangle mesh, plane or heightfield shapes, it will default to %s.",
        //         GetName().c_str(), AZStd::to_string(DefaultCenterOfMass).c_str());
        //     AZ_Warning("RigidBody", !computeMass,
        //         "Rigid body '%s' cannot compute Mass because it contains triangle mesh, plane or heightfield shapes, it will default to %0.1f.",
        //         GetName().c_str(), DefaultMass);
        //     AZ_Warning("RigidBody", !computeInertiaTensor,
        //         "Rigid body '%s' cannot compute Inertia because it contains triangle mesh, plane or heightfield shapes, it will default to %s.",
        //         GetName().c_str(), AZStd::to_string(DefaultInertiaTensor.RetrieveScale()).c_str());
        //
        //     SetCenterOfMassOffset(computeCenterOfMass ? DefaultCenterOfMass : centerOfMassOffsetOverride);
        //     SetMass(computeMass ? DefaultMass : massOverride);
        //     SetInertia(computeInertiaTensor ? DefaultInertiaTensor : inertiaTensorOverride);
        //     return;
        // }
        //
        // // Center of mass needs to be considered first since
        // // it's needed when computing mass and inertia.
        // if (computeCenterOfMass)
        // {
        //     // Compute Center of Mass
        //     UpdateCenterOfMass(includeAllShapesInMassCalculation);
        // }
        // else
        // {
        //     SetCenterOfMassOffset(centerOfMassOffsetOverride);
        // }
        // const physx::PxVec3 pxCenterOfMass = PxMathConvert(GetCenterOfMassLocal());
        //
        // if (computeMass)
        // {
        //     // Gather material densities from all shapes,
        //     // mass computation is based on them.
        //     AZStd::vector<float> densities;
        //     densities.reserve(m_shapes.size());
        //     for (const auto& shape : m_shapes)
        //     {
        //         const auto& physxMaterials = shape->GetBox3DMaterials();
        //         AZ_Assert(!physxMaterials.empty(), "Shape with no materials");
        //         densities.emplace_back(physxMaterials[0]->GetDensity());
        //     }
        //
        //     // Compute Mass + Inertia
        //     {
        //         physx::PxRigidBodyExt::updateMassAndInertia(*m_pxRigidActor,
        //             densities.data(), static_cast<AZ::u32>(densities.size()),
        //             &pxCenterOfMass, includeAllShapesInMassCalculation);
        //     }
        //
        //     // There is no physx function to only compute the mass without
        //     // computing the inertia. So now that both have been computed
        //     // we can override the inertia if it's suppose to use a
        //     // specific value set by the user.
        //     if (!computeInertiaTensor)
        //     {
        //         SetInertia(inertiaTensorOverride);
        //     }
        // }
        // else
        // {
        //     if (computeInertiaTensor)
        //     {
        //         // Set Mass + Compute Inertia
        //         physx::PxRigidBodyExt::setMassAndUpdateInertia(*m_pxRigidActor, massOverride, 
        //             &pxCenterOfMass, includeAllShapesInMassCalculation);
        //     }
        //     else
        //     {
        //         SetMass(massOverride);
        //         SetInertia(inertiaTensorOverride);
        //     }
        // }
    }

    AZ::u32 RigidBody::GetShapeCount() const 
    {
        return static_cast<AZ::u32>(m_shapes.size());
    }

    AZStd::shared_ptr<Physics::Shape> RigidBody::GetShape(AZ::u32 index)
    {
        AZStd::shared_ptr<const Physics::Shape> constShape = AZStd::as_const(*this).GetShape(index);
        return AZStd::const_pointer_cast<Physics::Shape>(constShape);
    }

    AZStd::shared_ptr<const Physics::Shape> RigidBody::GetShape(AZ::u32 index) const
    {
        if (index >= m_shapes.size())
        {
            return nullptr;
        }
        return m_shapes[index];
    }

    AZ::Vector3 RigidBody::GetCenterOfMassWorld() const
    {
        return b3Body_IsValid(m_bodyId) ? Box3DMathConvert(b3Body_GetWorldCenterOfMass(m_bodyId)) : AZ::Vector3::CreateZero();
    }

    AZ::Vector3 RigidBody::GetCenterOfMassLocal() const
    {
        if(b3Body_IsValid(m_bodyId))
        {
            return Box3DMathConvert(b3Body_GetLocalCenterOfMass(m_bodyId));
        }
        return AZ::Vector3::CreateZero();
    }

    AZ::Matrix3x3 RigidBody::GetInertiaWorld() const
    {
        if(b3Body_IsValid(m_bodyId))
        {
            // AZ::Vector3 inertiaDiagonal = PxMathConvert(m_pxRigidActor->getMassSpaceInertiaTensor());
            // AZ::Matrix3x3 rotationToWorld = AZ::Matrix3x3::CreateFromQuaternion(PxMathConvert(m_pxRigidActor->getGlobalPose().q.getConjugate()));
            // return Physics::Utils::DiagonalMatrixLocalToWorld(inertiaDiagonal, rotationToWorld);
            
            return Box3DMathConvert(b3InvertMatrix(b3Body_GetWorldInverseRotationalInertia(m_bodyId)));
            
        }

        return AZ::Matrix3x3::CreateZero();
    }

    AZ::Matrix3x3 RigidBody::GetInertiaLocal() const
    {
        if(b3Body_IsValid(m_bodyId))
        {
            // physx::PxVec3 inertiaDiagonal = m_pxRigidActor->getMassSpaceInertiaTensor();
            // return AZ::Matrix3x3::CreateDiagonal(PxMathConvert(inertiaDiagonal));
            
            return Box3DMathConvert(b3Body_GetLocalRotationalInertia(m_bodyId));
        }

        return AZ::Matrix3x3::CreateZero();
    }

    AZ::Matrix3x3 RigidBody::GetInverseInertiaWorld() const
    {
        if(b3Body_IsValid(m_bodyId))
        {
            // AZ::Vector3 inverseInertiaDiagonal = PxMathConvert(m_pxRigidActor->getMassSpaceInvInertiaTensor());
            // AZ::Matrix3x3 rotationToWorld = AZ::Matrix3x3::CreateFromQuaternion(PxMathConvert(m_pxRigidActor->getGlobalPose().q.getConjugate()));
            // return Physics::Utils::DiagonalMatrixLocalToWorld(inverseInertiaDiagonal, rotationToWorld);
            
            return Box3DMathConvert(b3Body_GetWorldInverseRotationalInertia(m_bodyId));
        }

        return AZ::Matrix3x3::CreateZero();
    }

    AZ::Matrix3x3 RigidBody::GetInverseInertiaLocal() const
    {
        if(b3Body_IsValid(m_bodyId))
        {
            // physx::PxVec3 inverseInertiaDiagonal = m_pxRigidActor->getMassSpaceInvInertiaTensor();
            // return AZ::Matrix3x3::CreateDiagonal(PxMathConvert(inverseInertiaDiagonal));
            
            return Box3DMathConvert(b3InvertMatrix(b3Body_GetLocalRotationalInertia(m_bodyId)));
        }
        return AZ::Matrix3x3::CreateZero();
    }

    float RigidBody::GetMass() const
    {
        if(b3Body_IsValid(m_bodyId))
        {
            return b3Body_GetMass(m_bodyId);
        }
        return 0.0f;
    }

    float RigidBody::GetInverseMass() const
    {
        if(b3Body_IsValid(m_bodyId))
        {
            return b3Body_GetInverseMass(m_bodyId);
        }
        return 0.0f;
    }

    void RigidBody::SetMass(float mass)
    {
        if(b3Body_IsValid(m_bodyId))
        {
            b3MassData massData = b3Body_GetMassData(m_bodyId);
            massData.mass = mass;
            b3Body_SetMassData(m_bodyId, massData);
        }
    }

    void RigidBody::SetCenterOfMassOffset(const AZ::Vector3& comOffset)
    {
        if(b3Body_IsValid(m_bodyId))
        {
            b3MassData massData = b3Body_GetMassData(m_bodyId);
            massData.center = Box3DMathConvert(comOffset);
            b3Body_SetMassData(m_bodyId, massData);
        }
    }

    void RigidBody::UpdateCenterOfMass(bool includeAllShapesInMassCalculation)
    {
        if (m_shapes.empty())
        {
            SetCenterOfMassOffset(DefaultCenterOfMass);
            return;
        }

        AZStd::vector<const b3ShapeId*> box3DShapes;
        box3DShapes.reserve(m_shapes.size());
        {
            // Filter shapes in the same way that updateMassAndInertia function does.
            for (const auto& shape : m_shapes)
            {
                const b3ShapeId& box3DShape = shape->GetShapeId();
                const bool includeShape = includeAllShapesInMassCalculation || IsSimulationShape(box3DShape);

                if (includeShape && CanShapeComputeMassProperties(box3DShape))
                {
                    box3DShapes.emplace_back(&box3DShape);
                }
            }
        }

        if (box3DShapes.empty())
        {
            SetCenterOfMassOffset(DefaultCenterOfMass);
            return;
        }

        const b3MassData box3DMassProperties = [this, &box3DShapes]
        {
            AZ_UNUSED(box3DShapes);
            // Note: Box3D computeMassPropertiesFromShapes function does not use densities
            //       to compute the shape's masses, which are needed to calculate the center of mass.
            //       This differs from updateMassAndInertia function, which uses material density values.
            //       So the masses used during center of mass calculation do not match the masses
            //       used during mass/inertia calculation. This is an inconsistency in PhysX.
            b3Body_ApplyMassFromShapes(m_bodyId);
            return b3Body_GetMassData(m_bodyId);
        }();

        SetCenterOfMassOffset(Box3DMathConvert(box3DMassProperties.center));
    }

    void RigidBody::SetInertia(const AZ::Matrix3x3& inertia)
    {
        if(b3Body_IsValid(m_bodyId))
        {
            b3MassData massData = b3Body_GetMassData(m_bodyId);
            massData.inertia = Box3DMathConvert(inertia);
            b3Body_SetMassData(m_bodyId, massData);
        }
    }

    AZ::Vector3 RigidBody::GetLinearVelocity() const
    {
        if(b3Body_IsValid(m_bodyId))
        {
            return Box3DMathConvert(b3Body_GetLinearVelocity(m_bodyId));
        }
        return AZ::Vector3::CreateZero();
    }

    void RigidBody::SetLinearVelocity(const AZ::Vector3& velocity)
    {
        if(b3Body_IsValid(m_bodyId))
        {
            b3Body_SetLinearVelocity(m_bodyId, Box3DMathConvert(velocity));
        }
    }

    AZ::Vector3 RigidBody::GetAngularVelocity() const
    {
        if(b3Body_IsValid(m_bodyId))
        {
            return Box3DMathConvert(b3Body_GetAngularVelocity(m_bodyId));
        }
        return AZ::Vector3::CreateZero();
    }

    void RigidBody::SetAngularVelocity(const AZ::Vector3& angularVelocity)
    {
        if(b3Body_IsValid(m_bodyId))
        {
            b3Body_SetAngularVelocity(m_bodyId, Box3DMathConvert(angularVelocity));
        }
    }

    AZ::Vector3 RigidBody::GetLinearVelocityAtWorldPoint(const AZ::Vector3& worldPoint) const
    {
        return b3Body_IsValid(m_bodyId) ?
               GetLinearVelocity() + GetAngularVelocity().Cross(worldPoint - GetCenterOfMassWorld()) :
               AZ::Vector3::CreateZero();
    }

    void RigidBody::ApplyLinearImpulse(const AZ::Vector3& impulse)
    {
        if(b3Body_IsValid(m_bodyId))
        {
            if (IsKinematic())
            {
                AZ_Warning("Box3D Rigid Body", false, "ApplyLinearImpulse is only valid if the rigid body is not kinematic. Name: %s", GetName().c_str());
                return;
            }
            b3Body_ApplyLinearImpulseToCenter(m_bodyId, Box3DMathConvert(impulse), true);
        }
    }

    void RigidBody::ApplyLinearImpulseAtWorldPoint(const AZ::Vector3& impulse, const AZ::Vector3& worldPoint)
    {
        if(b3Body_IsValid(m_bodyId))
        {
            if (IsKinematic())
            {
                AZ_Warning("Box3D Rigid Body", false, "ApplyLinearImpulseAtWorldPoint is only valid if the rigid body is not kinematic. Name: %s", GetName().c_str());
                return;
            }
            b3Body_ApplyLinearImpulse(m_bodyId, Box3DMathConvert(impulse), Box3DMathConvert(worldPoint), true);
        }
    }

    void RigidBody::ApplyAngularImpulse(const AZ::Vector3& angularImpulse)
    {
        if(b3Body_IsValid(m_bodyId))
        {
            if (IsKinematic())
            {
                AZ_Warning("Box3D Rigid Body", false, "ApplyAngularImpulse is only valid if the rigid body is not kinematic. Name: %s", GetName().c_str());
                return;
            }

            b3Body_ApplyAngularImpulse(m_bodyId, Box3DMathConvert(angularImpulse), true);
        }
    }

    void RigidBody::SetKinematic(bool isKinematic)
    {
        if(b3Body_IsValid(m_bodyId))
        {
            if (!isKinematic)
            {
                // check if any of the shapes on the rigid body would prevent switching to dynamic
                const bool allShapesCanComputeMassProperties = AZStd::all_of(
                    m_shapes.begin(),
                    m_shapes.end(),
                    [](const AZStd::shared_ptr<B3::Shape>& shape)
                    {
                        return CanShapeComputeMassProperties(shape->GetShapeId());
                    });
                if (!allShapesCanComputeMassProperties)
                {
                    AZ_Warning(
                        "Box3D Rigid Body",
                        false,
                        "Cannot set kinematic to false, because body has triangle mesh, plane or heightfield shapes attached. Name: %s",
                        GetName().c_str());
                    return;
                }
            }

            b3Body_SetType(m_bodyId, b3_kinematicBody);
        }
    }

    bool RigidBody::IsKinematic() const
    {
        bool result = false;

        if(b3Body_IsValid(m_bodyId))
        {
            result = b3Body_GetType(m_bodyId) == b3_kinematicBody;
        }

        return result;
    }

    void RigidBody::SetKinematicTarget(const AZ::Transform& targetTransform)
    {
        if (IsKinematic())
        {
            b3Body_SetTargetTransform(
                m_bodyId, 
                Box3DMathConvert(targetTransform),
                GetBox3DSystem()->GetBox3DConfiguration().m_fixedTimestep, // TODO: cache this
                true
                );
        }
        else
        {
            AZ_Error("Box3D Rigid Body", false, "SetKinematicTarget is only valid if rigid body is kinematic. Name: %s", GetName().c_str());
        }
    }

    bool RigidBody::IsGravityEnabled() const
    {
        if(b3Body_IsValid(m_bodyId))
        {
            return b3Body_GetGravityScale(m_bodyId) > 0.0f;
        }
        return false;
    }

    void RigidBody::SetGravityEnabled(bool enabled)
    {
        {
            b3Body_SetGravityScale(m_bodyId, 1.0f);
        }
        if (enabled)
        {
            ForceAwake();
        }
    }

    void RigidBody::SetSimulationEnabled(bool enabled)
    {
        if(b3Body_IsValid(m_bodyId))
        {
            enabled ? b3Body_Enable(m_bodyId) : b3Body_Disable(m_bodyId);
        }
    }

    void RigidBody::SetCCDEnabled([[maybe_unused]] bool enabled)
    {
        AZ_Warning("Box3D Rigid Body", false, "CCD is not supported per body in Box3D.")
    }

    AZ::Transform RigidBody::GetTransform() const
    {
        if(b3Body_IsValid(m_bodyId))
        {
            return Box3DMathConvert(b3Body_GetTransform(m_bodyId));
        }
        return AZ::Transform::CreateIdentity();
    }

    void RigidBody::SetTransform(const AZ::Transform& transform)
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

    AZ::Vector3 RigidBody::GetPosition() const
    {
        if(b3Body_IsValid(m_bodyId))
        {
            return Box3DMathConvert(b3Body_GetPosition(m_bodyId));
        }
        return AZ::Vector3::CreateZero();
    }

    AZ::Quaternion RigidBody::GetOrientation() const
    {
        if(b3Body_IsValid(m_bodyId))
        {
            return Box3DMathConvert(b3Body_GetRotation(m_bodyId));
        }
        return  AZ::Quaternion::CreateZero();
    }

    AZ::Aabb RigidBody::GetAabb() const
    {
        if(b3Body_IsValid(m_bodyId))
        {
            return Box3DMathConvert(b3Body_ComputeAABB(m_bodyId));
        }
        return  AZ::Aabb::CreateNull();
    }

    AZ::EntityId RigidBody::GetEntityId() const
    {
        return m_bodyUserData.GetEntityId();
    }

    AzPhysics::SceneQueryHit RigidBody::RayCast(const AzPhysics::RayCastRequest& request)
    {
        AZ_UNUSED(request);
        // return B3::SceneQueryHelpers::ClosestRayHitAgainstShapes(request, m_shapes, GetTransform());
        return AzPhysics::SceneQueryHit();
    }

    // Physics::ReferenceBase
    AZ::Crc32 RigidBody::GetNativeType() const
    {
        return B3::NativeTypeIdentifiers::RigidBody;
    }

    void* RigidBody::GetNativePointer() const
    {
        return m_bodyIdPtr.get();
    }

    // Not in API but needed to support PhysicsComponentBus
    float RigidBody::GetLinearDamping() const
    {
        if(b3Body_IsValid(m_bodyId))
        {
            return b3Body_GetLinearDamping(m_bodyId);
        }
        return 0.0f;
    }

    void RigidBody::SetLinearDamping(float damping)
    {
        if (damping < 0.0f)
        {
            AZ_Warning("Box3D Rigid Body", false, "Negative linear damping value (%6.4e). Name: %s", damping, GetName().c_str());
            return;
        }
        if(b3Body_IsValid(m_bodyId))
        {
            b3Body_SetLinearDamping(m_bodyId, damping);
        }
    }

    float RigidBody::GetAngularDamping() const
    {
        if(b3Body_IsValid(m_bodyId))
        {
            return b3Body_GetAngularDamping(m_bodyId);
        }
        return 0.0f;
    }

    void RigidBody::SetAngularDamping(float damping)
    {
        if (damping < 0.0f)
        {
            AZ_Warning("Box3D Rigid Body", false, "Negative angular damping value (%6.4e). Name: %s", damping, GetName().c_str());
            return;
        }

        if(b3Body_IsValid(m_bodyId))
        {
            b3Body_SetAngularDamping(m_bodyId, damping);
        }
    }

    bool RigidBody::IsAwake() const
    {
        if(b3Body_IsValid(m_bodyId))
        {
            return b3Body_IsAwake(m_bodyId);
        }
        return false;
    }

    void RigidBody::ForceAsleep()
    {
        if(b3Body_IsValid(m_bodyId) && b3Body_IsSleepEnabled(m_bodyId) && IsAwake())
        {
            b3Body_SetAwake(m_bodyId, false);
        }
    }

    void RigidBody::ForceAwake()
    {
        if(b3Body_IsValid(m_bodyId) && !IsAwake())
        {
            b3Body_SetAwake(m_bodyId, true);
        }
    }

    float RigidBody::GetSleepThreshold() const
    {
        if(b3Body_IsValid(m_bodyId))
        {
            return b3Body_GetSleepThreshold(m_bodyId);
        }
        return 0.0f;
    }

    void RigidBody::SetSleepThreshold(float threshold)
    {
        if (threshold < 0.0f)
        {
            AZ_Warning("Box3D Rigid Body", false, "Negative sleep threshold value (%6.4e). Name: %s", threshold, GetName().c_str());
            return;
        }

        if(b3Body_IsValid(m_bodyId))
        {
            b3Body_SetSleepThreshold(m_bodyId, threshold);
        }
    }

    void RigidBody::SetName(const AZStd::string& entityName)
    {
        m_debugName = entityName;

        if(b3Body_IsValid(m_bodyId))
        {
            b3Body_SetName(m_bodyId, m_debugName.c_str());
        }
    }

    const AZStd::string& RigidBody::GetName() const
    {
        return m_debugName;
    }
}
