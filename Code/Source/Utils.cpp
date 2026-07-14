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
        b3ShapeId CreateBox3DShapeFromConfig(
            const Physics::ColliderConfiguration& colliderConfiguration,
            const Physics::ShapeConfiguration& shapeConfiguration,
            const b3ShapeDef& shapeDef, 
            const b3BodyId& bodyId
            )
        {
            b3ShapeId newShapeId = b3_nullShapeId;
            
            if (!shapeConfiguration.m_scale.IsGreaterThan(AZ::Vector3::CreateZero()))
            {
                AZ_Error("Box3D Utils", false, "Negative or zero values are invalid for shape configuration scale values %s",
                    AZStd::to_string(shapeConfiguration.m_scale).c_str());
                return newShapeId;
            }

            auto shapeType = shapeConfiguration.GetShapeType();
            bool success = false;
            
            switch (shapeType)
            {
                case Physics::ShapeType::Sphere:
                {
                    const Physics::SphereShapeConfiguration& sphereConfig = static_cast<const Physics::SphereShapeConfiguration&>(shapeConfiguration);
                    if (sphereConfig.m_radius <= 0.0f)
                    {
                        AZ_Error("Box3D Utils", false, "Invalid radius value: %f", sphereConfig.m_radius);
                        break;
                    }
                        
                    b3Sphere sphere;
                    sphere.center = Box3DMathConvert(colliderConfiguration.m_position);
                    sphere.radius = sphereConfig.m_radius * shapeConfiguration.m_scale.GetMaxElement();
                    newShapeId = b3CreateSphereShape(bodyId, &shapeDef, &sphere);
                    if (b3Shape_IsValid(newShapeId))
                        success = true;
                    break;
                }
                case Physics::ShapeType::Capsule:
                {
                    const Physics::CapsuleShapeConfiguration& capsuleConfig = static_cast<const Physics::CapsuleShapeConfiguration&>(shapeConfiguration);
                    float height = capsuleConfig.m_height * capsuleConfig.m_scale.GetZ();
                    float radius = capsuleConfig.m_radius * AZ::GetMax(capsuleConfig.m_scale.GetX(), capsuleConfig.m_scale.GetY());

                    if (height <= 0.0f || radius <= 0.0f)
                    {
                        AZ_Error("Box3D Utils", false, "Negative or zero values are invalid for capsule dimensions (height: %f, radius: %f)",
                            capsuleConfig.m_height, capsuleConfig.m_radius);
                        break;
                    }

                    float halfHeight = 0.5f * height - radius;
                    if (halfHeight <= 0.0f)
                    {
                        AZ_Warning("Box3D", halfHeight < 0.0f, "Height must exceed twice the radius in capsule configuration (height: %f, radius: %f)",
                            capsuleConfig.m_height, capsuleConfig.m_radius);
                        halfHeight = std::numeric_limits<float>::epsilon();
                    }
                    
                    AZ::Vector3 axis = colliderConfiguration.m_rotation.TransformVector(AZ::Vector3::CreateAxisZ());
                    b3Capsule capsule;
                    capsule.center1 = Box3DMathConvert(colliderConfiguration.m_position - axis * halfHeight);
                    capsule.center2 = Box3DMathConvert(colliderConfiguration.m_position + axis * halfHeight);
                    capsule.radius = radius;
                    newShapeId = b3CreateCapsuleShape(bodyId, &shapeDef, &capsule);
                    if (b3Shape_IsValid(newShapeId))
                        success = true;
                    break;
                }
                case Physics::ShapeType::Box:
                    {
                        const Physics::BoxShapeConfiguration& boxConfig = static_cast<const Physics::BoxShapeConfiguration&>(shapeConfiguration);
                        if (!boxConfig.m_dimensions.IsGreaterThan(AZ::Vector3::CreateZero()))
                        {
                            AZ_Error("Box3D Utils", false, "Negative or zero values are invalid for box dimensions %s",
                                AZStd::to_string(boxConfig.m_dimensions).c_str());
                            break;
                        }
                        
                        b3BoxHull box = b3MakeScaledBoxHull(
                            Box3DMathConvert(boxConfig.m_dimensions * 0.5f),
                            Box3DMathConvert(colliderConfiguration.m_position, colliderConfiguration.m_rotation),
                            Box3DMathConvert(boxConfig.m_scale)
                            );
                        newShapeId = b3CreateHullShape(bodyId, &shapeDef, &box.base);
                        if (b3Shape_IsValid(newShapeId))
                            success = true;
                        break;
                    }
                default:
                    AZ_Warning("Box3D Rigid Body", false, "Shape not supported in Box3D. Shape Type: %d", shapeType);
                    break;
            }
                    
            if (success)
            {
                return newShapeId;
            }
        }
    }
}
