
#pragma once

#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Math/Color.h>

#include <BoxO3DE/Material/Box3DMaterial.h>

namespace B3
{
    //! Properties of a Box3D surface material.
    struct MaterialConfiguration
    {
        AZ_TYPE_INFO(B3::MaterialConfiguration, "{E9730F5E-C2CE-48FC-8F2A-12E54354B96A}");

        static void Reflect(AZ::ReflectContext* context);

        float m_Friction = 0.5f;
        float m_restitution = 0.5f;
        float m_density = 1000.0f;
        float m_rollingResistance = 0.0f;
        AZ::Vector3 m_tangentVelocity = AZ::Vector3::CreateZero();
        AZ::Color m_debugColor = AZ::Colors::LightBlue;

        CombineMode m_restitutionCombine = CombineMode::Average;
        CombineMode m_frictionCombine = CombineMode::Maximum;
        
        //! Creates a Physics Material Asset with random Id from the
        //! properties of material configuration.
        AZ::Data::Asset<Physics::MaterialAsset> CreateMaterialAsset() const;

        static void ValidateMaterialAsset(AZ::Data::Asset<Physics::MaterialAsset> materialAsset);
        
    private:
        static float GetMinDensityLimit();
        static float GetMaxDensityLimit();        
    };
}
