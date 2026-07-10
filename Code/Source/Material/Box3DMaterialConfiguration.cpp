#include <BoxO3DE/Material/Box3DMaterialConfiguration.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Math/Sfmt.h>
#include <AzFramework/Physics/NameConstants.h>

namespace B3
{
    void MaterialConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<B3::MaterialConfiguration>()
                ->Version(1)
                ->Field("Friction", &MaterialConfiguration::m_Friction)
                ->Field("Restitution", &MaterialConfiguration::m_restitution)
                ->Field("RollingResistance", &MaterialConfiguration::m_rollingResistance)
                ->Field("TangentVelocity", &MaterialConfiguration::m_tangentVelocity)
                ->Field("FrictionCombine", &MaterialConfiguration::m_frictionCombine)
                ->Field("RestitutionCombine", &MaterialConfiguration::m_restitutionCombine)
                // ->Field("Density", &MaterialConfiguration::m_density)
                ->Field("DebugColor", &MaterialConfiguration::m_debugColor)
                ;

            if (auto* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<B3::MaterialConfiguration>("", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "Jolt Material")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MaterialConfiguration::m_Friction, "Friction", "Friction coefficient for object movement.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
						->Attribute(AZ::Edit::Attributes::Max, 1.f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MaterialConfiguration::m_restitution, "Restitution", "Restitution coefficient, also referred to as bounciness.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MaterialConfiguration::m_rollingResistance, "Rolling Resistance", "Rolling resistance coefficient, only applies to spheres or capsules.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MaterialConfiguration::m_tangentVelocity, "Tangent Velocity", "Velocity on the surface of the material. Provides a way to create conveyor belts.")
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &MaterialConfiguration::m_frictionCombine, "Friction combine", "How the friction is combined between colliding objects. Default is geometric mean: sqrt(friction1 * friction2).")
                        ->EnumAttribute(CombineMode::Average, "Average")
                        ->EnumAttribute(CombineMode::Minimum, "Minimum")
                        ->EnumAttribute(CombineMode::Maximum, "Maximum")
                        ->EnumAttribute(CombineMode::Multiply, "Multiply")
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &MaterialConfiguration::m_restitutionCombine, "Restitution combine", "How the restitution is combined between colliding objects. Default is max(restitution1, restitution1).")
                        ->EnumAttribute(CombineMode::Average, "Average")
                        ->EnumAttribute(CombineMode::Minimum, "Minimum")
                        ->EnumAttribute(CombineMode::Maximum, "Maximum")
                        ->EnumAttribute(CombineMode::Multiply, "Multiply")
                    // ->DataElement(AZ::Edit::UIHandlers::Default, &MaterialConfiguration::m_density, "Density", "Material density")
                    //     ->Attribute(AZ::Edit::Attributes::Min, &MaterialConfiguration::GetMinDensityLimit)
                    //     ->Attribute(AZ::Edit::Attributes::Max, &MaterialConfiguration::GetMaxDensityLimit)
                    //     ->Attribute(AZ::Edit::Attributes::Suffix, " " + Physics::NameConstants::GetDensityUnit())
                    ->DataElement(AZ::Edit::UIHandlers::Color, &MaterialConfiguration::m_debugColor, "Debug Color", "Debug color to use for this material")
                    ;
            }
        }
    }

    AZ::Data::Asset<Physics::MaterialAsset> MaterialConfiguration::CreateMaterialAsset() const
    {
        AZ::Data::Asset<Physics::MaterialAsset> materialAsset =
            AZ::Data::AssetManager::Instance().CreateAsset<Physics::MaterialAsset>(
                AZ::Data::AssetId(AZ::Uuid::CreateRandom(), AZ::Sfmt().Rand32())); // add a random u32 to tie b3SurfaceMaterial back to asset material

        const Physics::MaterialAsset::MaterialProperties materialProperties =
        {
            {MaterialConstants::FrictionName, m_Friction},
            {MaterialConstants::RestitutionName, m_restitution},
            // {MaterialConstants::DensityName, m_density},
            {MaterialConstants::RestitutionCombineModeName, static_cast<AZ::u32>(m_restitutionCombine)},
            {MaterialConstants::FrictionCombineModeName, static_cast<AZ::u32>(m_frictionCombine)},
            {MaterialConstants::DebugColorName, m_debugColor}
        };

        materialAsset->SetData(
            MaterialConstants::MaterialAssetType,
            MaterialConstants::MaterialAssetVersion,
            materialProperties);

        return materialAsset;
    }

    void MaterialConfiguration::ValidateMaterialAsset(
        [[maybe_unused]] AZ::Data::Asset<Physics::MaterialAsset> materialAsset)
    {
#if !defined(AZ_RELEASE_BUILD)
        if (!materialAsset)
        {
            AZ_Error("MaterialConfiguration", false, "Invalid material asset")
            return;
        }

        AZ_Error("MaterialConfiguration", materialAsset->GetMaterialType() == MaterialConstants::MaterialAssetType,
            "Material asset '%s' has unexpected material type ('%s'). Expected type is '%.*s'.",
            materialAsset.GetHint().c_str(), materialAsset->GetMaterialType().c_str(), AZ_STRING_ARG(MaterialConstants::MaterialAssetType))

        AZ_Error("MaterialConfiguration", materialAsset->GetVersion() <= MaterialConstants::MaterialAssetVersion,
            "Material asset '%s' has unexpected material version (%u). Expected version is <='%u'.",
            materialAsset.GetHint().c_str(), materialAsset->GetVersion(), MaterialConstants::MaterialAssetVersion)

        auto checkProperties = [materialAsset](AZStd::span<const AZStd::string_view> materialPropertyNames)
        {
            const auto& materialProperties = materialAsset->GetMaterialProperties();

            for (const auto& materialPropertyName : materialPropertyNames)
            {
                AZ_Error(
                    "MaterialConfiguration",
                    materialProperties.find(materialPropertyName) != materialProperties.end(),
                    "Material asset '%s' does not have property '%.*s'.",
                    materialAsset.GetHint().c_str(),
                    AZ_STRING_ARG(materialPropertyName))
            }
        };

        // Check properties from version 1
        {
            const AZStd::fixed_vector materialPropertyNames =
            {
                MaterialConstants::FrictionName,
                MaterialConstants::RestitutionName,
                MaterialConstants::RollingResistanceName,
                MaterialConstants::TangentVelocityName,
                // MaterialConstants::DensityName,
                MaterialConstants::RestitutionCombineModeName,
                MaterialConstants::FrictionCombineModeName,
                MaterialConstants::DebugColorName
            };

            checkProperties(materialPropertyNames);
        }

#endif
    }

    // float MaterialConfiguration::GetMinDensityLimit()
    // {
    //     return MaterialConstants::MinDensityLimit;
    // }
    //
    // float MaterialConfiguration::GetMaxDensityLimit()
    // {
    //     return MaterialConstants::MaxDensityLimit;
    // }
}
