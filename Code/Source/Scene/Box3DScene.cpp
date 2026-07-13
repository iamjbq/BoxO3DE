#include <Scene/Box3DScene.h>

#include <BoxO3DE/MathConversions.h>
#include <System/Box3DSystem.h>
#include <BoxO3DE/Utils.h>
#include <Clients/Shape.h>
#include <Clients/RigidBody.h>

#include <AzCore/Console/IConsole.h>
#include <AzCore/Debug/ProfilerBus.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/containers/variant.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/Task/TaskGraph.h>
#include <AzFramework/Physics/Character.h>
#include <AzFramework/Physics/Collision/CollisionEvents.h>
#include <AzFramework/Physics/Configuration/RigidBodyConfiguration.h>
#include <AzFramework/Physics/Configuration/StaticRigidBodyConfiguration.h>
#include <AzFramework/Physics/Material/PhysicsMaterialManager.h>

namespace B3
{
    AZ_CVAR_EXTERNED(bool, box3d_batchTransformSync);

    AZ_CVAR(bool, box3d_parallelTransformSync, true, nullptr, AZ::ConsoleFunctorFlags::Null, "Multithreaded transform update for rigid bodies. "
        "Only relevant if batched transform update is enabled.");
    AZ_CVAR(size_t, box3d_parallelTransformSyncBatchSize, 250, nullptr, AZ::ConsoleFunctorFlags::Null,
        "How many rigid bodies should be processed per task");

    AZ_CLASS_ALLOCATOR_IMPL(Box3DScene, AZ::SystemAllocator);

    AZ_CVAR(bool, box3d_profileSimulationDatapoints, true, nullptr, AZ::ConsoleFunctorFlags::Null,
        "Expose Box3D simulation statistics to profiler. "
        "True: Simulation statistics will be collected for the profiler. "
        "False: Simulation statistics will not be collected.");

    // /*static*/ thread_local AZStd::vector<physx::PxRaycastHit> Box3DScene::s_rayCastBuffer;
    // /*static*/ thread_local AZStd::vector<physx::PxSweepHit> Box3DScene::s_sweepBuffer;
    // /*static*/ thread_local AZStd::vector<physx::PxOverlapHit> Box3DScene::s_overlapBuffer;

    namespace Internal
    {
        bool AddShape(AZStd::variant<AzPhysics::RigidBody*, AzPhysics::StaticRigidBody*> simulatedBody, const AzPhysics::ShapeVariantData& shapeData)
        {
            if (const auto* shapeColliderPair = AZStd::get_if<AzPhysics::ShapeColliderPair>(&shapeData))
            {
                bool shapeAdded = false;
                auto shapePtr = AZStd::make_shared<Shape>(*(shapeColliderPair->first), *(shapeColliderPair->second));
                AZStd::visit([shapePtr, &shapeAdded](auto&& body)
                    {
                        if (shapePtr)
                        {
                            body->AddShape(shapePtr); // TODO: shape creation will have to be done inside the body->AddShape since it REQUIRES a b3BodyId
                            shapeAdded = true;
                        }
                    }, simulatedBody);
                return shapeAdded;
            }
            else if (const auto* shapeColliderPairList = AZStd::get_if<AZStd::vector<AzPhysics::ShapeColliderPair>>(&shapeData))
            {
                bool shapeAdded = false;
                for (const auto& shapeColliderConfigs : *shapeColliderPairList)
                {
                    auto shapePtr = AZStd::make_shared<Shape>(*(shapeColliderConfigs.first), *(shapeColliderConfigs.second));
                    AZStd::visit([shapePtr, &shapeAdded](auto&& body)
                        {
                            if (shapePtr)
                            {
                                body->AddShape(shapePtr);
                                shapeAdded = true;
                            }
                        }, simulatedBody);
                }
                return shapeAdded;
            }
            else if (const auto* shape = AZStd::get_if<AZStd::shared_ptr<Physics::Shape>>(&shapeData))
            {
                auto shapePtr = *shape;
                AZStd::visit([shapePtr](auto&& body)
                    {
                        body->AddShape(shapePtr);
                    }, simulatedBody);
                return true;
            }
            else if (const auto* shapeList = AZStd::get_if<AZStd::vector<AZStd::shared_ptr<Physics::Shape>>>(&shapeData))
            {
                for (auto shapePtr : *shapeList)
                {
                    AZStd::visit([shapePtr](auto&& body)
                        {
                            body->AddShape(shapePtr);
                        }, simulatedBody);
                }
                return true;
            }
            return false;
        }
        
        template<class SimulatedBodyType, class ConfigurationType>
        AzPhysics::SimulatedBody* CreateSimulatedBody(const ConfigurationType* configuration, AZ::Crc32& crc, b3WorldId& worldId)
        {
            SimulatedBodyType* newBody = aznew SimulatedBodyType(*configuration, worldId);
            if (!AZStd::holds_alternative<AZStd::monostate>(configuration->m_colliderAndShapeData))
            {
                [[maybe_unused]] const bool shapeAdded = AddShape(newBody, configuration->m_colliderAndShapeData);
                AZ_Warning("Box3DScene", shapeAdded, "No Collider or Shape information found when creating Rigid body [%s]", configuration->m_debugName.c_str());
            }
            crc = AZ::Crc32(newBody, sizeof(*newBody));
            return newBody;
        }
        
