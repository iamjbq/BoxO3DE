#include <Scene/Box3DSceneInterface.h>

#include <AzFramework/Physics/Common/PhysicsJoint.h>
#include <AzFramework/Physics/Common/PhysicsSimulatedBody.h>
#include <AzFramework/Physics/Configuration/SceneConfiguration.h>

#include <System/Box3DSystem.h>

namespace B3
{
    namespace Internal
    {
        template<class Handler, class Function>
        void EventRegisterHelper(Box3DSystem* box3DSystem, AzPhysics::SceneHandle sceneHandle, Handler& handler, Function registerFunc)
        {
            if (AzPhysics::Scene* scene = box3DSystem->GetScene(sceneHandle))
            {
                auto func = AZStd::bind(registerFunc, scene, AZStd::placeholders::_1);
                func(handler);
            }
        }
    }
    
    Box3DSceneInterface::Box3DSceneInterface(Box3DSystem* Box3DSystem)
        : m_box3DSystem(Box3DSystem)
    {

    }

    // AzPhysics::SceneInterface ...
    AzPhysics::SceneHandle Box3DSceneInterface::GetSceneHandle(const AZStd::string& sceneName)
    {
        return m_box3DSystem->GetSceneHandle(sceneName);
    }

    AzPhysics::Scene* Box3DSceneInterface::GetScene(AzPhysics::SceneHandle handle)
    {
        return m_box3DSystem->GetScene(handle);
    }

    void Box3DSceneInterface::StartSimulation(AzPhysics::SceneHandle sceneHandle, float deltatime)
    {
        if (AzPhysics::Scene* scene = m_box3DSystem->GetScene(sceneHandle))
        {
            scene->StartSimulation(deltatime);
        }
    }

    void Box3DSceneInterface::FinishSimulation(AzPhysics::SceneHandle sceneHandle)
    {
        if (AzPhysics::Scene* scene = m_box3DSystem->GetScene(sceneHandle))
        {
            scene->FinishSimulation();
        }
    }

    void Box3DSceneInterface::SetEnabled(AzPhysics::SceneHandle sceneHandle, bool enable)
    {
        if (AzPhysics::Scene* scene = m_box3DSystem->GetScene(sceneHandle))
        {
            scene->SetEnabled(enable);
        }
    }

    bool Box3DSceneInterface::IsEnabled(AzPhysics::SceneHandle sceneHandle) const
    {
        if (AzPhysics::Scene* scene = m_box3DSystem->GetScene(sceneHandle))
        {
            return scene->IsEnabled();
        }
        return false;
    }

    AzPhysics::SimulatedBodyHandle Box3DSceneInterface::AddSimulatedBody(AzPhysics::SceneHandle sceneHandle, const AzPhysics::SimulatedBodyConfiguration* simulatedBodyConfig)
    {
        if (AzPhysics::Scene* scene = m_box3DSystem->GetScene(sceneHandle))
        {
            return scene->AddSimulatedBody(simulatedBodyConfig);
        }
        return AzPhysics::InvalidSimulatedBodyHandle;
    }

    AzPhysics::SimulatedBodyHandleList Box3DSceneInterface::AddSimulatedBodies(AzPhysics::SceneHandle sceneHandle, const AzPhysics::SimulatedBodyConfigurationList& simulatedBodyConfigs)
    {
        if (AzPhysics::Scene* scene = m_box3DSystem->GetScene(sceneHandle))
        {
            return scene->AddSimulatedBodies(simulatedBodyConfigs);
        }

        return {}; //return an empty list
    }

    AzPhysics::SimulatedBody* Box3DSceneInterface::GetSimulatedBodyFromHandle(AzPhysics::SceneHandle sceneHandle, AzPhysics::SimulatedBodyHandle bodyHandle)
    {
        if (AzPhysics::Scene* scene = m_box3DSystem->GetScene(sceneHandle))
        {
            return scene->GetSimulatedBodyFromHandle(bodyHandle);
        }
        return nullptr;
    }

