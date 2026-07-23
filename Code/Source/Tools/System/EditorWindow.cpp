#include <Tools/System/EditorWindow.h>

#include <AzCore/Interface/Interface.h>
#include <AzFramework/Physics/CollisionBus.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Physics/Configuration/CollisionConfiguration.h>
#include <AzFramework/Physics/Configuration/SceneConfiguration.h>
#include <AzToolsFramework/API/ViewPaneOptions.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <LyViewPaneNames.h>

#include <Tools/System/ui_EditorWindow.h>

#include <BoxO3DE/Configuration/Box3DConfiguration.h>
#include <Tools/System/ConfigurationWidget.h>
#include <System/Box3DSystem.h>
#include <BoxO3DE/Debug/Box3DDebugConfiguration.h>

namespace B3
{
    namespace Editor
    {
        EditorWindow::EditorWindow(QWidget* parent)
            : QWidget(parent)
            , m_ui(new Ui::EditorWindowClass())
        {
            m_ui->setupUi(this);

            auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get();
            const auto* box3DSystemConfiguration = azdynamic_cast<const B3::Box3DSystemConfiguration*>(physicsSystem->GetConfiguration());
            const AzPhysics::SceneConfiguration& defaultSceneConfiguration = physicsSystem->GetDefaultSceneConfiguration();
            const B3::Debug::DebugConfiguration& box3DDebugConfiguration = AZ::Interface<B3::Debug::Box3DDebugInterface>::Get()->GetDebugConfiguration();
            

            m_ui->m_box3DConfigurationWidget->SetConfiguration(
                *box3DSystemConfiguration,
                box3DDebugConfiguration,
                defaultSceneConfiguration
                );
            connect(m_ui->m_box3DConfigurationWidget, &B3::Editor::ConfigurationWidget::onConfigurationChanged,
                this, &EditorWindow::SaveConfiguration);
        }

        EditorWindow::~EditorWindow()
        {
        }
        
        void EditorWindow::RegisterViewClass()
        {
            AzToolsFramework::ViewPaneOptions options;
            options.preferedDockingArea = Qt::LeftDockWidgetArea;
            options.saveKeyName = "Box3DConfiguration";
            options.isPreview = true;
            AzToolsFramework::RegisterViewPane<EditorWindow>(LyViewPane::Box3DConfigurationEditor, LyViewPane::CategoryTools, options); // Adding to the tools menu
        }

        void EditorWindow::SaveConfiguration(
            const B3::Box3DSystemConfiguration& box3DSystemConfiguration,
            const B3::Debug::DebugConfiguration& box3DDebugConfig,
            const AzPhysics::SceneConfiguration& defaultSceneConfiguration)
        {
            auto* box3DSystem = GetBox3DSystem();
            if (box3DSystem == nullptr)
            {
                AZ_Error("Box3D", false, "Unable to save the Box3D configuration. The Box3DSystem is not initialized. Any changes have not been applied.");
                return;
            }

            //update the Box3D system config if it has changed
            const Box3DSettingsRegistryManager& settingsRegManager = box3DSystem->GetSettingsRegistryManager();
            if (box3DSystem->GetBox3DConfiguration() != box3DSystemConfiguration)
            {
                auto saveCallback = [](const Box3DSystemConfiguration& config, Box3DSettingsRegistryManager::Result result)
                {
                    AZ_Warning("Box3D", result == Box3DSettingsRegistryManager::Result::Success, "Unable to save the Box3D configuration. Any changes have not been applied.");
                    if (result == Box3DSettingsRegistryManager::Result::Success)
                    {
                        if (auto* box3DSystem = GetBox3DSystem())
                        {
                            box3DSystem->UpdateConfiguration(&config);
                        }
                    }
                };
                settingsRegManager.SaveSystemConfiguration(box3DSystemConfiguration, saveCallback);
            }

            if (box3DSystem->GetDefaultSceneConfiguration() != defaultSceneConfiguration)
            {
                auto saveCallback = [](const AzPhysics::SceneConfiguration& config, Box3DSettingsRegistryManager::Result result)
                {
                    AZ_Warning("Box3D", result == Box3DSettingsRegistryManager::Result::Success, "Unable to save the Default Scene configuration. Any changes have not been applied.");
                    if (result == Box3DSettingsRegistryManager::Result::Success)
                    {
                        if (auto* box3DSystem = GetBox3DSystem())
                        {
                            box3DSystem->UpdateDefaultSceneConfiguration(config);
                        }
                    }
                };
                settingsRegManager.SaveDefaultSceneConfiguration(defaultSceneConfiguration, saveCallback);
            }

            // Update the debug configuration
             if (auto* box3DDebug = AZ::Interface<Debug::Box3DDebugInterface>::Get())
             {
                 if (box3DDebug->GetDebugConfiguration() != box3DDebugConfig)
                 {
                     auto saveCallback = [](const Debug::DebugConfiguration& config, Box3DSettingsRegistryManager::Result result)
                     {
                         AZ_Warning("Box3D", result == Box3DSettingsRegistryManager::Result::Success, "Unable to save the Box3D debug configuration. Any changes have not been applied.");
                         if (result == Box3DSettingsRegistryManager::Result::Success)
                         {
                             if (auto* box3DDebug = AZ::Interface<Debug::Box3DDebugInterface>::Get())
                             {
                                 box3DDebug->UpdateDebugConfiguration(config);
                             }
                         }
                     };
                     settingsRegManager.SaveDebugConfiguration(box3DDebugConfig, saveCallback);
                 }
             }
        }
    }
}
