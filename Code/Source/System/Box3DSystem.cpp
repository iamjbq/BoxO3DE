#include <System/Box3DSystem.h>
#include <Scene/Box3DScene.h>

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Component/ComponentApplicationLifecycle.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Debug/PerformanceCollector.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/PlatformId/PlatformId.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

// only enable Box3D timestep warning when not running debug or in Release
#if !defined(DEBUG) && !defined(RELEASE)
#define ENABLE_BOX3D_TIMESTEP_WARNING
#endif

namespace B3
{
    // cvars //
    AZ_CVAR(bool, box3d_batchTransformSync, false, nullptr, AZ::ConsoleFunctorFlags::Null,
        "Batch entity transform syncs for the entire simulation pass. "
        "True: Sync entity transform once per Simulate call. "
        "False: Sync entity transform for every simulation sub-step.");

    AZ_CLASS_ALLOCATOR_IMPL(Box3DSystem, AZ::SystemAllocator);

#ifdef ENABLE_BOX3D_TIMESTEP_WARNING
    namespace FrameTimeWarning
    {
        static constexpr int MaxSamples = 1000;
        static int NumSamples = 0;
        static int NumSamplesOverLimit = 0;
        static float LostTime = 0.0f;
    }

    AZ_CVAR(bool, box3d_reportTimestepWarnings, false, nullptr, AZ::ConsoleFunctorFlags::Null, "A flag providing ability to turn on/off reporting of Box3D timestep warnings");
#endif

    // A helper function.
    AZ::Debug::PerformanceCollector::DataLogType GetDataLogTypeFromCVar(const AZ::CVarFixedString& newCaptureType)
    {
        if (newCaptureType.starts_with('a') || newCaptureType.starts_with('A'))
        {
            return AZ::Debug::PerformanceCollector::DataLogType::LogAllSamples;
        }
        else
        {
            return AZ::Debug::PerformanceCollector::DataLogType::LogStatistics;
        }
    }

    AZ_CVAR(AZ::u32, box3d_metricsFrameCountPerCaptureBatch, 60,
        [](const AZ::u32& newValue)
        {
            B3::GetBox3DSystem()->GetPerformanceCollector()->UpdateFrameCountPerCaptureBatch(newValue);
        },
        AZ::ConsoleFunctorFlags::DontReplicate, "Number of frames in which performance will be measured per batch.");

    AZ_CVAR(AZ::u32, box3d_metricsNumberOfCaptureBatches, 0,
        [](const AZ::u32& newValue)
        {
            B3::GetBox3DSystem()->GetPerformanceCollector()->UpdateNumberOfCaptureBatches(newValue);
        },
        AZ::ConsoleFunctorFlags::DontReplicate,
            "Collects and reports Box3D performance in this number of batches. "
            "Starts at 0, which means do not capture performance data. "
            "When this variable changes to > 0 we'll start performance capture.");

    AZ_CVAR(AZ::CVarFixedString, box3d_metricsDataLogType, "statistical",
        [](const AZ::CVarFixedString& newValue)
        {
            B3::GetBox3DSystem()->GetPerformanceCollector()->UpdateDataLogType(GetDataLogTypeFromCVar(newValue));
        },
        AZ::ConsoleFunctorFlags::DontReplicate, "Defines the kind of data collection and logging. "
            "If starts with 's' it will log statistical summaries (average, min, max, stdev), "
            "if starts with 'a' or 'A' will log all samples of data (high verbosity). Default=s");

    AZ_CVAR(AZ::u32, box3d_metricsWaitTimePerCaptureBatch, 0,
        [](const AZ::u32& newValue)
        {
            B3::GetBox3DSystem()->GetPerformanceCollector()->UpdateWaitTimeBeforeEachBatch(AZStd::chrono::seconds(newValue));
        },
        AZ::ConsoleFunctorFlags::DontReplicate, "How many seconds to wait before each batch of performance capture.");
    // cvars //
    
    // Callback for asserts, connect this to your own assert handler if you have one
    static int Box3DAssertFunction(const char* condition, const char* fileName, int lineNumber)
    {
        AZ_Assert(false, "Box3D - %s:%i: %s", fileName, lineNumber, (condition != nullptr? condition : ""))

        // Breakpoint
        return 0;
    };
    