        AzPhysics::SimulatedBody* CreateRigidBody(const AzPhysics::RigidBodyConfiguration* configuration, AZ::Crc32& crc, b3WorldId& worldId)
        {
            AZ_UNUSED_2(configuration, crc)
            RigidBody* newBody = aznew RigidBody(*configuration, worldId);
            if (!AZStd::holds_alternative<AZStd::monostate>(configuration->m_colliderAndShapeData))
            {
                [[maybe_unused]] const bool shapeAdded = AddShape(newBody, configuration->m_colliderAndShapeData);
                AZ_Warning("Box3DScene", shapeAdded, "No Collider or Shape information found when creating Rigid body [%s]", configuration->m_debugName.c_str());
            }
            const AzPhysics::MassComputeFlags& flags = configuration->GetMassComputeFlags();
            newBody->UpdateMassProperties(flags, configuration->m_centerOfMassOffset,
                configuration->m_inertiaTensor, configuration->m_mass);
            
            crc = AZ::Crc32(newBody, sizeof(*newBody));
            return newBody;
        }
        
        AzPhysics::SimulatedBody* CreateCharacterBody(Box3DScene* scene,
            const Physics::CharacterConfiguration* characterConfig)
        {
            AZ_UNUSED_2(scene, characterConfig)
            // CharacterController* controller = Utils::Characters::CreateCharacterController(scene, *characterConfig);
            // if (controller == nullptr)
            // {
            //     AZ_Error("Box3DScene", false, "Failed to create character controller.");
            //     return nullptr;
            // }
            // controller->EnablePhysics(*characterConfig);
            // controller->SetBasePosition(characterConfig->m_position);
            //
            // for (auto shape : characterConfig->m_colliders)
            // {
            //     controller->AttachShape(shape);
            // }
            //
            // return controller;
            return nullptr;
        }
    }
    
    Box3DScene::Box3DScene(const AzPhysics::SceneConfiguration& config, const AzPhysics::SceneHandle& sceneHandle)
        : Scene(config)
        , m_config(config)
        , m_sceneHandle(sceneHandle)
        , m_physicsSystemConfigChanged([this](const AzPhysics::SystemConfiguration* config)
            {
                m_raycastBufferSize = config->m_raycastBufferSize;
                m_shapecastBufferSize = config->m_shapecastBufferSize;
                m_overlapBufferSize = config->m_overlapBufferSize;
            })
    {
        // Set up the scene query buffer sizes
        if (auto* box3DSystem = GetBox3DSystem())
        {
            if (const AzPhysics::SystemConfiguration* sysConfig = box3DSystem->GetConfiguration())
            {
                m_raycastBufferSize = sysConfig->m_raycastBufferSize;
                m_shapecastBufferSize = sysConfig->m_shapecastBufferSize;
                m_overlapBufferSize = sysConfig->m_overlapBufferSize;
            }
            //register for future changes to the buffer sizes.
            box3DSystem->RegisterSystemConfigurationChangedEvent(m_physicsSystemConfigChanged);
        }
        m_gravity = m_config.m_gravity;
        
        if (auto* box3DSystem = GetBox3DSystem())
        {
            const B3::Box3DSystemConfiguration& systemConfig = box3DSystem->GetBox3DConfiguration();
            
            b3Capacity worldCapacity;
            worldCapacity.staticShapeCount = systemConfig.m_capacityConfiguration.m_maxStaticShapes;
            worldCapacity.staticBodyCount = systemConfig.m_capacityConfiguration.m_maxStaticBodies;
            worldCapacity.dynamicShapeCount = systemConfig.m_capacityConfiguration.m_maxDynamicShapes;
            worldCapacity.dynamicBodyCount = systemConfig.m_capacityConfiguration.m_maxDynamicBodies;
            worldCapacity.contactCount = systemConfig.m_capacityConfiguration.m_maxContacts;
            
            b3WorldDef worldDef = b3DefaultWorldDef();
            worldDef.gravity = Box3DMathConvert(m_config.m_gravity);
            worldDef.enableContinuous = m_config.m_enableCcd;
            worldDef.restitutionThreshold = m_config.m_bounceThresholdVelocity;
            worldDef.userData = this;
            worldDef.capacity = worldCapacity;
            
            m_worldId = b3CreateWorld(&worldDef);
            AZ_Assert(b3World_IsValid(m_worldId), "B3::Box3DScene world creation failed");
        }
        
    }

    Box3DScene::~Box3DScene()
    {
        m_physicsSystemConfigChanged.Disconnect();
        
        for (auto& simulatedBody : m_simulatedBodies)
        {
            if (simulatedBody.second != nullptr)
            {
                if (simulatedBody.second->m_simulating)
                {
                    // Disable simulation on body (not signaling OnSimulationBodySimulationDisabled event)
                    DisableSimulationOfBodyInternal(*simulatedBody.second);
                }
                m_simulatedBodyRemovedEvent.Signal(m_sceneHandle, simulatedBody.second->m_bodyHandle);
                delete simulatedBody.second;
            }
        }
        m_simulatedBodies.clear();

        ClearDeferedDeletions();

        b3DestroyWorld(m_worldId);
    }

