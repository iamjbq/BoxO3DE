
#pragma once

#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <AzQtComponents/Components/Widgets/VectorInput.h>

namespace AZ
{
    class Matrix3x3;
}

#include <QObject>

namespace B3
{
    
    namespace Editor
    {
        static constexpr AZ::Crc32 InertiaHandler = AZ_CRC_CE("RigidBodyInertia");

        class InertiaPropertyHandler
            : public QObject
            , public AzToolsFramework::PropertyHandler<AZ::Matrix3x3, AzQtComponents::VectorInput>
        {
            Q_OBJECT //AUTOMOC
        public:
            AZ_CLASS_ALLOCATOR(InertiaPropertyHandler, AZ::SystemAllocator);

            AZ::u32 GetHandlerName(void) const override;
            QWidget* CreateGUI(QWidget* parent) override;
            void ConsumeAttribute(AzQtComponents::VectorInput* GUI, AZ::u32 attrib,
                AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;
            void WriteGUIValuesIntoProperty(size_t index, AzQtComponents::VectorInput* GUI,
                AZ::Matrix3x3& instance, AzToolsFramework::InstanceDataNode* node) override;
            bool ReadValuesIntoGUI(size_t index, AzQtComponents::VectorInput* GUI,
                const AZ::Matrix3x3& instance, AzToolsFramework::InstanceDataNode* node) override;
        };
    } // namespace Editor
} // namespace B3