    // Callback for traces, connect this to your own trace function if you have one
    static void Box3DLogFunction(const char* message)
    {
        AZ_Trace("Jolt", message)
    }
    
    //! System allocator to be used for all Box3D gem persistent allocations.
    AZ_CHILD_ALLOCATOR_WITH_NAME(Box3DAllocator, "Box3DAllocator", "{32D5E884-E194-4DF5-B4D5-E3852BBF578D}", AZ::SystemAllocator);
    
    static void* Box3DAllocateFunction(int32_t size, int32_t alignment)
    {
        void* ptr = AZ::AllocatorInstance<Box3DAllocator>::Get().Allocate(size, alignment, 0, "Box3D");
        // AZ_Assert((reinterpret_cast<size_t>(ptr) & 15) == 0, "Box3D requires 16-byte aligned memory allocations.");
        return ptr;
    }
    
    static void Box3DFreeFunction(void* mem)
    {
        AZ::AllocatorInstance<Box3DAllocator>::Get().DeAllocate(mem);
    }
    
    Box3DSystem::Box3DSystem(AZStd::unique_ptr<Box3DSettingsRegistryManager> registryManager)
        : m_registryManager(AZStd::move(registryManager))
        , m_sceneInterface(this)
    {
        InitializePerformanceCollector();
    }

    Box3DSystem::~Box3DSystem()
    {
        Shutdown();
    }

    void Box3DSystem::Initialize(const AzPhysics::SystemConfiguration* config)
    {
        if (m_state == State::Initialized)
        {
            AZ_Warning("Box3DSystem", false, "Box3D system already initialized, Shutdown must be called first OR call Reinitialize or UpdateConfiguration(forceReinit=true) to reboot");
            return;
        }

        if (const auto* box3DConfig = azdynamic_cast<const Box3DSystemConfiguration*>(config))
        {
            m_systemConfig = *box3DConfig;
        }

        // Initialize the simulation scratch buffer. It must be in increments of 16K and 16-byte aligned.
        // m_scratchBufferSize = 16 * 1024 * m_systemConfig.m_limitsConfiguration.m_numScratchBufferBlocks;
        // m_scratchBufferAddress = AZ::AllocatorInstance<PhysXAllocator>::Get().allocate(m_scratchBufferSize, 16);
        // AZ_Assert((reinterpret_cast<size_t>(m_scratchBufferAddress) & 15) == 0, "PhysX requires 16-byte aligned memory allocations.");

        b3SetAssertFcn(&Box3DAssertFunction);
        b3SetLogFcn(&Box3DLogFunction);
        b3SetAllocator(&Box3DAllocateFunction, &Box3DFreeFunction);
        
        m_state = State::Initialized;
        m_initializeEvent.Signal(&m_systemConfig);
    }

    void Box3DSystem::Reinitialize()
    {
        //To be implemented with LYN-1146
        AZ_Warning("Box3DSystem", false, "Box3D Reinitialize currently not supported.");
    }

    void Box3DSystem::Shutdown()
    {
        if (m_state != State::Initialized)
        {
            return;
        }

        RemoveAllScenes();

        // Deallocate the scratch buffer
        // AZ::AllocatorInstance<PhysXAllocator>::Get().deallocate(m_scratchBufferAddress, m_scratchBufferSize, 16);

        m_accumulatedTime = 0.0f;
        m_state = State::Shutdown;
    }