    void Box3DScene::StartSimulation(float deltatime)
    {
        AZ_PROFILE_SCOPE(Physics, "Box3DScene::StartSimulation");

        if (!IsEnabled())
        {
            return;
        }

        {
            AZ_PROFILE_SCOPE(Physics, "OnSceneSimulationStartEvent::Signaled");
            m_sceneSimulationStartEvent.Signal(m_sceneHandle, deltatime);
        }

        m_currentDeltaTime = deltatime;

        b3World_Step(m_worldId, deltatime, m_subStepCount);
    }

    void Box3DScene::FinishSimulation()
    {
        AZ_PROFILE_SCOPE(Physics, "Box3DScene::FinishSimulation");

        if (!IsEnabled())
        {
            return;
        }

        // {
        //     AZ_PROFILE_SCOPE(Physics, "Box3DScene::CheckResults");
        //
        //     // Wait for the simulation to complete.
        //     // In the multithreaded environment we need to make sure we don't lock the scene for write here.
        //     // This is because contact modification callbacks can be issued from the job threads and cause deadlock
        //     // due to the callback code locking the scene.
        //     // https://devtalk.nvidia.com/default/topic/1024408/pxcontactmodifycallback-and-pxscene-locking/
        //     m_pxScene->checkResults(true);
        // }
        //
        // bool activeActorsEnabled = false;
        // {
        //     AZ_PROFILE_SCOPE(Physics, "Box3DScene::FetchResults");
        //     PHYSX_SCENE_WRITE_LOCK(m_pxScene);
        //
        //     activeActorsEnabled = m_pxScene->getFlags() & physx::PxSceneFlag::eENABLE_ACTIVE_ACTORS;
        //
        //     // Swap the buffers, invoke callbacks, build the list of active actors.
        //     m_pxScene->fetchResults(true);
        // }
        //
        // if (activeActorsEnabled)
        // {
        //     AZ_PROFILE_SCOPE(Physics, "Box3DScene::ActiveActors");
        //
        //     AzPhysics::SimulatedBodyHandleList activeBodyHandles;
        //
        //     {
        //         PHYSX_SCENE_READ_LOCK(m_pxScene);
        //         physx::PxU32 numActiveActors = 0;
        //         physx::PxActor** activeActors = m_pxScene->getActiveActors(numActiveActors);
        //         activeBodyHandles.reserve(numActiveActors);
        //         for (physx::PxU32 i = 0; i < numActiveActors; ++i)
        //         {
        //             if (ActorData* actorData = Utils::GetUserData(activeActors[i]))
        //             {
        //                 activeBodyHandles.emplace_back(actorData->GetBodyHandle());
        //             }
        //         }
        //     }
        //
        //     // Keep the event signal outside of the scene lock since there may be handlers that want to lock the scene for write
        //     m_sceneActiveSimulatedBodies.Signal(m_sceneHandle, activeBodyHandles, m_currentDeltaTime);
        //
        //     if (physx_batchTransformSync)
        //     {
        //         m_queuedActiveBodyIndices.IncreaseCapacity(activeBodyHandles.size());
        //
        //         for (const AzPhysics::SimulatedBodyHandle& bodyHandle : activeBodyHandles)
        //         {
        //             AzPhysics::SimulatedBodyIndex bodyIndex = AZStd::get<1>(bodyHandle);
        //             m_queuedActiveBodyIndices.Insert(bodyIndex);
        //         }
        //
        //         m_accumulatedDeltaTime += m_currentDeltaTime;
        //     }
        //     else
        //     {
        //         SyncActiveBodyTransform(activeBodyHandles);
        //     }
        // }
        
        {
            AZ_PROFILE_SCOPE(Physics, "Box3DScene::BodyEvents");
            
            AzPhysics::SimulatedBodyHandleList activeBodyHandles;
            
            {
                b3BodyEvents bodyEvents = b3World_GetBodyEvents(m_worldId);
                // int activeBodies = b3World_GetAwakeBodyCount(m_worldId); // This is likely less efficient
                
                activeBodyHandles.reserve(bodyEvents.moveCount);
                for (int i = 0; i < bodyEvents.moveCount; ++i)
                {
                    const b3BodyMoveEvent* moveEvent = bodyEvents.moveEvents + i;
                    
                    if (BodyData* bodyData = Utils::GetUserData(moveEvent->bodyId))
                    {
                        activeBodyHandles.emplace_back(bodyData->GetBodyHandle());
                    }
                }
            }
            
            // TODO: collect collision and sensor events here for processing
            // Utility function to call OnCollide like with Jolt possibly
            // for (int i = 0; i < contactEvents.beginCount; ++i)
            // {
            //     b3ContactBeginTouchEvent* beginEvent = contactEvents.beginEvents + i;
            //     ShapesStartTouching(beginEvent->shapeIdA, beginEvent->shapeIdB);
            // }
            
            // for (int i = 0; i < contactEvents.endCount; ++i)
            // {
            //     b3ContactEndTouchEvent* endEvent = contactEvents.endEvents + i;
            //
            //     // Use b3Shape_IsValid because a shape may have been destroyed
            //     if (b3Shape_IsValid(endEvent->shapeIdA) && b3Shape_IsValid(endEvent->shapeIdB))
            //     {
            //         ShapesStopTouching(endEvent->shapeIdA, endEvent->shapeIdB);
            //     }
            // }
            
            // This is meant for determining when to play sounds when b3WorldDef::hitEventThreshold and is manually enabled on shapes
            // for (int i = 0; i < contactEvents.hitCount; ++i)
            // {
            //     b3ContactHitEvent* hitEvent = contactEvents.hitEvents + i;
            //     if (hitEvent->approachSpeed > 10.0f)
            //     {
            //         // play sound
            //     }
            // }
            
            // b3SensorEvents sensorEvents = b3World_GetSensorEvents(myWorldId);
            // for (int i = 0; i < sensorEvents.beginCount; ++i)
            // {
            //     b3SensorBeginTouchEvent* beginTouch = sensorEvents.beginEvents + i;
            //     void* myUserData = b3Shape_GetUserData(beginTouch->visitorShapeId);
            //     // process begin event
            // }
            
            // Keep the event signal outside the scene lock since there may be handlers that want to lock the scene for write
            m_sceneActiveSimulatedBodies.Signal(m_sceneHandle, activeBodyHandles, m_currentDeltaTime);
            
            if (box3d_batchTransformSync)
            {
                m_queuedActiveBodyIndices.IncreaseCapacity(activeBodyHandles.size());
            
                for (const AzPhysics::SimulatedBodyHandle& bodyHandle : activeBodyHandles)
                {
                    AzPhysics::SimulatedBodyIndex bodyIndex = AZStd::get<1>(bodyHandle);
                    m_queuedActiveBodyIndices.Insert(bodyIndex);
                }
            
                m_accumulatedDeltaTime += m_currentDeltaTime;
            }
            else
            {
                SyncActiveBodyTransform(activeBodyHandles);
            }
        }

        FlushQueuedEvents();
        ClearDeferedDeletions();

        {
            AZ_PROFILE_SCOPE(Physics, "OnSceneSimulationFinishedEvent::Signaled");
            m_sceneSimulationFinishEvent.Signal(m_sceneHandle, m_currentDeltaTime);
        }

        UpdateAzProfilerDataPoints();
    }

