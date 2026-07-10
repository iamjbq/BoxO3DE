
#pragma once

#if !defined(Q_MOC_RUN)
#include <BoxO3DE/Configuration/Box3DConfiguration.h>
#include <Tools/System/ConfigurationWindowBus.h>
#include <AzFramework/Physics/Configuration/CollisionConfiguration.h>
#include <AzFramework/Physics/Configuration/SceneConfiguration.h>
#endif

#include <QWidget>

namespace AzQtComponents
{
    class TabWidget;
}

namespace B3
{
    namespace Editor
    {
        class SettingsWidget;
        class CollisionFilteringWidget;
        // class PvdWidget;

        /// Widget for editing Box3D configuration and settings.
        ///
        class ConfigurationWidget
            : public QWidget
            , public ConfigurationWindowRequestBus::Handler
        {
            Q_OBJECT

        public:
            AZ_CLASS_ALLOCATOR(ConfigurationWidget, AZ::SystemAllocator);

            explicit ConfigurationWidget(QWidget* parent = nullptr);
            ~ConfigurationWidget() override;

            void SetConfiguration(const B3::Box3DSystemConfiguration& box3DSystemConfiguration,
                                  // const B3::Debug::DebugConfiguration& joltDebugConfiguration,
                                  const AzPhysics::SceneConfiguration& defaultSceneConfiguration);

            // ConfigurationWindowRequestBus
            void ShowCollisionLayersTab() override;
            void ShowCollisionGroupsTab() override;
            void ShowGlobalSettingsTab() override;

            signals:
                void onConfigurationChanged(const B3::Box3DSystemConfiguration& box3DSystemConfiguration,
                                            // const B3::Debug::DebugConfiguration& joltDebugConfig,
                                            const AzPhysics::SceneConfiguration& defaultSceneConfiguration);

        private:
            AzPhysics::SceneConfiguration m_defaultSceneConfiguration;
            B3::Box3DSystemConfiguration m_box3DSystemConfiguration;
            // B3::Debug::DebugConfiguration m_box3DDebugConfiguration;

            AzQtComponents::TabWidget* m_tabs;
            SettingsWidget* m_settings;
            CollisionFilteringWidget* m_collisionFiltering;
            // PvdWidget* m_pvd;
        };
    }
}