    void Box3DSystem::Simulate(float deltaTime)
    {
        AZ_PROFILE_FUNCTION(Physics);

        if (m_state != State::Initialized)
        {
            AZ_Warning("Box3DSystem", false, "Call Simulate when Box3D system is not initialized");
            return;
        }

        auto simulateScenes = [this](float timeStep)
        {
            for (auto& scenePtr : m_sceneList)
            {
                if (scenePtr != nullptr && scenePtr->IsEnabled())
                {
                    AZ::Debug::ScopeDuration performanceScopeDuration(m_performanceCollector.get(), PerformanceSpecBox3DSimulationTime);
                    scenePtr->StartSimulation(timeStep);
                    scenePtr->FinishSimulation();
                }
            }
        };

#ifdef ENABLE_BOX3D_TIMESTEP_WARNING
        if (FrameTimeWarning::NumSamples < FrameTimeWarning::MaxSamples)
        {
            FrameTimeWarning::NumSamples++;
            if (deltaTime > m_systemConfig.m_maxTimestep)
            {
                FrameTimeWarning::NumSamplesOverLimit++;
                FrameTimeWarning::LostTime += deltaTime - m_systemConfig.m_maxTimestep;
            }
        }
        else
        {
            AZ_Warning("Box3DSystem", !box3d_reportTimestepWarnings || FrameTimeWarning::NumSamplesOverLimit <= 0,
                "[%d] of [%d] frames had a deltatime over the Max physics timestep[%.6f]. Box3D timestep was clamped on those frames, losing [%.6f] seconds.",
                FrameTimeWarning::NumSamplesOverLimit, FrameTimeWarning::NumSamples, m_systemConfig.m_maxTimestep, FrameTimeWarning::LostTime);
            FrameTimeWarning::NumSamples = 0;
            FrameTimeWarning::NumSamplesOverLimit = 0;
            FrameTimeWarning::LostTime = 0.0f;
        }
#endif
        deltaTime = AZ::GetClamp(deltaTime, 0.0f, m_systemConfig.m_maxTimestep);

        AZ_Assert(m_systemConfig.m_fixedTimestep >= 0.0f, "Box3DSystem - fixed timestep is negitive.");
        float tickTime = deltaTime;
        if (m_systemConfig.m_fixedTimestep > 0.0f) //use the fixed timestep
        {
            m_accumulatedTime += tickTime;
            //divide accumulated time by the fixed step and floor it to get the number of steps that would occur. Then multiply by fixedTimeStep to get the total executed time.
            tickTime = AZStd::floorf(m_accumulatedTime / m_systemConfig.m_fixedTimestep) * m_systemConfig.m_fixedTimestep;
            m_preSimulateEvent.Signal(tickTime);

            while (m_accumulatedTime >= m_systemConfig.m_fixedTimestep)
            {
                simulateScenes(m_systemConfig.m_fixedTimestep);
                m_accumulatedTime -= m_systemConfig.m_fixedTimestep;
            }
        }
        else
        {
            m_preSimulateEvent.Signal(tickTime);

            simulateScenes(tickTime);
        }
        
        // Flush performance data for this tick
        m_performanceCollector->FrameTick();
        
        if (box3d_batchTransformSync)
        {
            for (auto& scenePtr : m_sceneList)
            {
                if (scenePtr != nullptr && scenePtr->IsEnabled())
                {
                    Box3DScene* box3DScene = static_cast<Box3DScene*>(scenePtr.get());
                    box3DScene->FlushTransformSync();
                }
            }
        }

        m_postSimulateEvent.Signal(tickTime);
    }

    AzPhysics::SceneHandle Box3DSystem::AddScene(const AzPhysics::SceneConfiguration& config)
    {
        if (config.m_sceneName.empty())
        {
            AZ_Error("Box3DSystem", false, "AddScene: Trying to Add a scene without a name. SceneConfiguration::m_sceneName must have a value");
            return AzPhysics::InvalidSceneHandle;
        }

        if (!m_freeSceneSlots.empty()) //fill any free slots first before increasing the size of the scene list vector.
        {
            AzPhysics::SceneIndex freeIndex = m_freeSceneSlots.front();
            m_freeSceneSlots.pop();
            AZ_Assert(freeIndex < m_sceneList.size(), "Box3DSystem::AddScene: Free scene index is out of bounds!");
            AZ_Assert(m_sceneList[freeIndex] == nullptr, "Box3DSystem::AddScene: Free scene index is not free");

            const AzPhysics::SceneHandle sceneHandle(AZ::Crc32(config.m_sceneName), freeIndex);
            m_sceneList[freeIndex] = AZStd::make_unique<Box3DScene>(config, sceneHandle);
            m_sceneAddedEvent.Signal(sceneHandle);
            return sceneHandle;
        }

        if (m_sceneList.size() < AzPhysics::MaxNumberOfScenes) //add a new scene if it is under the limit
        {
            const AzPhysics::SceneHandle sceneHandle(AZ::Crc32(config.m_sceneName), static_cast<AzPhysics::SceneIndex>(m_sceneList.size()));
            m_sceneList.emplace_back(AZStd::make_unique<Box3DScene>(config, sceneHandle));
            m_sceneAddedEvent.Signal(sceneHandle);
            return sceneHandle;
        }
        AZ_Warning("Box3D", false, "Scene Limit reached[%u], unable to add new scene [%s]",
            AzPhysics::MaxNumberOfScenes,
            config.m_sceneName.c_str());
        return AzPhysics::InvalidSceneHandle;
    }