    void Box3DScene::SetEnabled(bool enable)
    {
        m_isEnabled = enable;
    }

    bool Box3DScene::IsEnabled() const
    {
        return m_isEnabled;
    }

    const AzPhysics::SceneConfiguration& Box3DScene::GetConfiguration() const
    {
        return m_config;
    }

    void Box3DScene::UpdateConfiguration(const AzPhysics::SceneConfiguration& config)
    {
        if (m_config != config)
        {
            m_config = config;
            m_configChangeEvent.Signal(m_sceneHandle, m_config);

            // set gravity verifies this is a new value.
            SetGravity(m_config.m_gravity);
        }
    }

    AzPhysics::SimulatedBodyHandle Box3DScene::AddSimulatedBody(
        const AzPhysics::SimulatedBodyConfiguration* simulatedBodyConfig)
    {
        AzPhysics::SimulatedBody* newBody = nullptr;
        AZ::Crc32 newBodyCrc;
        if (azrtti_istypeof<AzPhysics::RigidBodyConfiguration>(simulatedBodyConfig))
        {
            newBody = Internal::CreateRigidBody(
                azdynamic_cast<const AzPhysics::RigidBodyConfiguration*>(simulatedBodyConfig), newBodyCrc, m_worldId);
        }
        else if (azrtti_istypeof<AzPhysics::StaticRigidBodyConfiguration>(simulatedBodyConfig))
        {
            // newBody = Internal::CreateSimulatedBody<StaticRigidBody, AzPhysics::StaticRigidBodyConfiguration>(
            //     azdynamic_cast<const AzPhysics::StaticRigidBodyConfiguration*>(simulatedBodyConfig), newBodyCrc, m_worldId);
        }
        else if (azrtti_istypeof<Physics::CharacterConfiguration>(simulatedBodyConfig))
        {
            newBody = Internal::CreateCharacterBody(this, azdynamic_cast<const Physics::CharacterConfiguration*>(simulatedBodyConfig));
        }
        else if (azrtti_istypeof<Physics::RagdollConfiguration>(simulatedBodyConfig))
        {
            // newBody = Internal::CreateRagdollBody(this, azdynamic_cast<const Physics::RagdollConfiguration*>(simulatedBodyConfig));
        }
        else
        {
            AZ_Warning("Box3DScene", false, "Unknown SimulatedBodyConfiguration.");
            return AzPhysics::InvalidSimulatedBodyHandle;
        }

        if (newBody != nullptr)
        {
            AzPhysics::SimulatedBodyIndex index;

            if (m_freeSceneSlots.empty())
            {
                m_simulatedBodies.emplace_back(newBodyCrc, newBody);
                index = static_cast<AzPhysics::SimulatedBodyIndex>(m_simulatedBodies.size() - 1);
            }
            else
            {
                //fill any free slots first before increasing the size of the simulatedBodies vector.
                index = m_freeSceneSlots.front();
                m_freeSceneSlots.pop();
                AZ_Assert(index < m_simulatedBodies.size(), "Box3DScene::AddSimulatedBody: Free simulated body index is out of bounds");
                AZ_Assert(m_simulatedBodies[index].second == nullptr, "Box3DScene::AddSimulatedBody: Free simulated body index is not free");

                m_simulatedBodies[index] = AZStd::make_pair(newBodyCrc, newBody);
            }

            const AzPhysics::SimulatedBodyHandle newBodyHandle(newBodyCrc, index);
            newBody->m_sceneOwner = m_sceneHandle;
            newBody->m_bodyHandle = newBodyHandle;
            m_simulatedBodyAddedEvent.Signal(m_sceneHandle, newBodyHandle);

            // Enable simulation by default (not signaling OnSimulationBodySimulationEnabled event)
            if (simulatedBodyConfig->m_startSimulationEnabled)
            {
                EnableSimulationOfBodyInternal(*newBody);
            }

            return newBodyHandle;
        }

        return AzPhysics::InvalidSimulatedBodyHandle;
    }

