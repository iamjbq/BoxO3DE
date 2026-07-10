
#pragma once

#include <AzCore/std/optional.h>
#include <AzFramework/Physics/Configuration/SceneConfiguration.h>
#include <BoxO3DE/Configuration/Box3DConfiguration.h>

namespace B3
{
    //! Handles loading and saving the settings registry
    class Box3DSettingsRegistryManager
    {
    public:
        enum class Result : AZ::u8
        {
            Success,
            Failed
        };
        
        using OnBox3DConfigSaveComplete = AZStd::function<void(const Box3DSystemConfiguration&, Result)>;
        using OnDefaultSceneConfigSaveComplete = AZStd::function<void(const AzPhysics::SceneConfiguration&, Result)>;
        // using OnBox3DDebugConfigSaveComplete = AZStd::function<void(const Debug::DebugConfiguration&, Result)>;

        Box3DSettingsRegistryManager();
        virtual ~Box3DSettingsRegistryManager() = default;

        //! Load the Jolt Configuration from the Settings Registry
        //! @return Returns true if successful.
        virtual AZStd::optional<Box3DSystemConfiguration> LoadSystemConfiguration() const;

        //! Load the Default Scene Configuration from the Settings Registry
        //! @return Returns true if successful.
        virtual AZStd::optional<AzPhysics::SceneConfiguration> LoadDefaultSceneConfiguration() const;

        //! Load the Jolt Debug Configuration from the Settings Registry
        //! @return Returns true if successful.
        // virtual AZStd::optional<Debug::DebugConfiguration> LoadDebugConfiguration() const;

        //! Save the Jolt Configuration from the Settings Registry
        //! @return Returns true if successful. When not in Editor, always returns false.
        virtual void SaveSystemConfiguration(const Box3DSystemConfiguration& config, const OnBox3DConfigSaveComplete& saveCallback) const;

        //! Save the Default Scene Configuration  from the Settings Registry
        //! @return Returns true if successful. When not in Editor, always returns false.
        virtual void SaveDefaultSceneConfiguration(const AzPhysics::SceneConfiguration& config, const OnDefaultSceneConfigSaveComplete& saveCallback) const;

        //! Save the Jolt Debug Configuration from the Settings Registry
        //! @return Returns true if successful. When not in Editor, always returns false.
        // virtual void SaveDebugConfiguration(const Debug::DebugConfiguration& config, const OnJoltDebugConfigSaveComplete& saveCallback) const;

    protected:
        AZStd::string m_settingsRegistryPath;
        AZStd::string m_defaultSceneConfigSettingsRegistryPath;
        // AZStd::string m_debugSettingsRegistryPath;
    };
}
