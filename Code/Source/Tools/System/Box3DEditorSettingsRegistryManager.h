
#pragma once

#include <AzCore/IO/Path/Path.h>

#include <Configuration/Box3DSettingsRegistryManager.h>

namespace B3
{
    //! Handles loading and saving the settings registry
    class Box3DEditorSettingsRegistryManager : public Box3DSettingsRegistryManager
    {
    public:
        Box3DEditorSettingsRegistryManager();

        // Box3DSystemSettingsRegistry ...
        void SaveSystemConfiguration(const Box3DSystemConfiguration& config, const OnBox3DConfigSaveComplete& saveCallback) const override;
        void SaveDefaultSceneConfiguration(const AzPhysics::SceneConfiguration& config, const OnDefaultSceneConfigSaveComplete& saveCallback) const override;
        void SaveDebugConfiguration(const Debug::DebugConfiguration& config, const OnBox3DDebugConfigSaveComplete& saveCallback) const override;

    private:
        AZ::IO::FixedMaxPath m_box3DConfigurationFilePath = "Registry/box3dsystemconfiguration.setreg";
        AZ::IO::FixedMaxPath m_defaultSceneConfigFilePath = "Registry/box3ddefaultsceneconfiguration.setreg";
        AZ::IO::FixedMaxPath m_debugConfigurationFilePath = "Registry/box3ddebugconfiguration.setreg";

        bool m_initialized = false;
    };
}

