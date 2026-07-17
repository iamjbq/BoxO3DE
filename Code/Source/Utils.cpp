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

#include <box3d/box3d.h>

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
                    const Physics::SphereShapeConfiguration& sphereConfig = static_cast<const
                        Physics::SphereShapeConfiguration&>(shapeConfiguration);
                    if (sphereConfig.m_radius <= 0.0f)
                    {
                        AZ_Error("Box3D Utils", false, "Invalid radius value: %f", sphereConfig.m_radius)
                        ;
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
                    const Physics::CapsuleShapeConfiguration& capsuleConfig = static_cast<const
                        Physics::CapsuleShapeConfiguration&>(shapeConfiguration);
                    float height = capsuleConfig.m_height * capsuleConfig.m_scale.GetZ();
                    float radius = capsuleConfig.m_radius * AZ::GetMax(capsuleConfig.m_scale.GetX(),
                                                                       capsuleConfig.m_scale.GetY());

                    if (height <= 0.0f || radius <= 0.0f)
                    {
                        AZ_Error("Box3D Utils", false,
                                 "Negative or zero values are invalid for capsule dimensions (height: %f, radius: %f)",
                                 capsuleConfig.m_height, capsuleConfig.m_radius)
                        ;
                        break;
                    }

                    float halfHeight = 0.5f * height - radius;
                    if (halfHeight <= 0.0f)
                    {
                        AZ_Warning("Box3D", halfHeight < 0.0f,
                                   "Height must exceed twice the radius in capsule configuration (height: %f, radius: %f)",
                                   capsuleConfig.m_height, capsuleConfig.m_radius)
                        ;
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
                    const Physics::BoxShapeConfiguration& boxConfig = static_cast<const Physics::BoxShapeConfiguration&>
                        (shapeConfiguration);
                    if (!boxConfig.m_dimensions.IsGreaterThan(AZ::Vector3::CreateZero()))
                    {
                        AZ_Error("Box3D Utils", false, "Negative or zero values are invalid for box dimensions %s",
                                 AZStd::to_string(boxConfig.m_dimensions).c_str())
                        ;
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
            case Physics::ShapeType::Cylinder:
                {
                    break;
                }
            case Physics::ShapeType::PhysicsAsset:
                {
                    AZ_Assert(false,
                              "CreateBox3DShapeFromConfig: Cannot pass PhysicsAsset configuration since it is a collection of shapes. "
                              "Please iterate over m_colliderShapes in the asset and call this function for each of them.")
                    ;
                    break;
                }
            case Physics::ShapeType::TriangleMesh:
                {
                    break;
                }
            case Physics::ShapeType::CookedMesh:
                {
                    // const Physics::CookedMeshShapeConfiguration& constCookedMeshShapeConfig =
                    //     static_cast<const Physics::CookedMeshShapeConfiguration&>(shapeConfiguration);
                    //
                    // // We are deliberately removing the const off of the ShapeConfiguration here because we're going to change the cached
                    // // native mesh pointer that gets stored in the configuration.
                    // Physics::CookedMeshShapeConfiguration& cookedMeshShapeConfig =
                    //     const_cast<Physics::CookedMeshShapeConfiguration&>(constCookedMeshShapeConfig);

                    // physx::PxBase* nativeMeshObject = nullptr;
                    //
                    // // Use the cached mesh object if it is there, otherwise create one and save in the shape configuration
                    // if (cookedMeshShapeConfig.GetCachedNativeMesh())
                    // {
                    //     nativeMeshObject = static_cast<physx::PxBase*>(cookedMeshShapeConfig.GetCachedNativeMesh());
                    // }
                    // else
                    // {
                    //     nativeMeshObject = CreateNativeMeshObjectFromCookedData(
                    //         cookedMeshShapeConfig.GetCookedMeshData(),
                    //         cookedMeshShapeConfig.GetMeshType());
                    //
                    //     if (nativeMeshObject)
                    //     {
                    //         cookedMeshShapeConfig.SetCachedNativeMesh(nativeMeshObject);
                    //     }
                    //     else
                    //     {
                    //         AZ_Warning("PhysX Rigid Body", false,
                    //             "Unable to create a mesh object from the CookedMeshShapeConfiguration buffer. "
                    //             "Please check if the data was cooked correctly.");
                    //         return false;
                    //     }
                    // }
                    //
                    // return MeshDataToPxGeometry(nativeMeshObject, pxGeometry, cookedMeshShapeConfig.m_scale);
                    break;
                }
            case Physics::ShapeType::Heightfield:
                {
                    const Physics::HeightfieldShapeConfiguration& constHeightfieldConfig =
                        static_cast<const Physics::HeightfieldShapeConfiguration&>(shapeConfiguration);

                    // We are deliberately removing the const off of the ShapeConfiguration here because we're going to change the cached
                    // native heightfield pointer that gets stored in the configuration.
                    Physics::HeightfieldShapeConfiguration& heightfieldConfig =
                        const_cast<Physics::HeightfieldShapeConfiguration&>(constHeightfieldConfig);

                    const AZStd::vector<Physics::HeightMaterialPoint>& heightSamples = heightfieldConfig.GetSamples();
                    AZStd::vector<float> heightValues(heightSamples.size());
                    for (size_t heightIndex = 0; heightIndex < heightSamples.size(); ++heightIndex)
                    {
                        heightValues[heightIndex] = heightSamples[heightIndex].m_height;
                    }

                    // TODO: may need to do more data processing here like scaling for int16 or massaging the shape of the data
                    b3HeightFieldDef def = {0};
                    def.heights = heightValues.data(); // float[countX * countZ]
                    def.countX = static_cast<int>(heightfieldConfig.GetNumColumnVertices());
                    def.countZ = static_cast<int>(heightfieldConfig.GetNumRowVertices());
                    def.scale = Box3DMathConvert(heightfieldConfig.m_scale);
                    def.globalMinimumHeight = heightfieldConfig.GetMinHeightBounds();
                    def.globalMaximumHeight = heightfieldConfig.GetMaxHeightBounds();

                    b3HeightFieldData* hf = b3CreateHeightField(&def);
                    newShapeId = b3CreateHeightFieldShape(bodyId, &shapeDef, hf);
                    break;
                }
            case Physics::ShapeType::Native:
                {
                    break;
                }
            case Physics::ShapeType::ConvexHull:
                {
                    break;
                }
            default:
                AZ_Warning("Box3D Rigid Body", false, "Shape not supported in Box3D. Shape Type: %d", shapeType)
                ;
                break;
            }
                    
            if (success)
            {
                // Do something
            }
            return newShapeId;
        }
    }
}