    AzPhysics::SimulatedBodyHandleList Box3DScene::AddSimulatedBodies(
        const AzPhysics::SimulatedBodyConfigurationList& simulatedBodyConfigs)
    {
        AzPhysics::SimulatedBodyHandleList newBodyHandles;
        newBodyHandles.reserve(simulatedBodyConfigs.size());
        for (auto* config : simulatedBodyConfigs)
        {
            newBodyHandles.emplace_back(AddSimulatedBody(config));
        }
        return newBodyHandles;
    }

    AzPhysics::SimulatedBody* Box3DScene::GetSimulatedBodyFromHandle(AzPhysics::SimulatedBodyHandle bodyHandle)
    {
        if (bodyHandle == AzPhysics::InvalidSimulatedBodyHandle)
        {
            return nullptr;
        }

        AzPhysics::SimulatedBodyIndex index = AZStd::get<AzPhysics::HandleTypeIndex::Index>(bodyHandle);
        if (index < m_simulatedBodies.size()
            && m_simulatedBodies[index].first == AZStd::get<AzPhysics::HandleTypeIndex::Crc>(bodyHandle))
        {
            return m_simulatedBodies[index].second;
        }
        return nullptr;
    }

    AzPhysics::SimulatedBodyList Box3DScene::GetSimulatedBodiesFromHandle(
        const AzPhysics::SimulatedBodyHandleList& bodyHandles)
    {
        AzPhysics::SimulatedBodyList results;
        for (auto& handle : bodyHandles)
        {
            results.emplace_back(GetSimulatedBodyFromHandle(handle));
        }
        return results;
    }

    void Box3DScene::RemoveSimulatedBody(AzPhysics::SimulatedBodyHandle& bodyHandle)
    {
        if (bodyHandle == AzPhysics::InvalidSimulatedBodyHandle)
        {
            return;
        }

        AzPhysics::SimulatedBodyIndex index = AZStd::get<AzPhysics::HandleTypeIndex::Index>(bodyHandle);
        if (index < m_simulatedBodies.size()
            && m_simulatedBodies[index].first == AZStd::get<AzPhysics::HandleTypeIndex::Crc>(bodyHandle))
        {
            if (m_simulatedBodies[index].second->m_simulating)
            {
                // Disable simulation on body (not signaling OnSimulationBodySimulationDisabled event)
                DisableSimulationOfBodyInternal(*m_simulatedBodies[index].second);
            }

            m_simulatedBodyRemovedEvent.Signal(m_sceneHandle, bodyHandle);

            m_deferredDeletions.push_back(m_simulatedBodies[index].second);
            m_simulatedBodies[index] = AZStd::make_pair(AZ::Crc32(), nullptr);
            m_freeSceneSlots.push(index);

            bodyHandle = AzPhysics::InvalidSimulatedBodyHandle;
        }
    }

    void Box3DScene::RemoveSimulatedBodies(AzPhysics::SimulatedBodyHandleList& bodyHandles)
    {
        for (auto& handle: bodyHandles)
        {
            RemoveSimulatedBody(handle);
        }
    }

    void Box3DScene::EnableSimulationOfBody(AzPhysics::SimulatedBodyHandle bodyHandle)
    {
        if (bodyHandle == AzPhysics::InvalidSimulatedBodyHandle)
        {
            return;
        }

        if (AzPhysics::SimulatedBody* body = GetSimulatedBodyFromHandle(bodyHandle))
        {
            if (body->m_simulating)
            {
                return;
            }

            m_simulatedBodySimulationEnabledEvent.Signal(m_sceneHandle, bodyHandle);

            EnableSimulationOfBodyInternal(*body);
        }
        else
        {
            AZ_Warning("PhysXScene", false, "Unable to enable Simulated body, failed to find body.")
        }
    }

    void Box3DScene::DisableSimulationOfBody(AzPhysics::SimulatedBodyHandle bodyHandle)
    {
        if (bodyHandle == AzPhysics::InvalidSimulatedBodyHandle)
        {
            return;
        }

        if (AzPhysics::SimulatedBody* body = GetSimulatedBodyFromHandle(bodyHandle))
        {
            if (!body->m_simulating)
            {
                return;
            }

            m_simulatedBodySimulationDisabledEvent.Signal(m_sceneHandle, bodyHandle);

            DisableSimulationOfBodyInternal(*body);
        }
        else
        {
            AZ_Warning("PhysXScene", false, "Unable to disable Simulated body, failed to find body.")
        }
    }

