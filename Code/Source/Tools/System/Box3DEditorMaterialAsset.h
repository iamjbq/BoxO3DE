
#pragma once

#include <BoxO3DE/Material/Box3DMaterialConfiguration.h>

namespace B3
{
    //! EditorMaterialAsset defines a single Box3D material asset.
    //! This is an editor asset, and it's authored by Asset Editor.
    //! When this asset is processed by Asset Processor it creates
    //! a generic Physics material asset in the cache (agnostic to backend).
    class EditorMaterialAsset
        : public AZ::Data::AssetData
    {
    public:
        AZ_CLASS_ALLOCATOR(B3::EditorMaterialAsset, AZ::SystemAllocator);
        AZ_RTTI(B3::EditorMaterialAsset, "{956B6C56-20CC-4D42-979E-DF51A0C1A4E6}", AZ::Data::AssetData);

        static void Reflect(AZ::ReflectContext* context);

        static constexpr const char* FileExtension = "box3dmaterial";

        EditorMaterialAsset() = default;
        virtual ~EditorMaterialAsset() = default;

        const MaterialConfiguration& GetMaterialConfiguration() const;

    protected:
        MaterialConfiguration m_materialConfiguration;
    };
} // B3
