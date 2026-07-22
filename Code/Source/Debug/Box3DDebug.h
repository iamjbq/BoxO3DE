
#pragma once

#include <AzCore/Interface/Interface.h>
#include <BoxO3DE/Debug/Box3DDebugInterface.h>

namespace physx
{
    class PxFoundation;
    class PxPvdTransport;
    class PxPvd;
}

namespace B3
{
    namespace Debug
    {
        //! Implementation of the Box3DDebugInterface.
        class Box3DDebug
            : public AZ::Interface<Box3DDebugInterface>::Registrar
        {
        public:
            Box3DDebug() = default;

            // physx::PxPvd* InitializePhysXPvd(physx::PxFoundation* foundation);
            // void ShutdownPhysXPvd();

            // Box3DDebugInterface ...
            void Initialize(const DebugConfiguration& config) override;
            void UpdateDebugConfiguration(const DebugConfiguration& config) override;
            const DebugConfiguration& GetDebugConfiguration() const override;
            // const PvdConfiguration& GetPhysXPvdConfiguration() const override;
            const DebugDisplayData& GetDebugDisplayData() const override;
            void UpdateColliderProximityVisualization(const ColliderProximityVisualization& data) override;

        private:
            // [[maybe_unused]] physx::PxPvdTransport* m_pvdTransport = nullptr;
            // physx::PxPvd* m_pvd = nullptr;

            DebugConfiguration m_config;
        };
    }// namespace Debug
}// namespace B3