    AzPhysics::JointHandle Box3DScene::AddJoint(const AzPhysics::JointConfiguration* jointConfig,
        AzPhysics::SimulatedBodyHandle parentBody, AzPhysics::SimulatedBodyHandle childBody)
    {
        AZ_UNUSED_3(jointConfig, parentBody, childBody)
        
        return AzPhysics::InvalidJointHandle;
    }

    AzPhysics::Joint* Box3DScene::GetJointFromHandle(AzPhysics::JointHandle jointHandle)
    {
        if (jointHandle == AzPhysics::InvalidJointHandle)
        {
            return nullptr;
        }

        AzPhysics::JointIndex index = AZStd::get<AzPhysics::HandleTypeIndex::Index>(jointHandle);
        if (index < m_joints.size()
            && m_joints[index].first == AZStd::get<AzPhysics::HandleTypeIndex::Crc>(jointHandle))
        {
            return m_joints[index].second;
        }
        return nullptr;
    }

    void Box3DScene::RemoveJoint(AzPhysics::JointHandle jointHandle)
    {
        if (jointHandle == AzPhysics::InvalidJointHandle)
        {
            return;
        }

        AzPhysics::JointIndex index = AZStd::get<AzPhysics::HandleTypeIndex::Index>(jointHandle);
        if (index < m_joints.size()
            && m_joints[index].first == AZStd::get<AzPhysics::HandleTypeIndex::Crc>(jointHandle))
        {
            m_deferredDeletionsJoints.push_back(m_joints[index].second);
            m_joints[index] = AZStd::make_pair(AZ::Crc32(), nullptr);
            m_freeJointSlots.push(index);
            jointHandle = AzPhysics::InvalidJointHandle;
        }
    }

    AzPhysics::SceneQueryHits Box3DScene::QueryScene(const AzPhysics::SceneQueryRequest* request)
    {
        AzPhysics::SceneQueryHits hits;
        QueryScene(request, hits);
        return hits;
    }

    bool Box3DScene::QueryScene(const AzPhysics::SceneQueryRequest* request, AzPhysics::SceneQueryHits& result)
    {
        AZ_UNUSED(result)
        if (request == nullptr)
        {
            return false; // return 0 hits
        }

        // Query flags.
        // const physx::PxQueryFlags queryFlags = SceneQueryHelpers::GetPxQueryFlags(request->m_queryType);
        // const physx::PxQueryFilterData queryData(queryFlags);
        //
        // switch (request->m_requestType)
        // {
        // case AzPhysics::SceneQueryRequest::RequestType::Raycast:
        //     {
        //         return Internal::RayCast(static_cast<const AzPhysics::RayCastRequest*>(request),
        //             s_rayCastBuffer, m_pxScene, queryData, m_raycastBufferSize, result);
        //     }
        // case AzPhysics::SceneQueryRequest::RequestType::Shapecast:
        //     {
        //         return Internal::ShapeCast(static_cast<const AzPhysics::ShapeCastRequest*>(request),
        //             s_sweepBuffer, m_pxScene, queryData, m_shapecastBufferSize, result);
        //     }
        // case AzPhysics::SceneQueryRequest::RequestType::Overlap:
        //     {
        //         return Internal::OverlapQuery(static_cast<const AzPhysics::OverlapRequest*>(request),
        //             s_overlapBuffer, m_pxScene, queryData, m_overlapBufferSize, result);
        //     }
        // default:
        //     {
        //         AZ_Warning("Physx", false, "Unknown Scene Query request type.");
        //     }
        // };

        return false;
    }

    AzPhysics::SceneQueryHitsList Box3DScene::QuerySceneBatch(const AzPhysics::SceneQueryRequests& requests)
    {
        AzPhysics::SceneQueryHitsList results;
        results.reserve(requests.size());
        for (auto& request : requests)
        {
            results.emplace_back(QueryScene(request.get()));
        }
        return results;
    }

    bool Box3DScene::QuerySceneAsync(AzPhysics::SceneQuery::AsyncRequestId requestId,
        const AzPhysics::SceneQueryRequest* request, AzPhysics::SceneQuery::AsyncCallback callback)
    {
        AZ_UNUSED_3(requestId, request, callback)
        AZ_Warning("Box3D", false, "Currently unimplemented.");
        return false;
    }

    bool Box3DScene::QuerySceneAsyncBatch(AzPhysics::SceneQuery::AsyncRequestId requestId,
        const AzPhysics::SceneQueryRequests& requests, AzPhysics::SceneQuery::AsyncBatchCallback callback)
    {
        AZ_UNUSED_3(requestId, requests, callback)
        AZ_Warning("Box3D", false, "Currently unimplemented.");
        return false;
    }

    void Box3DScene::SuppressCollisionEvents(const AzPhysics::SimulatedBodyHandle& bodyHandleA,
        const AzPhysics::SimulatedBodyHandle& bodyHandleB)
    {
        AZ_UNUSED_2(bodyHandleA, bodyHandleB)
        // AzPhysics::SimulatedBody* bodyA = GetSimulatedBodyFromHandle(bodyHandleA);
        // AzPhysics::SimulatedBody* bodyB = GetSimulatedBodyFromHandle(bodyHandleB);
        // if (bodyA != nullptr && bodyB != nullptr)
        // {
        //     m_collisionFilterCallback.RegisterSuppressedCollision(bodyA, bodyB);
        // }
    }

