
#pragma once

#include <Tools/System/Box3DSubComponentModeBase.h>
#include <AzToolsFramework/Manipulators/TranslationManipulators.h>

namespace B3
{
    /// Subcomponent mode for modifying offset on a collider in the viewport.
    class ColliderOffsetMode : public Box3DSubComponentModeBase
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL

        ColliderOffsetMode();

        // Box3DSubComponentModeBase ...
        void Setup(const AZ::EntityComponentIdPair& idPair) override;
        void Refresh(const AZ::EntityComponentIdPair& idPair) override;
        void Teardown(const AZ::EntityComponentIdPair& idPair) override;
        void ResetValues(const AZ::EntityComponentIdPair& idPair) override;

    private:
        void OnManipulatorMoved(
            const AZ::Vector3& startPosition, const AZ::Vector3& offset, const AZ::EntityComponentIdPair& idPair);

        AzToolsFramework::TranslationManipulators m_translationManipulators;
    };
} //namespace B3
