#include "Box3DSettingsRegistryManager.h"

#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>

#include <AzFramework/Physics/Configuration/CollisionConfiguration.h>

namespace B3
{
#if !defined(BOXO3DE_SETREG_GEM_NAME)
    #error "Missing required BOXO3DE_SETREG_GEM_NAME definition"
#endif //!defined(BOXO3DE_SETREG_GEM_NAME)

    Box3DSettingsRegistryManager::Box3DSettingsRegistryManager()
    {
        m_settingsRegistryPath = AZStd::string::format("%s/Gems/" BOXO3DE_SETREG_GEM_NAME "/Box3DSystemConfiguration", AZ::SettingsRegistryMergeUtils::OrganizationRootKey);
        m_defaultSceneConfigSettingsRegistryPath = AZStd::string::format("%s/Gems/" BOXO3DE_SETREG_GEM_NAME "/DefaultSceneConfiguration", AZ::SettingsRegistryMergeUtils::OrganizationRootKey);
        // m_debugSettingsRegistryPath = AZStd::string::format("%s/Gems/" BOXO3DE_SETREG_GEM_NAME "/Debug/Box3DDebugConfiguration", AZ::SettingsRegistryMergeUtils::OrganizationRootKey);
    }

    AZStd::optional<Box3DSystemConfiguration> Box3DSettingsRegistryManager::LoadSystemConfiguration() const
    {
        Box3DSystemConfiguration systemConfig;

        bool configurationRead = false;
        
        AZ::SettingsRegistryInterface* settingsRegistry = AZ::SettingsRegistry::Get();
        if (settingsRegistry)
        {
            configurationRead = settingsRegistry->GetObject(systemConfig, m_settingsRegistryPath);
        }

        if (configurationRead)
        {
            AZ_TracePrintf("Box3DSystem", R"(Box3DConfiguration was read from settings registry at pointer path)"
                R"( "%s)" "\n",
                m_settingsRegistryPath.c_str());
            return systemConfig;
        }
        return AZStd::nullopt;
    }

    AZStd::optional<AzPhysics::SceneConfiguration> Box3DSettingsRegistryManager::LoadDefaultSceneConfiguration() const
    {
        AzPhysics::SceneConfiguration sceneConfig;
        bool configurationRead = false;

        AZ::SettingsRegistryInterface* settingsRegistry = AZ::SettingsRegistry::Get();
        if (settingsRegistry)
        {
            configurationRead = settingsRegistry->GetObject(sceneConfig, m_defaultSceneConfigSettingsRegistryPath);
        }

        if (configurationRead)
        {
            AZ_TracePrintf("Box3DSystem", R"(Default Scene Configuration was read from settings registry at pointer path)"
                R"("%s)" "\n",
                m_defaultSceneConfigSettingsRegistryPath.c_str());
            return sceneConfig;
        }
        return AZStd::nullopt;
    }

    // AZStd::optional<Debug::DebugConfiguration> Box3DSettingsRegistryManager::LoadDebugConfiguration() const
    // {
    //     Debug::DebugConfiguration systemConfig;
    //
    //     bool configurationRead = false;
    //
    //     AZ::SettingsRegistryInterface* settingsRegistry = AZ::SettingsRegistry::Get();
    //     if (settingsRegistry)
    //     {
    //         configurationRead = settingsRegistry->GetObject(systemConfig, m_debugSettingsRegistryPath);
    //     }
    //
    //     if (configurationRead)
    //     {
    //         AZ_TracePrintf("Box3DSystem", R"(Debug::DebugConfiguration was read from settings registry at pointer path)"
    //             R"( "%s)" "\n",
    //             m_debugSettingsRegistryPath.c_str());
    //         return systemConfig;
    //     }
    //     return AZStd::nullopt;
    // }

    void Box3DSettingsRegistryManager::SaveSystemConfiguration([[maybe_unused]] const Box3DSystemConfiguration& config, const OnBox3DConfigSaveComplete& saveCallback) const
    {
        //Box3DSettingsRegistryManager will implement, as saving is editor only currently.
        if (saveCallback)
        {
            saveCallback(config, Result::Failed);
        }
    }

    void Box3DSettingsRegistryManager::SaveDefaultSceneConfiguration([[maybe_unused]] const AzPhysics::SceneConfiguration& config, const OnDefaultSceneConfigSaveComplete& saveCallback) const
    {
        //Box3DSettingsRegistryManager will implement, as saving is editor only currently.
        if (saveCallback)
        {
            saveCallback(config, Result::Failed);
        }
    }

    // void Box3DSettingsRegistryManager::SaveDebugConfiguration([[maybe_unused]] const Debug::DebugConfiguration& config, const OnBox3DDebugConfigSaveComplete& saveCallback) const
    // {
    //     //Box3DSettingsRegistryManager will implement, as saving is editor only currently.
    //     if (saveCallback)
    //     {
    //         saveCallback(config, Result::Failed);
    //     }
    // }
}
