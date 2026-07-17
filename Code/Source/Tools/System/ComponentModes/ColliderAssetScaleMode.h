
#pragma once

#include <Tools/System/Box3DSubComponentModeBase.h>
#include <AzToolsFramework/Manipulators/ScaleManipulators.h>

namespace B3
{
    /// Subcomponent mode for modifying the asset scale on a collider in the viewport.
    class ColliderAssetScaleMode : public B3::Box3DSubComponentModeBase
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL

        ColliderAssetScaleMode();

        // Box3DSubComponentModeBase ...
        void Setup(const AZ::EntityComponentIdPair& idPair) override;
        void Refresh(const AZ::EntityComponentIdPair& idPair) override;
        void Teardown(const AZ::EntityComponentIdPair& idPair) override;
        void ResetValues(const AZ::EntityComponentIdPair& idPair) override;

    private:

        void OnManipulatorDown(const AZ::EntityComponentIdPair& idPair);
        void OnManipulatorMoved(const AZ::Vector3& scale, const AZ::EntityComponentIdPair& idPair);

        AZ::Vector3 m_initialScale;
        AzToolsFramework::ScaleManipulators m_dimensionsManipulators;
    };
} //namespace B3