    void Box3DScene::UnsuppressCollisionEvents(const AzPhysics::SimulatedBodyHandle& bodyHandleA,
        const AzPhysics::SimulatedBodyHandle& bodyHandleB)
    {
        AZ_UNUSED_2(bodyHandleA, bodyHandleB)
        // AzPhysics::SimulatedBody* bodyA = GetSimulatedBodyFromHandle(bodyHandleA);
        // AzPhysics::SimulatedBody* bodyB = GetSimulatedBodyFromHandle(bodyHandleB);
        // if (bodyA != nullptr && bodyB != nullptr)
        // {
        //     m_collisionFilterCallback.UnregisterSuppressedCollision(bodyA, bodyB);
        // }
    }

    void Box3DScene::SetGravity(const AZ::Vector3& gravity)
    {
        if (b3World_IsValid(m_worldId) && !m_gravity.IsClose(gravity))
        {
            m_gravity = gravity;
            {
                b3World_SetGravity(m_worldId, Box3DMathConvert(m_gravity));
            }
            m_sceneGravityChangedEvent.Signal(m_sceneHandle, m_gravity);
        }
    }

    AZ::Vector3 Box3DScene::GetGravity() const
    {
        return m_gravity;
    }

    void* Box3DScene::GetNativePointer() const
    {
        AZ_Warning("Box3DScene", false, "Box3DScene::GetNativePointer() is not implemented");
        
        return nullptr;
    }

    void Box3DScene::FlushTransformSync()
    {
        AZ_PROFILE_SCOPE(Physics, "Box3D::FlushTransformSync");

        auto transformSync = [this](AzPhysics::SimulatedBodyIndex bodyIndex)
        {
            if (bodyIndex < m_simulatedBodies.size() && m_simulatedBodies[bodyIndex].second)
            {
                m_simulatedBodies[bodyIndex].second->SyncTransform(m_accumulatedDeltaTime);
            }
        };

        if (box3d_parallelTransformSync)
        {
            m_queuedActiveBodyIndices.ApplyParallel(transformSync, m_worldId);
        }
        else
        {
            m_queuedActiveBodyIndices.Apply(transformSync);
        }

        m_queuedActiveBodyIndices.Clear();
        m_accumulatedDeltaTime = 0.0f;
    }

    void Box3DScene::EnableSimulationOfBodyInternal(AzPhysics::SimulatedBody& body)
    {
        AZ_UNUSED(body);
        //character controller is a special actor and only needs the m_simulating flag set,
        // if (!azrtti_istypeof<PhysX::CharacterController>(body) &&
        //     !azrtti_istypeof<PhysX::Ragdoll>(body) &&
        //     !azrtti_istypeof<PhysX::ArticulationLink>(body))
        // {
        //     auto pxActor = static_cast<physx::PxActor*>(body.GetNativePointer());
        //     AZ_Assert(pxActor, "Simulated Body doesn't have a valid physx actor");
        //
        //     {
        //         PHYSX_SCENE_WRITE_LOCK(m_pxScene);
        //         m_pxScene->addActor(*pxActor);
        //     }
        //
        //     if (azrtti_istypeof<PhysX::RigidBody>(body))
        //     {
        //         auto rigidBody = azdynamic_cast<PhysX::RigidBody*>(&body);
        //         if (rigidBody->ShouldStartAsleep())
        //         {
        //             rigidBody->ForceAsleep();
        //         }
        //     }
        // }
        //
        // body.m_simulating = true;
    }

    void Box3DScene::DisableSimulationOfBodyInternal(AzPhysics::SimulatedBody& body)
    {
        AZ_UNUSED(body);
        //character controller is a special actor and only needs the m_simulating flag set,
        // if (!azrtti_istypeof<PhysX::CharacterController>(body) &&
        //     !azrtti_istypeof<PhysX::Ragdoll>(body) &&
        //     !azrtti_istypeof<PhysX::ArticulationLink>(body))
        // {
        //     auto pxActor = static_cast<physx::PxActor*>(body.GetNativePointer());
        //     AZ_Assert(pxActor, "Simulated Body doesn't have a valid physx actor");
        //
        //     {
        //         PHYSX_SCENE_WRITE_LOCK(m_pxScene);
        //         m_pxScene->removeActor(*pxActor);
        //     }
        // }
        // body.m_simulating = false;
    }

    void Box3DScene::FlushQueuedEvents()
    {
        //send queued trigger events
        ProcessTriggerEvents();

        //send queued collision events
        ProcessCollisionEvents();
    }

    void Box3DScene::ClearDeferedDeletions()
    {
        // swap the deletions in case the simulated body
        // manages more bodies and removes them on destruction (ie. Ragdoll).
        AZStd::vector<AzPhysics::SimulatedBody*> deletions;
        deletions.swap(m_deferredDeletions);
        for (auto* simulatedBody : deletions)
        {
            delete simulatedBody;
        }

        AZStd::vector<AzPhysics::Joint*> jointDeletions;
        jointDeletions.swap(m_deferredDeletionsJoints);
        for (auto* joint : jointDeletions)
        {
            delete joint;
        }
    }

