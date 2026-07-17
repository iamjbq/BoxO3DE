
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI_Internals.h>
#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>
#include <AzFramework/Physics/Configuration/SceneConfiguration.h>
#include <BoxO3DE/Configuration/Box3DConfiguration.h>
#endif

#include <QWidget>
// #include <BoxO3DE/Debug/Box3DDebugInterface.h>

namespace B3
{
    namespace Editor
    {
        class DocumentationLinkWidget;

        class SettingsWidget
            : public QWidget
            , private AzToolsFramework::IPropertyEditorNotify
        {
            Q_OBJECT

        public:
            AZ_CLASS_ALLOCATOR(SettingsWidget, AZ::SystemAllocator);

            explicit SettingsWidget(QWidget* parent = nullptr);

            void SetValue(const B3::Box3DSystemConfiguration& box3DSystemConfiguration,
                const AzPhysics::SceneConfiguration& defaultSceneConfiguration
                // const Debug::DebugDisplayData& debugDisplayData
                );

        signals:
            void onValueChanged(const B3::Box3DSystemConfiguration& box3DSystemConfiguration,
                const AzPhysics::SceneConfiguration& defaultSceneConfiguration
                // const B3::Debug::DebugDisplayData& debugDisplayData
                );

        private:
            void CreatePropertyEditor(QWidget* parent);

            void BeforePropertyModified(AzToolsFramework::InstanceDataNode* /*node*/) override;
            void AfterPropertyModified(AzToolsFramework::InstanceDataNode* /*node*/) override;
            void SetPropertyEditingActive(AzToolsFramework::InstanceDataNode* /*node*/) override;
            void SetPropertyEditingComplete(AzToolsFramework::InstanceDataNode* /*node*/) override;
            void SealUndoStack() override;

            AzToolsFramework::ReflectedPropertyEditor* m_propertyEditor;
            DocumentationLinkWidget* m_documentationLinkWidget;
            B3::Box3DSystemConfiguration m_box3DSystemConfiguration;
            AzPhysics::SceneConfiguration m_defaultSceneConfiguration;
            // B3::Debug::DebugDisplayData m_debugDisplayData;
        };
    }
} // B3
