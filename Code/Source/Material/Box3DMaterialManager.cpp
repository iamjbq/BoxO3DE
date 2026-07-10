
#include <Material/Box3DMaterialManager.h>

#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/Asset/AssetManager.h>

#include <BoxO3DE/Material/Box3DMaterial.h>
#include <BoxO3DE/Material/Box3DMaterialConfiguration.h>


namespace B3
{
    AZStd::shared_ptr<Physics::Material> MaterialManager::CreateDefaultMaterialInternal()
    {
        const MaterialConfiguration defaultMaterialConfiguration;

        AZ::Data::Asset<Physics::MaterialAsset> defaultMaterialAsset =
            defaultMaterialConfiguration.CreateMaterialAsset();

        return CreateMaterialInternal(
            Physics::MaterialId::CreateFromAssetId(defaultMaterialAsset.GetId()),
            defaultMaterialAsset);
    }

    AZStd::shared_ptr<Physics::Material> MaterialManager::CreateMaterialInternal(
        const Physics::MaterialId& id,
        const AZ::Data::Asset<Physics::MaterialAsset>& materialAsset)
    {
        return AZStd::shared_ptr<Physics::Material>(aznew Material(id, materialAsset));
    }
}
