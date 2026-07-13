
#include <BoxO3DE/Material/Box3DMaterial.h>

#include <AzCore/Interface/Interface.h>
#include <AzFramework/Physics/Material/PhysicsMaterialSlots.h>
#include <AzFramework/Physics/Material/PhysicsMaterialManager.h>

#include <BoxO3DE/Material/Box3DMaterialConfiguration.h>

#include "BoxO3DE/MathConversions.h"

namespace B3
{
    AZStd::shared_ptr<Material> Material::FindOrCreateMaterial(const AZ::Data::Asset<Physics::MaterialAsset>& materialAsset)
    {
        return AZStd::rtti_pointer_cast<B3::Material>(
            AZ::Interface<Physics::MaterialManager>::Get()->FindOrCreateMaterial(
                Physics::MaterialId::CreateFromAssetId(materialAsset.GetId()),
                materialAsset));
    }

    AZStd::vector<AZStd::shared_ptr<Material>> Material::FindOrCreateMaterials(const Physics::MaterialSlots& materialSlots)
    {
        AZStd::shared_ptr<Material> defaultMaterial =
            AZStd::rtti_pointer_cast<B3::Material>(
                AZ::Interface<Physics::MaterialManager>::Get()->GetDefaultMaterial());

        const size_t slotsCount = materialSlots.GetSlotsCount();

        AZStd::vector<AZStd::shared_ptr<Material>> materials;
        materials.reserve(slotsCount);

        for (size_t slotIndex = 0; slotIndex < slotsCount; ++slotIndex)
        {
            if (const auto materialAsset = materialSlots.GetMaterialAsset(slotIndex);
                materialAsset.GetId().IsValid())
            {
                auto material = Material::FindOrCreateMaterial(materialAsset);
                if (material)
                {
                    materials.push_back(material);
                }
                else
                {
                    materials.push_back(defaultMaterial);
                }
            }
            else
            {
                materials.push_back(defaultMaterial);
            }
        }

        return materials;
    }

    AZStd::shared_ptr<Material> Material::CreateMaterialWithRandomId(const AZ::Data::Asset<Physics::MaterialAsset>& materialAsset)
    {
        return AZStd::rtti_pointer_cast<Material>(
            AZ::Interface<Physics::MaterialManager>::Get()->FindOrCreateMaterial(
                Physics::MaterialId::CreateRandom(),
                materialAsset));
    }
    
    Material::~Material()
    {
        AZ::Data::AssetBus::Handler::BusDisconnect();
    }

    Material::Material(const Physics::MaterialId& id, const AZ::Data::Asset<Physics::MaterialAsset>& materialAsset)
        : Physics::Material(id, materialAsset)
    {
        const MaterialConfiguration defaultMaterialConfiguration;
        
        // TODO: b3SurfaceMaterial is passed by value in Box3D, and making a unique_ptr might not make sense
        // Create the JoltPhysicsMaterial with default values
        // m_joltMaterial = JoltMaterialUniquePtr(
        //     new JoltPhysicsMaterial(
        //         "Default",
        //         JPH::Color::sWhite,
        //         defaultMaterialConfiguration.m_Friction, defaultMaterialConfiguration.m_restitution, defaultMaterialConfiguration.m_density
        //     ),
        //     []([[maybe_unused]] B3::JoltPhysicsMaterial* joltPhysicsMaterial)
        //     {
        //         // Nothing to do here yet
        //     });
        
        m_box3DMaterial = {
            defaultMaterialConfiguration.m_Friction,
            defaultMaterialConfiguration.m_restitution,
            defaultMaterialConfiguration.m_rollingResistance,
            Box3DMathConvert(defaultMaterialConfiguration.m_tangentVelocity),
            id.m_subId, // need this to identify in friction/restitution override functions
            defaultMaterialConfiguration.m_debugColor.ToU32()
        };
        
        // AZ_Assert(m_box3DMaterialPtr, "Failed to create Jolt material")
        // m_joltMaterial->m_userData = this;
            
        // Assign default values to members
        m_friction = defaultMaterialConfiguration.m_Friction;
        m_restitution = defaultMaterialConfiguration.m_restitution;
        m_debugColor = defaultMaterialConfiguration.m_debugColor;

        // When OnAssetReady is called, it will set all the properties from the material asset
        AZ::Data::AssetBus::Handler::BusConnect(m_materialAsset.GetId());
    }

    Physics::MaterialPropertyValue Material::GetProperty(AZStd::string_view propertyName) const
    {
        if (propertyName == MaterialConstants::FrictionName)
        {
            return GetFriction();
        }
        else if (propertyName == MaterialConstants::RestitutionName)
        {
            return GetRestitution();
        }
        else if (propertyName == MaterialConstants::DensityName)
        {
            return GetDensity();
        }
        else if (propertyName == MaterialConstants::RollingResistanceName)
        {
            return GetRollingResistance();
        }
        else if (propertyName == MaterialConstants::TangentVelocityName)
        {
            return GetTangentVelocity();
        }
        else if (propertyName == MaterialConstants::RestitutionCombineModeName)
        {
            return static_cast<AZ::u32>(GetRestitutionCombineMode());
        }
        else if (propertyName == MaterialConstants::FrictionCombineModeName)
        {
            return static_cast<AZ::u32>(GetFrictionCombineMode());
        }
        else if (propertyName == MaterialConstants::DebugColorName)
        {
            return GetDebugColor();
        }
        else
        {
            AZ_Error("B3::Material", false, "Unknown property '%.*s'", AZ_STRING_ARG(propertyName));
            return 0.0f;
        }
    }

