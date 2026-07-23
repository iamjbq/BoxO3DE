#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>
#include <AzToolsFramework/UI/PropertyEditor/InstanceDataHierarchy.h>
#include <QBoxLayout>
#include <Tools/System/SettingsWidget.h>
#include <Tools/System/DocumentationLinkWidget.h>
#include <NameConstants.h>

namespace B3
{
    namespace Editor
    {
        static const char* const s_settingsDocumentationLink = "Learn more about <a href=%0>configuring Box3D.</a>";
        static const char* const s_settingsDocumentationAddress = "configuring/configuration-global";

        SettingsWidget::SettingsWidget(QWidget* parent)
            : QWidget(parent)
        {
            CreatePropertyEditor(this);
        }

        void SettingsWidget::SetValue(const B3::Box3DSystemConfiguration& box3DSystemConfiguration,
            const AzPhysics::SceneConfiguration& defaultSceneConfiguration,
            const Debug::DebugDisplayData& debugDisplayData
            )
        {
            m_box3DSystemConfiguration = box3DSystemConfiguration;
            m_defaultSceneConfiguration = defaultSceneConfiguration;
            m_debugDisplayData = debugDisplayData;

            blockSignals(true);
            m_propertyEditor->ClearInstances();
            m_propertyEditor->AddInstance(&m_box3DSystemConfiguration.m_capacityConfiguration);
            m_propertyEditor->AddInstance(&m_box3DSystemConfiguration);
            m_propertyEditor->AddInstance(&m_defaultSceneConfiguration);
            m_propertyEditor->AddInstance(&m_debugDisplayData);
            m_propertyEditor->InvalidateAll();
            blockSignals(false);
        }

        void SettingsWidget::CreatePropertyEditor(QWidget* parent)
        {
            QVBoxLayout* verticalLayout = new QVBoxLayout(parent);
            verticalLayout->setContentsMargins(0, 0, 0, 0);
            verticalLayout->setSpacing(0);

            m_documentationLinkWidget = new DocumentationLinkWidget(s_settingsDocumentationLink, (UXNameConstants::GetBox3DDocsRoot() + s_settingsDocumentationAddress).c_str());

            AZ::SerializeContext* m_serializeContext;
            AZ::ComponentApplicationBus::BroadcastResult(m_serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
            AZ_Assert(m_serializeContext, "Failed to retrieve serialize context.");

            const int propertyLabelWidth = 250;
            m_propertyEditor = new AzToolsFramework::ReflectedPropertyEditor(parent);
            m_propertyEditor->Setup(m_serializeContext, this, true, propertyLabelWidth);
            m_propertyEditor->show();
            m_propertyEditor->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

            verticalLayout->addWidget(m_documentationLinkWidget);
            verticalLayout->addWidget(m_propertyEditor);
        }

        void SettingsWidget::BeforePropertyModified(AzToolsFramework::InstanceDataNode* /*node*/)
        {
        }

        void SettingsWidget::AfterPropertyModified(AzToolsFramework::InstanceDataNode* /*node*/)
        {
        }

        void SettingsWidget::SetPropertyEditingActive(AzToolsFramework::InstanceDataNode* /*node*/)
        {
        }

        void SettingsWidget::SetPropertyEditingComplete(AzToolsFramework::InstanceDataNode* /*node*/)
        {
            emit onValueChanged(m_box3DSystemConfiguration,
                m_defaultSceneConfiguration,
                m_debugDisplayData
            );
        }

        void SettingsWidget::SealUndoStack()
        {
        }
    } // Editor
} // B3

#include <Tools/System/moc_SettingsWidget.cpp>
