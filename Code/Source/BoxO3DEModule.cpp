#include <BoxO3DEModule.h>

#include <AzCore/Module/DynamicModuleHandle.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <Clients/BoxO3DESystemComponent.h>
#include <Clients/ComponentDescriptors.h>
#include <Configuration/Box3DSettingsRegistryManager.h>

#if defined(BOXO3DE_EDITOR)
#include <Tools/BoxO3DEEditorSystemComponent.h>
#include <Tools/EditorComponentDescriptors.h>
#include <Tools/System/Box3DEditorSettingsRegistryManager.h>
#endif

namespace B3
{
    BoxO3DEModule::BoxO3DEModule()
        : AZ::Module()
#if defined(BOXO3DE_EDITOR)
        , m_box3DSystem(AZStd::make_unique<Box3DEditorSettingsRegistryManager>())
#else
        , m_box3DSystem(AZStd::make_unique<Box3DSettingsRegistryManager>())
#endif
    {
        static_assert(alignof(B3::Box3DSystemConfiguration) == 16);
        static_assert(alignof(B3::Box3DSystem) == 16);

        LoadModules();

        // This will associate the AzTypeInfo information for the components with the SerializeContext, BehaviorContext and EditContext.
        AZStd::list<AZ::ComponentDescriptor*> descriptorsToAdd = B3::GetDescriptors();
        m_descriptors.insert(m_descriptors.end(), descriptorsToAdd.begin(), descriptorsToAdd.end());
#if defined(BOXO3DE_EDITOR)
        AZStd::list<AZ::ComponentDescriptor*> editorDescriptorsToAdd = B3::GetEditorDescriptors();
        m_descriptors.insert(m_descriptors.end(), editorDescriptorsToAdd.begin(), editorDescriptorsToAdd.end());
#endif
    }

    BoxO3DEModule::~BoxO3DEModule()
    {
        m_box3DSystem.Shutdown();

        AZ::GetGlobalSerializeContextModule().Cleanup();

        UnloadModules();
    }

    AZ::ComponentTypeList BoxO3DEModule::GetRequiredSystemComponents() const
    {
        return AZ::ComponentTypeList{
            azrtti_typeid<BoxO3DESystemComponent>(),
#if defined(BOXO3DE_EDITOR)
            azrtti_typeid<BoxO3DEEditorSystemComponent>(),
#endif
        };
    }

    void BoxO3DEModule::LoadModules()
    {
#if defined(BOXO3DE_EDITOR)
        {
            AZStd::unique_ptr<AZ::DynamicModuleHandle> sceneCoreModule = AZ::DynamicModuleHandle::Create("SceneCore");
            [[maybe_unused]] bool ok = sceneCoreModule->Load(AZ::DynamicModuleHandle::LoadFlags::InitFuncRequired);
            AZ_Error("B3::BoxO3DEModule", ok, "Error loading SceneCore module");

            m_modules.push_back(AZStd::move(sceneCoreModule));
        }
#endif
    }

    void BoxO3DEModule::UnloadModules()
    {
        // Unload modules in reserve order that were loaded
        for (auto module = m_modules.rbegin(); module != m_modules.rend(); ++module)
        {
            module->reset();
        }
        m_modules.clear();
    }
}

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), B3::BoxO3DEModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_BoxO3DE, B3::BoxO3DEModule)
#endif