    void Material::SetProperty(AZStd::string_view propertyName, Physics::MaterialPropertyValue value)
    {
        if (propertyName == MaterialConstants::FrictionName)
        {
            SetFriction(value.GetValue<float>());
        }
        else if (propertyName == MaterialConstants::RestitutionName)
        {
            SetRestitution(value.GetValue<float>());
        }
        else if (propertyName == MaterialConstants::DensityName)
        {
            return SetDensity(value.GetValue<float>());
        }
        else if (propertyName == MaterialConstants::RollingResistanceName)
        {
            SetRollingResistance(value.GetValue<float>());
        }
        else if (propertyName == MaterialConstants::TangentVelocityName)
        {
            SetTangentVelocity(value.GetValue<AZ::Vector3>());
        }
        else if (propertyName == MaterialConstants::RestitutionCombineModeName)
        {
            SetRestitutionCombineMode(static_cast<CombineMode>(value.GetValue<AZ::u32>()));
        }
        else if (propertyName == MaterialConstants::FrictionCombineModeName)
        {
            SetFrictionCombineMode(static_cast<CombineMode>(value.GetValue<AZ::u32>()));
        }
        else if (propertyName == MaterialConstants::DebugColorName)
        {
            SetDebugColor(value.GetValue<AZ::Color>());
        }
        else
        {
            AZ_Error("B3::Material", false, "Unknown property '%.*s'", AZ_STRING_ARG(propertyName));
        }
    }

    float Material::GetFriction() const
    {
        return m_friction;
    }

    void Material::SetFriction(float friction)
    {
        AZ_Warning(
            "Box3D Material", friction >= 0.0f && friction <= 1.0f, "Friction value %f will be clamped into range [0, 1]",
            friction)

        m_friction = AZ::GetClamp(friction, 0.0f, 1.0f);
        
        m_box3DMaterial.friction = m_friction;
    }

    float Material::GetRestitution() const
    {
        return m_restitution;
    }

    void Material::SetRestitution(float restitution)
    {
        AZ_Warning(
            "Box3D Material", restitution >= 0.0f && restitution <= 1.0f, "Restitution value %f will be clamped into range [0, 1]",
            restitution)

        m_restitution = AZ::GetClamp(restitution, 0.0f, 1.0f);
        
        m_box3DMaterial.restitution = m_restitution;
    }

    float Material::GetDensity() const
    {
        return m_density;
    }

    void Material::SetDensity(float density)
    {
        AZ_Warning(
            "Box3D Material", density >= MaterialConstants::MinDensityLimit && density <= MaterialConstants::MaxDensityLimit,
            "Density value %f will be clamped into range [%f, %f].", density, MaterialConstants::MinDensityLimit, MaterialConstants::MaxDensityLimit);

        m_density = AZ::GetClamp(density, MaterialConstants::MinDensityLimit, MaterialConstants::MaxDensityLimit);
    }

    float Material::GetRollingResistance() const
    {
        return m_rollingResistance;
    }

    void Material::SetRollingResistance(float rollingResistance)
    {
        AZ_Warning(
            "Box3D Material", rollingResistance >= 0.0f && rollingResistance <= 1.0f, "Rolling resistance value %f will be clamped into range [0, 1]",
            rollingResistance)

        m_rollingResistance = AZ::GetClamp(rollingResistance, 0.0f, 1.0f);
        
        m_box3DMaterial.rollingResistance = m_rollingResistance;
    }

    AZ::Vector3 Material::GetTangentVelocity() const
    {
        return m_tangentVelocity;
    }

    void Material::SetTangentVelocity(const AZ::Vector3& tangentVelocity)
    {
        if (!m_tangentVelocity.IsClose(tangentVelocity))
        {
            m_tangentVelocity = tangentVelocity;
            
            m_box3DMaterial.tangentVelocity = Box3DMathConvert(m_tangentVelocity);
        }
    }

    CombineMode Material::GetFrictionCombineMode() const
    {
        return m_frictionCombineMode;
    }

    void Material::SetFrictionCombineMode(CombineMode mode)
    {
        m_frictionCombineMode = mode;
    }

    CombineMode Material::GetRestitutionCombineMode() const
    {
        return m_restitutionCombineMode;
    }

    void Material::SetRestitutionCombineMode(CombineMode mode)
    {
        m_restitutionCombineMode = mode;
    }

    const AZ::Color& Material::GetDebugColor() const
    {
        return m_debugColor;
    }

    void Material::SetDebugColor(const AZ::Color& debugColor)
    {
        m_debugColor = debugColor;
    }

    b3SurfaceMaterial Material::GetNativeMaterial() const
    {
        return m_box3DMaterial;
    }

    void Material::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        m_materialAsset = asset;

        MaterialConfiguration::ValidateMaterialAsset(m_materialAsset);

        for (const auto& materialProperty : m_materialAsset->GetMaterialProperties())
        {
            SetProperty(materialProperty.first, materialProperty.second);
        }
    }

    void Material::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        OnAssetReady(asset);
    }
}