    AzPhysics::SimulatedBodyList Box3DSceneInterface::GetSimulatedBodiesFromHandle(AzPhysics::SceneHandle sceneHandle, const AzPhysics::SimulatedBodyHandleList& bodyHandles)
    {
        if (AzPhysics::Scene* scene = m_box3DSystem->GetScene(sceneHandle))
        {
            return scene->GetSimulatedBodiesFromHandle(bodyHandles);
        }
        return {}; //return an empty list
    }

    void Box3DSceneInterface::RemoveSimulatedBody(AzPhysics::SceneHandle sceneHandle, AzPhysics::SimulatedBodyHandle& bodyHandle)
    {
        if (AzPhysics::Scene* scene = m_box3DSystem->GetScene(sceneHandle))
        {
            scene->RemoveSimulatedBody(bodyHandle);
        }
    }

    void Box3DSceneInterface::RemoveSimulatedBodies(AzPhysics::SceneHandle sceneHandle, AzPhysics::SimulatedBodyHandleList& bodyHandles)
    {
        if (AzPhysics::Scene* scene = m_box3DSystem->GetScene(sceneHandle))
        {
            scene->RemoveSimulatedBodies(bodyHandles);
        }
    }

    void Box3DSceneInterface::EnableSimulationOfBody(AzPhysics::SceneHandle sceneHandle, AzPhysics::SimulatedBodyHandle bodyHandle)
    {
        if (AzPhysics::Scene* scene = m_box3DSystem->GetScene(sceneHandle))
        {
            scene->EnableSimulationOfBody(bodyHandle);
        }
    }

    void Box3DSceneInterface::DisableSimulationOfBody(AzPhysics::SceneHandle sceneHandle, AzPhysics::SimulatedBodyHandle bodyHandle)
    {
        if (AzPhysics::Scene* scene = m_box3DSystem->GetScene(sceneHandle))
        {
            scene->DisableSimulationOfBody(bodyHandle);
        }
    }

    AzPhysics::JointHandle Box3DSceneInterface::AddJoint(
        AzPhysics::SceneHandle sceneHandle, const AzPhysics::JointConfiguration* jointConfig, 
        AzPhysics::SimulatedBodyHandle parentBody, AzPhysics::SimulatedBodyHandle childBody) 
    {
        if (AzPhysics::Scene* scene = m_box3DSystem->GetScene(sceneHandle))
        {
            return scene->AddJoint(jointConfig, parentBody, childBody);
        }

        return AzPhysics::InvalidJointHandle;
    }

    AzPhysics::Joint* Box3DSceneInterface::GetJointFromHandle(AzPhysics::SceneHandle sceneHandle, AzPhysics::JointHandle jointHandle) 
    {
        if (AzPhysics::Scene* scene = m_box3DSystem->GetScene(sceneHandle))
        {
            return scene->GetJointFromHandle(jointHandle);
        }

        return nullptr;
    }

    void Box3DSceneInterface::RemoveJoint(AzPhysics::SceneHandle sceneHandle, AzPhysics::JointHandle jointHandle) 
    {
        if (AzPhysics::Scene* scene = m_box3DSystem->GetScene(sceneHandle))
        {
            scene->RemoveJoint(jointHandle);
        }
    }

    AzPhysics::SceneQueryHits Box3DSceneInterface::QueryScene(
        AzPhysics::SceneHandle sceneHandle, const AzPhysics::SceneQueryRequest* request)
    {
        if (AzPhysics::Scene* scene = m_box3DSystem->GetScene(sceneHandle))
        {
            return scene->QueryScene(request);
        }
        return AzPhysics::SceneQueryHits();
    }

