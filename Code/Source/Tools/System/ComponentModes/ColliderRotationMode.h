
#pragma once

#include <Tools/System/Box3DSubComponentModeBase.h>
#include <AzToolsFramework/Manipulators/RotationManipulators.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>

namespace B3
{
    /// Subcomponent mode for modifying the rotation on a collider in the viewport.
    class ColliderRotationMode
        : public Box3DSubComponentModeBase
        , private AzFramework::EntityDebugDisplayEventBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL

        ColliderRotationMode();

        // Box3DSubComponentModeBase ...
        void Setup(const AZ::EntityComponentIdPair& idPair) override;
        void Refresh(const AZ::EntityComponentIdPair& idPair) override;
        void Teardown(const AZ::EntityComponentIdPair& idPair) override;
        void ResetValues(const AZ::EntityComponentIdPair& idPair) override;

    private:
        // AzFramework::EntityDebugDisplayEventBus ...
        void DisplayEntityViewport(
            const AzFramework::ViewportInfo& viewportInfo,
            AzFramework::DebugDisplayRequests& debugDisplay) override;

        AzToolsFramework::RotationManipulators m_rotationManipulators;
    };
} //namespace B3
