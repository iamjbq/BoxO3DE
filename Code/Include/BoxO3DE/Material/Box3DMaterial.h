
#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/smart_ptr/enable_shared_from_this.h>
#include <AzCore/Math/Color.h>

#include <AzFramework/Physics/Material/PhysicsMaterial.h>
#include <AzFramework/Physics/Material/PhysicsMaterialAsset.h>

#include <box3d/box3d.h>

namespace Physics
{
    class MaterialSlots;
};

namespace B3
{
    class MaterialManager;
    
    //! Enumeration that determines how two materials properties are combined when
    //! processing collisions.
    enum class CombineMode : AZ::u8
    {
        Average,
        Minimum,
        Maximum,
        Multiply,

        ENUM_COUNT
    };

    namespace MaterialConstants
    {
        inline constexpr AZStd::string_view MaterialAssetType = "Box3D";
        inline constexpr AZ::u32 MaterialAssetVersion = 1;

        inline constexpr AZStd::string_view FrictionName = "Friction";
        inline constexpr AZStd::string_view RestitutionName = "Restitution";
        // inline constexpr AZStd::string_view DensityName = "Density";
        inline constexpr AZStd::string_view RollingResistanceName = "RollingResistance";
        inline constexpr AZStd::string_view TangentVelocityName = "TangentVelocity";
        inline constexpr AZStd::string_view RestitutionCombineModeName = "RestitutionCombineMode";
        inline constexpr AZStd::string_view FrictionCombineModeName = "FrictionCombineMode";
        inline constexpr AZStd::string_view DebugColorName = "DebugColor";

        // inline constexpr float MinDensityLimit = 0.01f; //!< Minimum possible value of density.
        // inline constexpr float MaxDensityLimit = 100000.0f; //!< Maximum possible value of density.
    }
    
    //! Runtime Box3D material instance.
    //! It handles the reloading of its data if the material asset it
    //! was created from is modified.
    //! It also provides functions to create Box3D materials.
    class Material
        : public Physics::Material
        , public AZStd::enable_shared_from_this<Material>
        , protected AZ::Data::AssetBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(Material, AZ::SystemAllocator)
        AZ_RTTI(B3::Material, "{9B4088E2-22EB-4CD4-A25E-618BE947B99C}", Physics::Material)

        //! Function to create a material instance from an asset.
        //! The material id will be constructed from the asset id.
        //! If the material id is found in the manager it returns the existing material instance.
        //! @param materialAsset Material asset to create the material instance from.
        //! @return Material instance created or found. It can return nullptr if the creation failed or if the asset passed is invalid.
        static AZStd::shared_ptr<Material> FindOrCreateMaterial(const AZ::Data::Asset<Physics::MaterialAsset>& materialAsset);

        //! Function to create material instances from material slots.
        //! The material ids will be constructed from the asset ids of the assets assigned to the slots.
        //! It will always return a valid list of materials, the slots with invalid or no assets will have
        //! the default material instance.
        //! @param materialSlots Material slots with the list of material assets to create the material instances from.
        //! @return List of material instances created. It will always return a valid list.
        static AZStd::vector<AZStd::shared_ptr<Material>> FindOrCreateMaterials(const Physics::MaterialSlots& materialSlots);

        //! Function to create a material instance from an asset.
        //! A random material will be used. This function is useful to create several instances from the same asset.
        //! @param materialAsset Material asset to create the material instance from.
        //! @return Material instance created. It can return nullptr if the creation failed or if the asset passed is invalid.
        static AZStd::shared_ptr<Material> CreateMaterialWithRandomId(const AZ::Data::Asset<Physics::MaterialAsset>& materialAsset);

        ~Material() override;

        // Physics::Material overrides ...
        Physics::MaterialPropertyValue GetProperty(AZStd::string_view propertyName) const override;
        void SetProperty(AZStd::string_view propertyName, Physics::MaterialPropertyValue value) override;

        float GetFriction() const;
        void SetFriction(float friction);

        float GetRestitution() const;
        void SetRestitution(float restitution);
        
        float GetRollingResistance() const;
        void SetRollingResistance(float rollingResistance);
        
        AZ::Vector3 GetTangentVelocity() const;
        void SetTangentVelocity(const AZ::Vector3& tangentVelocity);
        
        CombineMode GetFrictionCombineMode() const;
        void SetFrictionCombineMode(CombineMode mode);
        
        CombineMode GetRestitutionCombineMode() const;
        void SetRestitutionCombineMode(CombineMode mode);

        const AZ::Color& GetDebugColor() const;
        void SetDebugColor(const AZ::Color& debugColor);

        b3SurfaceMaterial GetNativeMaterial() const;

    protected:
        // AssetBus overrides...
        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

    private:
        friend class MaterialManager;

        Material(
            const Physics::MaterialId& id,
            const AZ::Data::Asset<Physics::MaterialAsset>& materialAsset);
        
        using Box3DMaterialUniquePtr = AZStd::unique_ptr<b3SurfaceMaterial, AZStd::function<void(b3SurfaceMaterial*)>>;
        Box3DMaterialUniquePtr m_box3DMaterialPtr;
        
        b3SurfaceMaterial m_box3DMaterial;
        
        float m_friction = 0.5;
        float m_restitution = 0.5f;
        float m_rollingResistance = 0.0f; //!< Only used for spheres and capsules
        AZ::Vector3 m_tangentVelocity = AZ::Vector3::CreateZero();
        AZ::Color m_debugColor = AZ::Colors::LightBlue;

        CombineMode m_frictionCombineMode = CombineMode::Average;
        CombineMode m_restitutionCombineMode = CombineMode::Maximum;
    };
}
