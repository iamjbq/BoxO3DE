#include <Tools/System/PropertyTypes.h>

#include <Tools/System/CollisionLayerWidget.h>
#include <Tools/System/CollisionGroupWidget.h>
// #include <Tools/System/InertiaPropertyHandler.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>

namespace B3
{
    namespace Editor
    {
        void RegisterPropertyTypes()
        {
            AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(&AzToolsFramework::PropertyTypeRegistrationMessages::RegisterPropertyType, aznew B3::Editor::CollisionLayerWidget());
            AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(&AzToolsFramework::PropertyTypeRegistrationMessages::RegisterPropertyType, aznew B3::Editor::CollisionGroupWidget());
            AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(&AzToolsFramework::PropertyTypeRegistrationMessages::RegisterPropertyType, aznew B3::Editor::CollisionGroupEnumPropertyComboBoxHandler());
            // AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(&AzToolsFramework::PropertyTypeRegistrationMessages::RegisterPropertyType, aznew B3::Editor::InertiaPropertyHandler());
        }

        void UnregisterPropertyTypes()
        {
            AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(&AzToolsFramework::PropertyTypeRegistrationMessages::UnregisterPropertyType, aznew B3::Editor::CollisionLayerWidget());
            AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(&AzToolsFramework::PropertyTypeRegistrationMessages::UnregisterPropertyType, aznew B3::Editor::CollisionGroupWidget());
            AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(&AzToolsFramework::PropertyTypeRegistrationMessages::UnregisterPropertyType, aznew B3::Editor::CollisionGroupEnumPropertyComboBoxHandler());
            // AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(&AzToolsFramework::PropertyTypeRegistrationMessages::UnregisterPropertyType, aznew B3::Editor::InertiaPropertyHandler());
        }
    }
}