    AzPhysics::SceneHandleList Box3DSystem::AddScenes(const AzPhysics::SceneConfigurationList& configs)
    {
        AzPhysics::SceneHandleList sceneHandles;
        sceneHandles.reserve(configs.size());
        for (const auto& config : configs)
        {
            AzPhysics::SceneHandle sceneHandle = AddScene(config);
            sceneHandles.emplace_back(sceneHandle);
        }
        return sceneHandles;
    }

    AzPhysics::SceneHandle Box3DSystem::GetSceneHandle(const AZStd::string& sceneName)
    {
        const AZ::Crc32 sceneCrc(sceneName);
        auto sceneItr = AZStd::find_if(m_sceneList.begin(), m_sceneList.end(), [sceneCrc](auto& scene) {
            return scene != nullptr && sceneCrc == scene->GetId();
            });

        if (sceneItr != m_sceneList.end())
        {
            return AzPhysics::SceneHandle((*sceneItr)->GetId(), static_cast<AzPhysics::SceneIndex>(AZStd::distance(m_sceneList.begin(), sceneItr)));
        }
        return AzPhysics::InvalidSceneHandle;
    }

    AzPhysics::Scene* Box3DSystem::GetScene(AzPhysics::SceneHandle handle)
    {
        if (handle == AzPhysics::InvalidSceneHandle)
        {
            return nullptr;
        }

        AzPhysics::SceneIndex index = AZStd::get<AzPhysics::HandleTypeIndex::Index>(handle);
        if (index < m_sceneList.size())
        {
            if (auto& scenePtr = m_sceneList[index];
                scenePtr != nullptr)
            {
                if (scenePtr->GetId() == AZStd::get<AzPhysics::HandleTypeIndex::Crc>(handle))
                {
                    return scenePtr.get();
                }
            }
        }
        return nullptr;
    }

    AzPhysics::SceneList Box3DSystem::GetScenes(const AzPhysics::SceneHandleList& handles)
    {
        AzPhysics::SceneList requestedSceneList;
        requestedSceneList.reserve(handles.size());
        for (const auto& handle : handles)
        {
            AzPhysics::Scene* scene = GetScene(handle);
            requestedSceneList.emplace_back(scene);
        }
        return requestedSceneList;
    }

    AzPhysics::SceneList& Box3DSystem::GetAllScenes()
    {
        return m_sceneList;
    }

    void Box3DSystem::RemoveScene(AzPhysics::SceneHandle handle)
    {
        if (handle == AzPhysics::InvalidSceneHandle)
        {
            return;
        }

        AZ::u64 index = AZStd::get<AzPhysics::HandleTypeIndex::Index>(handle);
        if (index < m_sceneList.size() )
        {
            if (auto& scenePtr = m_sceneList[index];
                scenePtr != nullptr)
            {
                if (scenePtr->GetId() == AZStd::get<AzPhysics::HandleTypeIndex::Crc>(handle))
                {
                    m_sceneRemovedEvent.Signal(handle);
                    m_sceneList[index].reset();
                    m_freeSceneSlots.push(static_cast<AzPhysics::SceneIndex>(index));
                }
            }
        }
    }

    void Box3DSystem::RemoveScenes(const AzPhysics::SceneHandleList& handles)
    {
        for (const auto& handle : handles)
        {
            RemoveScene(handle);
        }
    }

    void Box3DSystem::RemoveAllScenes()
    {
        m_sceneList.clear();

        //clear the free slots queue
        AZStd::queue<AzPhysics::SceneIndex> empty;
        m_freeSceneSlots.swap(empty);
    }

