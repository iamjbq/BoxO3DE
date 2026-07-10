
#pragma once

#include <BoxO3DE/BoxO3DETypeIds.h>

#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/Memory_fwd.h>
#include <AzCore/Module/Module.h>
#include <AzCore/RTTI/RTTIMacros.h>
#include <AzCore/RTTI/TypeInfoSimple.h>

#include <System/Box3DSystem.h>

namespace AZ
{
    class DynamicModuleHandle;
}

namespace B3
{
    class BoxO3DEModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(BoxO3DEModule, BoxO3DEModuleTypeId, AZ::Module);
        AZ_CLASS_ALLOCATOR(BoxO3DEModule, AZ::SystemAllocator)

        BoxO3DEModule();
        virtual ~BoxO3DEModule();

        AZ::ComponentTypeList GetRequiredSystemComponents() const override;

    private:
        void LoadModules();
        void UnloadModules();

        /// Required modules to load/unload when Box3D Gem module is created/destroyed
        AZStd::vector<AZStd::unique_ptr<AZ::DynamicModuleHandle>> m_modules;
        Box3DSystem m_box3DSystem;
    };
}


