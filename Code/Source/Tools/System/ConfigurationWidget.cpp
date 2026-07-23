#include <Tools/System/ConfigurationWidget.h>

#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Physics/Collision/CollisionGroups.h>
#include <AzFramework/Physics/Collision/CollisionLayers.h>
#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>
#include <AzToolsFramework/UI/PropertyEditor/InstanceDataHierarchy.h>
#include <AzQtComponents/Components/Widgets/TabWidget.h>
#include <QBoxLayout>

#include <Tools/System/SettingsWidget.h>
#include <Tools/System/CollisionFilteringWidget.h>

namespace B3
{
    namespace Editor
    {
        ConfigurationWidget::ConfigurationWidget(QWidget* parent)
            : QWidget(parent)
        {
            QVBoxLayout* verticalLayout = new QVBoxLayout(this);
            verticalLayout->setContentsMargins(0, 5, 0, 0);
            verticalLayout->setSpacing(0);

            m_tabs = new AzQtComponents::TabWidget(this);
            AzQtComponents::TabWidget::applySecondaryStyle(m_tabs, false);

            m_settings = new SettingsWidget();
            m_collisionFiltering = new CollisionFilteringWidget();
            // m_pvd = new PvdWidget();

            m_tabs->addTab(m_settings, "Global Configuration");
            m_tabs->addTab(m_collisionFiltering, "Collision Filtering");
            // m_tabs->addTab(m_pvd, "Debugger");

            verticalLayout->addWidget(m_tabs);

            connect(m_settings, &SettingsWidget::onValueChanged,
                this, [this](const B3::Box3DSystemConfiguration& box3DSystemConfiguration,
                            const AzPhysics::SceneConfiguration& defaultSceneConfiguration,
                             const Debug::DebugDisplayData& debugDisplayData
                             )
            {
                m_box3DSystemConfiguration = box3DSystemConfiguration;
                m_defaultSceneConfiguration = defaultSceneConfiguration;
                m_box3DDebugConfiguration.m_debugDisplayData = debugDisplayData;
                emit onConfigurationChanged(m_box3DSystemConfiguration,
                    m_box3DDebugConfiguration,
                    m_defaultSceneConfiguration);
            });

            connect(m_collisionFiltering, &CollisionFilteringWidget::onConfigurationChanged,
                this, [this](const AzPhysics::CollisionLayers& layers, const AzPhysics::CollisionGroups& groups)
            {
                m_box3DSystemConfiguration.m_collisionConfig.m_collisionLayers = layers;
                m_box3DSystemConfiguration.m_collisionConfig.m_collisionGroups = groups;
                emit onConfigurationChanged(m_box3DSystemConfiguration,
                    m_box3DDebugConfiguration,
                    m_defaultSceneConfiguration);
            });

            // connect(m_pvd, &PvdWidget::onValueChanged,
            //     this, [this](const Debug::PvdConfiguration& configuration)
            // {
            //     m_box3DDebugConfiguration.m_pvdConfigurationData = configuration;
            //     emit onConfigurationChanged(m_box3DSystemConfiguration, m_box3DDebugConfiguration, m_defaultSceneConfiguration);
            // });

            ConfigurationWindowRequestBus::Handler::BusConnect();
        }

        ConfigurationWidget::~ConfigurationWidget()
        {
            ConfigurationWindowRequestBus::Handler::BusDisconnect();
        }

        void ConfigurationWidget::SetConfiguration(
            const B3::Box3DSystemConfiguration& box3DSystemConfiguration,
            const B3::Debug::DebugConfiguration& box3DDebugConfiguration,
            const AzPhysics::SceneConfiguration& defaultSceneConfiguration)
        {
            m_box3DSystemConfiguration = box3DSystemConfiguration;
            m_defaultSceneConfiguration = defaultSceneConfiguration;
            m_box3DDebugConfiguration = box3DDebugConfiguration;
            m_settings->SetValue(m_box3DSystemConfiguration,
                m_defaultSceneConfiguration,
                m_box3DDebugConfiguration.m_debugDisplayData
                );
            m_collisionFiltering->SetConfiguration(m_box3DSystemConfiguration.m_collisionConfig.m_collisionLayers, m_box3DSystemConfiguration.m_collisionConfig.m_collisionGroups);
            // m_pvd->SetValue(m_box3DDebugConfiguration.m_pvdConfigurationData);
        }

        void ConfigurationWidget::ShowCollisionLayersTab()
        {
            const int index = m_tabs->indexOf(m_collisionFiltering);
            m_tabs->setCurrentIndex(index);
            m_collisionFiltering->ShowLayersTab();
        }

        void ConfigurationWidget::ShowCollisionGroupsTab()
        {
            const int index = m_tabs->indexOf(m_collisionFiltering);
            m_tabs->setCurrentIndex(index);
            m_collisionFiltering->ShowGroupsTab();
        }

        void ConfigurationWidget::ShowGlobalSettingsTab()
        {
            const int index = m_tabs->indexOf(m_settings);
            m_tabs->setCurrentIndex(index);
        }
    }
}

#include <Tools/System/moc_ConfigurationWidget.cpp>