    bool Box3DSceneInterface::QueryScene(
        AzPhysics::SceneHandle sceneHandle, const AzPhysics::SceneQueryRequest* request, AzPhysics::SceneQueryHits& result)
    {
        if (AzPhysics::Scene* scene = m_box3DSystem->GetScene(sceneHandle))
        {
            return scene->QueryScene(request, result);
        }
        return false;
    }

    AzPhysics::SceneQueryHitsList Box3DSceneInterface::QuerySceneBatch(
        AzPhysics::SceneHandle sceneHandle, const AzPhysics::SceneQueryRequests& requests)
    {
        if (AzPhysics::Scene* scene = m_box3DSystem->GetScene(sceneHandle))
        {
            return scene->QuerySceneBatch(requests);
        }
        return {}; //return an empty list
    }

    bool Box3DSceneInterface::QuerySceneAsync(
        AzPhysics::SceneHandle sceneHandle, AzPhysics::SceneQuery::AsyncRequestId requestId,
        const AzPhysics::SceneQueryRequest* request, AzPhysics::SceneQuery::AsyncCallback callback)
    {
        if (AzPhysics::Scene* scene = m_box3DSystem->GetScene(sceneHandle))
        {
            return scene->QuerySceneAsync(requestId, request, callback);
        }
        return false;
    }

    bool Box3DSceneInterface::QuerySceneAsyncBatch(
        AzPhysics::SceneHandle sceneHandle, AzPhysics::SceneQuery::AsyncRequestId requestId,
        const AzPhysics::SceneQueryRequests& requests, AzPhysics::SceneQuery::AsyncBatchCallback callback)
    {
        if (AzPhysics::Scene* scene = m_box3DSystem->GetScene(sceneHandle))
        {
            return scene->QuerySceneAsyncBatch(requestId, requests, callback);
        }
        return false;
    }

    void Box3DSceneInterface::SuppressCollisionEvents(AzPhysics::SceneHandle sceneHandle,
        const AzPhysics::SimulatedBodyHandle& bodyHandleA,
        const AzPhysics::SimulatedBodyHandle& bodyHandleB)
    {
        if (AzPhysics::Scene* scene = m_box3DSystem->GetScene(sceneHandle))
        {
            scene->SuppressCollisionEvents(bodyHandleA, bodyHandleB);
        }

    }

    void Box3DSceneInterface::UnsuppressCollisionEvents(AzPhysics::SceneHandle sceneHandle,
        const AzPhysics::SimulatedBodyHandle& bodyHandleA,
        const AzPhysics::SimulatedBodyHandle& bodyHandleB)
    {
        if (AzPhysics::Scene* scene = m_box3DSystem->GetScene(sceneHandle))
        {
            scene->UnsuppressCollisionEvents(bodyHandleA, bodyHandleB);
        }
    }

    void Box3DSceneInterface::SetGravity(AzPhysics::SceneHandle sceneHandle, const AZ::Vector3& gravity)
    {
        if (AzPhysics::Scene* scene = m_box3DSystem->GetScene(sceneHandle))
        {
            scene->SetGravity(gravity);
        }
    }

    AZ::Vector3 Box3DSceneInterface::GetGravity(AzPhysics::SceneHandle sceneHandle) const
    {
        if (AzPhysics::Scene* scene = m_box3DSystem->GetScene(sceneHandle))
        {
            return scene->GetGravity();
        }
        return AZ::Vector3::CreateZero();
    }

    void Box3DSceneInterface::RegisterSceneConfigurationChangedEventHandler(AzPhysics::SceneHandle sceneHandle,
        AzPhysics::SceneEvents::OnSceneConfigurationChanged::Handler& handler)
    {
        Internal::EventRegisterHelper(m_box3DSystem, sceneHandle, handler, &AzPhysics::Scene::RegisterSceneConfigurationChangedEventHandler);
    }
    
