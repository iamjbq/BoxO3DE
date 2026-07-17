
#pragma once

#include <Tools/System/Box3DSubComponentModeBase.h>
#include <AzToolsFramework/ComponentModes/BoxViewportEdit.h>

namespace B3
{
    /// Subcomponent mode for modifying the box dimensions on a collider.
    class ColliderBoxMode : public B3::Box3DSubComponentModeBase
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL

        ColliderBoxMode();

        // Box3DSubComponentModeBase ...
        void Setup(const AZ::EntityComponentIdPair& idPair) override;
        void Refresh(const AZ::EntityComponentIdPair& idPair) override;
        void Teardown(const AZ::EntityComponentIdPair& idPair) override;
        void ResetValues(const AZ::EntityComponentIdPair& idPair) override;

    private:
        AZStd::unique_ptr<AzToolsFramework::BoxViewportEdit> m_boxEdit;
    };
} //namespace B3
