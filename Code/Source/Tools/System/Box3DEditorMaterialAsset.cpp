#include <Tools/System/Box3DEditorMaterialAsset.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace B3
{
    void EditorMaterialAsset::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<B3::EditorMaterialAsset, AZ::Data::AssetData>()
                ->Version(1)
                ->Attribute(AZ::Edit::Attributes::EnableForAssetEditor, true)
                ->Field("MaterialConfiguration", &EditorMaterialAsset::m_materialConfiguration)
                ;

            if (auto* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<B3::EditorMaterialAsset>("", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &EditorMaterialAsset::m_materialConfiguration, "Box3D Material",
                        "Box3D material properties")
                        ->Attribute(AZ::Edit::Attributes::ForceAutoExpand, true);
            }
        }
    }

    const MaterialConfiguration& EditorMaterialAsset::GetMaterialConfiguration() const
    {
        return m_materialConfiguration;
    }
} // B3