    void Box3DSceneInterface::RegisterSimulationBodyAddedHandler(AzPhysics::SceneHandle sceneHandle,
        AzPhysics::SceneEvents::OnSimulationBodyAdded::Handler& handler)
    {
        Internal::EventRegisterHelper(m_box3DSystem, sceneHandle, handler, &AzPhysics::Scene::RegisterSimulationBodyAddedHandler);
    }

    void Box3DSceneInterface::RegisterSimulationBodyRemovedHandler(AzPhysics::SceneHandle sceneHandle,
        AzPhysics::SceneEvents::OnSimulationBodyRemoved::Handler& handler)
    {
        Internal::EventRegisterHelper(m_box3DSystem, sceneHandle, handler, &AzPhysics::Scene::RegisterSimulationBodyRemovedHandler);
    }

    void Box3DSceneInterface::RegisterSimulationBodySimulationEnabledHandler(AzPhysics::SceneHandle sceneHandle,
        AzPhysics::SceneEvents::OnSimulationBodySimulationEnabled::Handler& handler)
    {
        Internal::EventRegisterHelper(m_box3DSystem, sceneHandle, handler, &AzPhysics::Scene::RegisterSimulationBodySimulationEnabledHandler);
    }

    void Box3DSceneInterface::RegisterSimulationBodySimulationDisabledHandler(AzPhysics::SceneHandle sceneHandle,
        AzPhysics::SceneEvents::OnSimulationBodySimulationDisabled::Handler& handler)
    {
        Internal::EventRegisterHelper(m_box3DSystem, sceneHandle, handler, &AzPhysics::Scene::RegisterSimulationBodySimulationDisabledHandler);
    }

    void Box3DSceneInterface::RegisterSceneSimulationStartHandler(AzPhysics::SceneHandle sceneHandle, AzPhysics::SceneEvents::OnSceneSimulationStartHandler& handler)
    {
        Internal::EventRegisterHelper(m_box3DSystem, sceneHandle, handler, &AzPhysics::Scene::RegisterSceneSimulationStartHandler);
    }

    void Box3DSceneInterface::RegisterSceneSimulationFinishHandler(AzPhysics::SceneHandle sceneHandle, AzPhysics::SceneEvents::OnSceneSimulationFinishHandler& handler)
    {
        Internal::EventRegisterHelper(m_box3DSystem, sceneHandle, handler, &AzPhysics::Scene::RegisterSceneSimulationFinishHandler);
    }

    void Box3DSceneInterface::RegisterSceneActiveSimulatedBodiesHandler(AzPhysics::SceneHandle sceneHandle, AzPhysics::SceneEvents::OnSceneActiveSimulatedBodiesEvent::Handler& handler)
    {
        Internal::EventRegisterHelper(m_box3DSystem, sceneHandle, handler, &AzPhysics::Scene::RegisterSceneActiveSimulatedBodiesHandler);
    }

    void Box3DSceneInterface::RegisterSceneCollisionEventHandler(AzPhysics::SceneHandle sceneHandle,
        AzPhysics::SceneEvents::OnSceneCollisionsEvent::Handler& handler)
    {
        Internal::EventRegisterHelper(m_box3DSystem, sceneHandle, handler, &AzPhysics::Scene::RegisterSceneCollisionEventHandler);
    }

    void Box3DSceneInterface::RegisterSceneTriggersEventHandler(AzPhysics::SceneHandle sceneHandle,
        AzPhysics::SceneEvents::OnSceneTriggersEvent::Handler& handler)
    {
        Internal::EventRegisterHelper(m_box3DSystem, sceneHandle, handler, &AzPhysics::Scene::RegisterSceneTriggersEventHandler);
    }

    void Box3DSceneInterface::RegisterSceneGravityChangedEvent(AzPhysics::SceneHandle sceneHandle, AzPhysics::SceneEvents::OnSceneGravityChangedEvent::Handler& handler)
    {
        Internal::EventRegisterHelper(m_box3DSystem, sceneHandle, handler, &AzPhysics::Scene::RegisterSceneGravityChangedEvent);
    }
}