    AZStd::pair<AzPhysics::SceneHandle, AzPhysics::SimulatedBodyHandle> Box3DSystem::FindAttachedBodyHandleFromEntityId(
        AZ::EntityId entityId)
    {
        for (auto& scenePtr : m_sceneList)
        {
            if (scenePtr == nullptr)
            {
                continue;
            }
            if (auto* box3DScene = azdynamic_cast<Box3DScene*>(scenePtr.get()))
            {
                for (const auto& [_, body] : box3DScene->GetSimulatedBodyList())
                {
                    if (body != nullptr && body->GetEntityId() == entityId)
                    {
                        return AZStd::make_pair(box3DScene->GetSceneHandle(), body->m_bodyHandle);
                    }
                }
            }
        }
        return AZStd::make_pair(AzPhysics::InvalidSceneHandle, AzPhysics::InvalidSimulatedBodyHandle);
    }

    const AzPhysics::SystemConfiguration* Box3DSystem::GetConfiguration() const
    {
        return &m_systemConfig;
    }

    void Box3DSystem::UpdateConfiguration(const AzPhysics::SystemConfiguration* newConfig, [[maybe_unused]] bool forceReinitialization)
    {
        if (const auto* box3DConfig = azdynamic_cast<const Box3DSystemConfiguration*>(newConfig);
            m_systemConfig != (*box3DConfig))
        {
            m_systemConfig = (*box3DConfig);
            m_configChangeEvent.Signal(box3DConfig);

            // Restarting the simulation if required
        }
    }
    
    const Box3DSystemConfiguration& Box3DSystem::GetBox3DConfiguration() const
    {
        return m_systemConfig;
    }

    void Box3DSystem::UpdateDefaultSceneConfiguration(const AzPhysics::SceneConfiguration& sceneConfiguration)
    {
        if (m_defaultSceneConfiguration != sceneConfiguration)
        {
            m_defaultSceneConfiguration = sceneConfiguration;

            m_onDefaultSceneConfigurationChangedEvent.Signal(&m_defaultSceneConfiguration);
        }
    }

    const AzPhysics::SceneConfiguration& Box3DSystem::GetDefaultSceneConfiguration() const
    {
        return m_defaultSceneConfiguration;
    }
    
    const Box3DSettingsRegistryManager& Box3DSystem::GetSettingsRegistryManager() const
    {
        return *m_registryManager;
    }

    void Box3DSystem::SetCollisionLayerName(int index, const AZStd::string& layerName)
    {
        m_systemConfig.m_collisionConfig.m_collisionLayers.SetName(aznumeric_cast<AZ::u64>(index), layerName);
    }

    void Box3DSystem::CreateCollisionGroup(const AZStd::string& groupName, const AzPhysics::CollisionGroup& group)
    {
        m_systemConfig.m_collisionConfig.m_collisionGroups.CreateGroup(groupName, group);
    }

    AZ::Debug::PerformanceCollector* Box3DSystem::GetPerformanceCollector()
    {
        return m_performanceCollector.get();
    }

    void Box3DSystem::InitializePerformanceCollector()
    {
        auto performanceMetrics = AZStd::to_array<AZStd::string_view>({
            PerformanceSpecBox3DSimulationTime,
        });

        AZStd::string platformName = AZ::GetPlatformName(AZ::g_currentPlatform);
        auto logCategory =
            AZStd::string::format("%.*s-%s", AZ_STRING_ARG(PerformanceLogCategory), platformName.c_str());
        auto fileExtension =
            AZStd::string::format("%.*s.json", AZ_STRING_ARG(PerformanceLogCategory));
        AZStd::to_lower(fileExtension.begin(), fileExtension.end());
        m_performanceCollector = AZStd::make_unique<AZ::Debug::PerformanceCollector>(
            logCategory,
            performanceMetrics,
            [](AZ::u32)
            {
            },
            fileExtension);

        m_performanceCollector->UpdateDataLogType(GetDataLogTypeFromCVar(box3d_metricsDataLogType));
        m_performanceCollector->UpdateFrameCountPerCaptureBatch(box3d_metricsFrameCountPerCaptureBatch);
        m_performanceCollector->UpdateWaitTimeBeforeEachBatch(AZStd::chrono::seconds(box3d_metricsWaitTimePerCaptureBatch));
        m_performanceCollector->UpdateNumberOfCaptureBatches(box3d_metricsNumberOfCaptureBatches);
    }

    Box3DSystem* GetBox3DSystem()
    {
        return azdynamic_cast<Box3DSystem*>(AZ::Interface<AzPhysics::SystemInterface>::Get());
    }
}
