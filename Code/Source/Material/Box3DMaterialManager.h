
#pragma once

#include <AzFramework/Physics/Material/PhysicsMaterialManager.h>

namespace B3
{
    //! Material manager specialization for Box3D.
    class MaterialManager
        : public AZ::Interface<Physics::MaterialManager>::Registrar
    {
    public:
        AZ_RTTI(B3::MaterialManager, "{FAB99FA3-7803-418D-AAA5-A19F786D96A6}", Physics::MaterialManager)

        MaterialManager() = default;

    protected:
        AZStd::shared_ptr<Physics::Material> CreateDefaultMaterialInternal() override;
        AZStd::shared_ptr<Physics::Material> CreateMaterialInternal(const Physics::MaterialId& id, const AZ::Data::Asset<Physics::MaterialAsset>& materialAsset) override;
        
    };
}
