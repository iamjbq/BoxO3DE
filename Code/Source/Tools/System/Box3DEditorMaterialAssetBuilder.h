
#pragma once

#include <AssetBuilderSDK/AssetBuilderBusses.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>

#include <BoxO3DE/Material/Box3DMaterialConfiguration.h>

namespace B3
{
    class EditorMaterialAsset;

    //! Builder to convert Box3D Editor Material assets in the
    //! source folder into Physics Material assets in the cache folder.
    class EditorMaterialAssetBuilder
        : public AssetBuilderSDK::AssetBuilderCommandBus::Handler
    {
    public:
        AZ_RTTI(B3::EditorMaterialAssetBuilder, "{7A79BEE0-0526-4DB8-8645-C48B7C83F079}");

        EditorMaterialAssetBuilder() = default;

        // Asset Builder Callback Functions...
        void CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response) const;
        void ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response) const;

        // AssetBuilderSDK::AssetBuilderCommandBus overrides...
        void ShutDown() override { }

    private:
        AZ::Data::Asset<EditorMaterialAsset> LoadEditorMaterialAsset(const AZStd::string& assetFullPath) const;

        bool SerializeOutPhysicsMaterialAsset(
            AZ::Data::Asset<Physics::MaterialAsset> physicsMaterialAsset,
            const AssetBuilderSDK::ProcessJobRequest& request,
            AssetBuilderSDK::ProcessJobResponse& response) const;
    };
} // B3
