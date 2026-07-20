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
        bool IsPrimitiveShape(const Physics::ShapeConfiguration& shapeConfig)
        {
            const Physics::ShapeType shapeType = shapeConfig.GetShapeType();
            // box and cylinder are build as convex hulls in Box3D
            return
                shapeType == Physics::ShapeType::Capsule ||
                shapeType == Physics::ShapeType::Sphere;
        }

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
                    AZStd::to_string(shapeConfiguration.m_scale).c_str())
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
                        
                        break;
                    }

                    float halfHeight = 0.5f * height - radius;
                    if (halfHeight <= 0.0f)
                    {
                        AZ_Warning("Box3D", halfHeight < 0.0f,
                                   "Height must exceed twice the radius in capsule configuration (height: %f, radius: %f)",
                                   capsuleConfig.m_height, capsuleConfig.m_radius)
                        
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
                        
                        break;
                    }
                    
                    b3BoxHull box = b3MakeScaledBoxHull(
                        Box3DMathConvert(boxConfig.m_dimensions * 0.5f),
                        Box3DMathConvert(colliderConfiguration.m_position, colliderConfiguration.m_rotation),
                        Box3DMathConvert(boxConfig.m_scale)
                    );
                    newShapeId = b3CreateHullShape(bodyId, &shapeDef, &box.base); // b3BoxHull values are stack/struct-allocated and must not be freed
                    if (b3Shape_IsValid(newShapeId))
                        success = true;
                    break;
                }
            case Physics::ShapeType::Cylinder:
                {
                    const B3::CylinderShapeConfiguration& cylinderConfig = static_cast<const
                        B3::CylinderShapeConfiguration&>(shapeConfiguration);
                    // float height = cylinderConfig.m_height * cylinderConfig.m_scale.GetZ();
                    // float radius = cylinderConfig.m_radius * AZ::GetMax(cylinderConfig.m_scale.GetX(),
                    //                                                    cylinderConfig.m_scale.GetY());
                    float height = cylinderConfig.m_height; // With b3CreateTransformedHullShape we don't need to pre-scale
                    float radius = cylinderConfig.m_radius;

                    if (height <= 0.0f || radius <= 0.0f)
                    {
                        AZ_Error("Box3D Utils", false,
                                 "Negative or zero values are invalid for cylinder dimensions (height: %f, radius: %f)",
                                 cylinderConfig.m_height, cylinderConfig.m_radius)
                        
                        break;
                    }
                    
                    // TODO: Unsure what yOffset is
                    b3HullData* cylinder = b3CreateCylinder(height, radius, 0.0f, cylinderConfig.m_subdivisionCount);
                    if (cylinder == NULL)
                    {
                        // degenerate input: coincident or coplanar points, or insufficient volume
                    }
                    b3Transform transform = Box3DMathConvert(colliderConfiguration.m_position, colliderConfiguration.m_rotation);
                    newShapeId = b3CreateTransformedHullShape(bodyId, &shapeDef, cylinder, transform, Box3DMathConvert(cylinderConfig.m_scale));
                    if (b3Shape_IsValid(newShapeId))
                        success = true;
                    b3DestroyHull(cylinder); // Heap allocated and must be freed manually
                    break;
                }
            case Physics::ShapeType::ConvexHull:
                {
                    const Physics::ConvexHullShapeConfiguration& convexHullConfig = static_cast<const Physics::ConvexHullShapeConfiguration&>(shapeConfiguration);
                    
                    if (convexHullConfig.m_vertexCount == 0)
                    {
                        AZ_Error("Box3D Utils", false, "Zero vertex points to create hull: %u", convexHullConfig.m_vertexCount)
                        
                        break;
                    }
                    
                    b3HullData* hull = b3CreateHull(
                        static_cast<const b3Vec3*>(convexHullConfig.m_vertexData),
                        static_cast<int>(convexHullConfig.m_vertexCount),
                        static_cast<int>(convexHullConfig.m_vertexCount)
                        );
                    if (hull == NULL)
                    {
                        // degenerate input: coincident or coplanar points, or insufficient volume
                    }
                    b3Transform transform = Box3DMathConvert(colliderConfiguration.m_position, colliderConfiguration.m_rotation);
                    newShapeId = b3CreateTransformedHullShape(bodyId, &shapeDef, hull, transform, Box3DMathConvert(convexHullConfig.m_scale));
                    if (b3Shape_IsValid(newShapeId))
                        success = true;
                    b3DestroyHull(hull);
                    break;
                    
                }
            case Physics::ShapeType::TriangleMesh:
                {
                    break;
                }
            case Physics::ShapeType::CookedMesh:
                {
                    // TODO: unclear whether this has any use in Box3D
                    // const Physics::CookedMeshShapeConfiguration& constCookedMeshShapeConfig =
                    //     static_cast<const Physics::CookedMeshShapeConfiguration&>(shapeConfiguration);
                    //
                    // // We are deliberately removing the const off of the ShapeConfiguration here because we're going to change the cached
                    // // native mesh pointer that gets stored in the configuration.
                    // Physics::CookedMeshShapeConfiguration& cookedMeshShapeConfig =
                    //     const_cast<Physics::CookedMeshShapeConfiguration&>(constCookedMeshShapeConfig);
                    //
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
            case Physics::ShapeType::PhysicsAsset:
                {
                    // TODO: I think this is like a Box3D serialized compound shape that can be streamed 
                    AZ_Assert(false,
                              "CreateBox3DShapeFromConfig: Cannot pass PhysicsAsset configuration since it is a collection of shapes. "
                              "Please iterate over m_colliderShapes in the asset and call this function for each of them.")
                    ;
                    break;
                }
            case Physics::ShapeType::Native:
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

        AzPhysics::Scene* GetDefaultScene()
        {
            AzPhysics::SceneHandle sceneHandle;
            Physics::DefaultWorldBus::BroadcastResult(sceneHandle, &Physics::DefaultWorldRequests::GetDefaultSceneHandle);

            if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
            {
                if (auto* scene = physicsSystem->GetScene(sceneHandle))
                {
                    return scene;
                }
            }

            return nullptr;
        }

        AZStd::optional<Physics::CookedMeshShapeConfiguration> CreateCookedMeshConfiguration(const AZStd::vector<AZ::Vector3>& points, const AZ::Vector3& scale)
        {
            Physics::CookedMeshShapeConfiguration shapeConfig;

            AZStd::vector<AZ::u8> cookedData;
            bool cookingResult = false;
            Physics::SystemRequestBus::BroadcastResult(cookingResult, &Physics::SystemRequests::CookConvexMeshToMemory,
                points.data(), aznumeric_cast<AZ::u32>(points.size()), cookedData);
            shapeConfig.SetCookedMeshData(cookedData.data(), cookedData.size(),
                Physics::CookedMeshShapeConfiguration::MeshType::Convex);
            shapeConfig.m_scale = scale;

            if (!cookingResult)
            {
                AZ_Error("Box3D", false, "Box3D cooking of mesh data failed")
                return {};
            }

            return shapeConfig;
        }

        float GetTransformScale(AZ::EntityId entityId)
        {
            float transformScale = 1.0f;
            AZ::TransformBus::EventResult(transformScale, entityId, &AZ::TransformBus::Events::GetWorldUniformScale);
            return transformScale;
        }

        AZ::Vector3 GetNonUniformScale(AZ::EntityId entityId)
        {
            AZ::Vector3 nonUniformScale = AZ::Vector3::CreateOne();
            AZ::NonUniformScaleRequestBus::EventResult(nonUniformScale, entityId, &AZ::NonUniformScaleRequests::GetScale);
            return nonUniformScale;
        }

        AZ::Vector3 GetOverallScale(AZ::EntityId entityId)
        {
            return GetTransformScale(entityId) * GetNonUniformScale(entityId);
        }

        void CreateShapesFromAsset(const Physics::PhysicsAssetShapeConfiguration& assetConfiguration,
            const Physics::ColliderConfiguration& originalColliderConfiguration, bool hasNonUniformScale,
            AZ::u8 subdivisionLevel, AZStd::vector<AZStd::shared_ptr<Physics::Shape>>& resultingShapes)
        {
            AZ_UNUSED_5(assetConfiguration, originalColliderConfiguration, hasNonUniformScale, subdivisionLevel, resultingShapes);
            // AzPhysics::ShapeColliderPairList resultingColliderShapeConfigs;
            // GetColliderShapeConfigsFromAsset(assetConfiguration, originalColliderConfiguration,
            //     hasNonUniformScale, subdivisionLevel, resultingColliderShapeConfigs);
            //
            // resultingShapes.reserve(resultingShapes.size() + resultingColliderShapeConfigs.size());
            //
            // for (const AzPhysics::ShapeColliderPair& shapeConfigPair : resultingColliderShapeConfigs)
            // {
            //     // Scale the collider offset
            //     shapeConfigPair.first->m_position *= shapeConfigPair.second->m_scale;
            //
            //     AZStd::shared_ptr<Physics::Shape> shape;
            //     Physics::SystemRequestBus::BroadcastResult(shape, &Physics::SystemRequests::CreateShape,
            //         *shapeConfigPair.first, *shapeConfigPair.second);
            //
            //     if (shape)
            //     {
            //         resultingShapes.emplace_back(shape);
            //     }
            // }
        }

        // AZ::Aabb GetColliderAabb(const AZ::Transform& worldTransform,
        //     bool hasNonUniformScale,
        //     AZ::u8 subdivisionLevel,
        //     const ::Physics::ShapeConfiguration& shapeConfiguration,
        //     const ::Physics::ColliderConfiguration& colliderConfiguration)
        // {
        //     const AZ::Aabb worldPosAabb = AZ::Aabb::CreateFromPoint(worldTransform.GetTranslation());
        //     physx::PxGeometryHolder geometryHolder;
        //     bool isAssetShape = shapeConfiguration.GetShapeType() == Physics::ShapeType::PhysicsAsset;
        //
        //     if (!isAssetShape)
        //     {
        //         if (!hasNonUniformScale)
        //         {
        //             if (CreatePxGeometryFromConfig(shapeConfiguration, geometryHolder))
        //             {
        //                 return GetPxGeometryAabb(geometryHolder, worldTransform, colliderConfiguration);
        //             }
        //         }
        //         else
        //         {
        //             auto convexPrimitive = Utils::CreateConvexPointsFromPrimitive(colliderConfiguration, shapeConfiguration, subdivisionLevel, shapeConfiguration.m_scale);
        //             if (convexPrimitive.has_value())
        //             {
        //                 if (CreatePxGeometryFromConfig(convexPrimitive.value(), geometryHolder))
        //                 {
        //                     Physics::ColliderConfiguration colliderConfigurationNoOffset = colliderConfiguration;
        //                     colliderConfigurationNoOffset.m_rotation = AZ::Quaternion::CreateIdentity();
        //                     colliderConfigurationNoOffset.m_position = AZ::Vector3::CreateZero();
        //                     return GetPxGeometryAabb(geometryHolder, worldTransform, colliderConfigurationNoOffset);
        //                 }
        //             }
        //         }
        //         return worldPosAabb;
        //     }
        //     else
        //     {
        //         const Physics::PhysicsAssetShapeConfiguration& physicsAssetConfig =
        //             static_cast<const Physics::PhysicsAssetShapeConfiguration&>(shapeConfiguration);
        //
        //         if (!physicsAssetConfig.m_asset.IsReady())
        //         {
        //             return worldPosAabb;
        //         }
        //
        //         AzPhysics::ShapeColliderPairList colliderShapes;
        //         GetColliderShapeConfigsFromAsset(physicsAssetConfig,
        //             colliderConfiguration,
        //             hasNonUniformScale,
        //             subdivisionLevel,
        //             colliderShapes);
        //
        //         if (colliderShapes.empty())
        //         {
        //             return worldPosAabb;
        //         }
        //
        //         AZ::Aabb aabb = AZ::Aabb::CreateNull();
        //         for (const auto& colliderShape : colliderShapes)
        //         {
        //             if (colliderShape.second &&
        //                 CreatePxGeometryFromConfig(*colliderShape.second, geometryHolder))
        //             {
        //                 aabb.AddAabb(
        //                     GetPxGeometryAabb(geometryHolder, worldTransform, *colliderShape.first)
        //                 );
        //             }
        //             else
        //             {
        //                 return worldPosAabb;
        //             }
        //         }
        //         return aabb;
        //     }
        // }
        
        AZStd::optional<Physics::ConvexHullShapeConfiguration> CreateConvexPointsFromPrimitive(
            const Physics::ColliderConfiguration& colliderConfig,
            const Physics::ShapeConfiguration& primitiveShapeConfig, AZ::u8 subdivisionLevel,
            [[maybe_unused]] const AZ::Vector3& scale)
        {
            AZ::u8 subdivisionLevelClamped = AZ::GetClamp(subdivisionLevel, MinCapsuleSubdivisionLevel, MaxCapsuleSubdivisionLevel);

            // auto applyColliderOffset = [&colliderConfig](const AZ::Vector3 point) {
            //     return colliderConfig.m_rotation.TransformVector(point) + colliderConfig.m_position;
            // };
            auto applyColliderOffset = [&colliderConfig](const AZ::Vector3 point) { // TODO: Verify this since we transform/scale at hull creation
                AZ_UNUSED(colliderConfig)
                return point;
            };
            
            Physics::ConvexHullShapeConfiguration convexConfig;
            
            AZStd::vector<AZ::Vector3> points;
            auto shapeType = primitiveShapeConfig.GetShapeType();
            switch (shapeType)
            {
            case Physics::ShapeType::Box:
            {
                auto boxConfig = static_cast<const Physics::BoxShapeConfiguration&>(primitiveShapeConfig);
                
                points.reserve(8);
                const float x = 0.5f * boxConfig.m_dimensions.GetX();
                const float y = 0.5f * boxConfig.m_dimensions.GetY();
                const float z = 0.5f * boxConfig.m_dimensions.GetZ();
                points.push_back(applyColliderOffset(AZ::Vector3(-x, -y, -z)));
                points.push_back(applyColliderOffset(AZ::Vector3(-x, -y, +z)));
                points.push_back(applyColliderOffset(AZ::Vector3(-x, +y, -z)));
                points.push_back(applyColliderOffset(AZ::Vector3(-x, +y, +z)));
                points.push_back(applyColliderOffset(AZ::Vector3(+x, -y, -z)));
                points.push_back(applyColliderOffset(AZ::Vector3(+x, -y, +z)));
                points.push_back(applyColliderOffset(AZ::Vector3(+x, +y, -z)));
                points.push_back(applyColliderOffset(AZ::Vector3(+x, +y, +z)));
                    
                convexConfig.m_vertexData = points.data();
                convexConfig.m_vertexCount = static_cast<AZ::u32>(points.size());
                return convexConfig;
            }
            case Physics::ShapeType::Capsule:
            {
                auto capsuleConfig = static_cast<const Physics::CapsuleShapeConfiguration&>(primitiveShapeConfig);
                const AZ::u8 numLayers = subdivisionLevelClamped;
                const AZ::u8 numPerLayer = 4 * subdivisionLevelClamped;
                points.reserve(2 * numLayers * numPerLayer + 2);
                points.push_back(applyColliderOffset(AZ::Vector3::CreateAxisZ(0.5f * capsuleConfig.m_height)));
                points.push_back(applyColliderOffset(AZ::Vector3::CreateAxisZ(-0.5f * capsuleConfig.m_height)));
                for (AZ::u8 layerIndex = 0; layerIndex < numLayers; layerIndex++)
                {
                    const float theta = (layerIndex + 1) * AZ::Constants::HalfPi / aznumeric_cast<float>(numLayers);
                    const float layerRadius = capsuleConfig.m_radius * AZ::Sin(theta);
                    const float layerHeight = 0.5f * capsuleConfig.m_height + capsuleConfig.m_radius * (AZ::Cos(theta) - 1.0f);
                    for (AZ::u8 radialIndex = 0; radialIndex < numPerLayer; radialIndex++)
                    {
                        const float phi = radialIndex * AZ::Constants::TwoPi / aznumeric_cast<float>(numPerLayer);
                        points.push_back(applyColliderOffset(AZ::Vector3(
                            layerRadius * AZ::Cos(phi), layerRadius * AZ::Sin(phi), layerHeight)));
                        points.push_back(applyColliderOffset(AZ::Vector3(
                            layerRadius * AZ::Cos(phi), layerRadius * AZ::Sin(phi), -layerHeight)));
                    }
                }
                    
                convexConfig.m_vertexData = points.data();
                convexConfig.m_vertexCount = static_cast<AZ::u32>(points.size());
                return convexConfig;
            }
            case Physics::ShapeType::Sphere:
            {
                auto sphereConfig = static_cast<const Physics::SphereShapeConfiguration&>(primitiveShapeConfig);
                const AZ::u8 numLayers = 2 * subdivisionLevelClamped;
                const AZ::u8 numPerLayer = 4 * subdivisionLevelClamped;
                points.reserve((numLayers - 1) * numPerLayer + 2);
                points.push_back(applyColliderOffset(AZ::Vector3::CreateAxisZ(sphereConfig.m_radius)));
                points.push_back(applyColliderOffset(AZ::Vector3::CreateAxisZ(-sphereConfig.m_radius)));

                for (AZ::u8 layerIndex = 1; layerIndex < numLayers; layerIndex++)
                {
                    const float theta = layerIndex * AZ::Constants::Pi / aznumeric_cast<float>(numLayers);
                    const float layerRadius = sphereConfig.m_radius * AZ::Sin(theta);
                    const float layerHeight = sphereConfig.m_radius * AZ::Cos(theta);
                    for (AZ::u8 radialIndex = 0; radialIndex < numPerLayer; radialIndex++)
                    {
                        const float phi = radialIndex * AZ::Constants::TwoPi / aznumeric_cast<float>(numPerLayer);
                        points.push_back(applyColliderOffset(AZ::Vector3(
                            layerRadius * AZ::Cos(phi), layerRadius * AZ::Sin(phi), layerHeight)));
                    }
                }
                    
                convexConfig.m_vertexData = points.data();
                convexConfig.m_vertexCount = static_cast<AZ::u32>(points.size());
                return convexConfig;
            }
            case Physics::ShapeType::ConvexHull:
                return static_cast<const Physics::ConvexHullShapeConfiguration&>(primitiveShapeConfig);
            // case Physics::ShapeType::CookedMesh:
            //     return static_cast<const Physics::CookedMeshShapeConfiguration&>(primitiveShapeConfig);
            default:
                AZ_Error("Box3D Utils", false, "CreateConvexPointsFromPrimitive was called with a non-primitive shape configuration.")
                return {};
            }
        }
        
        // Returns a point list of the frustum extents based on the supplied frustum parameters.
        AZStd::optional<AZStd::vector<AZ::Vector3>> CreatePointsAtFrustumExtents(float height, float bottomRadius, float topRadius, AZ::u8 subdivisions)
        {
            AZStd::vector<AZ::Vector3> points;

            if (height <= 0.0f)
            {
                AZ_Error("Box3D", false, "Frustum height %f must be greater than 0.", height);
                return {};
            }

            if (bottomRadius < 0.0f)
            {
                AZ_Error("Box3D", false, "Frustum bottom radius %f must be greater or equal to 0.", bottomRadius);
                return {};
            }
            else if (topRadius < 0.0f)
            {
                AZ_Error("Box3D", false, "Frustum top radius %f must be greater or equal to 0.", topRadius);
                return {};
            }
            else if (bottomRadius == 0.0f && topRadius == 0.0f)
            {
                AZ_Error("Box3D", false, "Either frustum bottom radius or top radius must be greater than to 0.");
                return {};
            }

            if (subdivisions < MinFrustumSubdivisions || subdivisions > MaxFrustumSubdivisions)
            {
                AZ_Error("Box3D", false, "Frustum subdivision count %u is not in [%u, %u] range", subdivisions, MinFrustumSubdivisions, MaxFrustumSubdivisions);
                return {};
            }

            points.reserve(subdivisions * 2);
            const float halfHeight = height * 0.5f;
            const double step = AZ::Constants::TwoPi / aznumeric_cast<double>(subdivisions);

            for (double rad = 0; rad < AZ::Constants::TwoPi; rad += step)
            {
                float x = aznumeric_cast<float>(std::cos(rad));
                float y = aznumeric_cast<float>(std::sin(rad));

                points.emplace_back(x * topRadius, y * topRadius, +halfHeight);
                points.emplace_back(x * bottomRadius, y * bottomRadius, -halfHeight);
            }

            return points;
        }

    }
}