    void Box3DScene::ProcessTriggerEvents()
    {
        AZ_PROFILE_SCOPE(Physics, "Box3DScene::ProcessTriggerEvents");
        
        if (m_queuedTriggerEvents.empty())
        {
            return; // nothing to signal
        }
        
        m_sceneTriggerEvent.Signal(m_sceneHandle, m_queuedTriggerEvents);

        for (auto& triggerEvent : m_queuedTriggerEvents)
        {
            if (triggerEvent.m_triggerBody != nullptr)
            {
                triggerEvent.m_triggerBody->ProcessTriggerEvent(triggerEvent);
            }
            if (triggerEvent.m_otherBody != nullptr)
            {
                triggerEvent.m_otherBody->ProcessTriggerEvent(triggerEvent);
            }
        }
        
        //cleanup events for next simulate
        m_queuedTriggerEvents.clear();
    }

    void Box3DScene::ProcessCollisionEvents()
    {
        AZ_PROFILE_SCOPE(Physics, "Box3DScene::ProcessCollisionEvents");
        
        if (m_queuedCollisionEvents.empty())
        {
            return; //nothing to signal
        }
        //send all event to any scene listeners
        m_sceneCollisionEvent.Signal(m_sceneHandle, m_queuedCollisionEvents);

        //send events to each body listener
        for (auto& collision : m_queuedCollisionEvents)
        {
            //trigger on body 1
            if (collision.m_body1 != nullptr)
            {
                collision.m_body1->ProcessCollisionEvent(collision);
            }
            //trigger for body 2
            if (collision.m_body2 != nullptr)
            {
                //swap the data as the event expects the trigger body to be body1.
                //this is ok to do as this event is no longer used after calling TriggerCollisionEvent
                AZStd::swap(collision.m_bodyHandle1, collision.m_bodyHandle2);
                AZStd::swap(collision.m_body1, collision.m_body2);
                AZStd::swap(collision.m_shape1, collision.m_shape2);
                collision.m_body1->ProcessCollisionEvent(collision);
            }
        }

        //cleanup events for next simulate
        
        m_queuedCollisionEvents.clear();
    }

    void Box3DScene::UpdateAzProfilerDataPoints()
    {
    }

    void Box3DScene::SyncActiveBodyTransform(const AzPhysics::SimulatedBodyHandleList& activeBodyHandles)
    {
        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            for (const AzPhysics::SimulatedBodyHandle& bodyHandle : activeBodyHandles)
            {
                if (AzPhysics::SimulatedBody* simBody = sceneInterface->GetSimulatedBodyFromHandle(m_sceneHandle, bodyHandle))
                {
                    simBody->SyncTransform(m_currentDeltaTime);
                }
            }
        }
    }
    
    void Box3DScene::QueuedActiveBodyIndices::Insert(AzPhysics::SimulatedBodyIndex bodyIndex)
    {
        if (m_uniqueIndices.insert(bodyIndex).second)
        {
            m_packedIndices.emplace_back(bodyIndex);
        }
    }

    void Box3DScene::QueuedActiveBodyIndices::IncreaseCapacity(size_t extraSize)
    {
        m_packedIndices.reserve(m_packedIndices.size() + extraSize);
    }

    void Box3DScene::QueuedActiveBodyIndices::Clear()
    {
        m_uniqueIndices.clear();
        m_packedIndices.clear();
    }

    void Box3DScene::QueuedActiveBodyIndices::Apply(const AZStd::function<void(AzPhysics::SimulatedBodyIndex)>& applyFunction)
    {
        AZStd::for_each(m_packedIndices.begin(), m_packedIndices.end(), applyFunction);
    }

    void Box3DScene::QueuedActiveBodyIndices::ApplyParallel(const AZStd::function<void(AzPhysics::SimulatedBodyIndex)>& applyFunction, b3WorldId worldId)
    {
        AZ::TaskGraph taskGraph("Parallel Sync");
        AZ::TaskGraphEvent finishEvent("Parallel sync event");

        {
            AZ_PROFILE_SCOPE(Physics, "Sync Setup");
            
            size_t batchSize = box3d_parallelTransformSyncBatchSize;
            size_t fullSize = m_packedIndices.size();
            for (size_t i = 0; i < fullSize; i += batchSize)
            {
                AZ::TaskDescriptor taskDescriptor{"SyncTask", "Physics"};
                taskGraph.AddTask(
                    taskDescriptor,
                    [start = i, end = AZStd::min(i + batchSize, fullSize), &applyFunction, worldId, this]()
                    {
                        AZ_PROFILE_SCOPE(Physics, "Sync Task");
                        
                        // Note: It is important to keep the scene locked for read for the entire task execution.
                        // Otherwise, the functions reading data from the rigid body will have to lock it locally.
                        // This causes a huge amount of context switches making the execution of each task ~20x slower. 
                        // PHYSX_SCENE_READ_LOCK(pxScene);
                        AZ_UNUSED(worldId)
                        
                        for (size_t batchIndex = start; batchIndex < end; ++batchIndex)
                        {
                            applyFunction(m_packedIndices[batchIndex]);
                        }
                    });
            }

            taskGraph.Submit(&finishEvent);
        }

        finishEvent.Wait();
    }

}
