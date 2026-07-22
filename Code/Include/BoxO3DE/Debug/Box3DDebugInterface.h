
#pragma once

#include <AzCore/EBus/Event.h>
#include <BoxO3DE/Debug/Box3DDebugConfiguration.h>

namespace B3
{
    namespace Debug
    {
        //! Event to notify changes to the Collider proximity visulization data.
        //! When triggered the event will send the newly applied Collider Proximity Visualization data.
        using ColliderProximityVisualizationChangedEvent = AZ::Event<const ColliderProximityVisualization&>;

        //! Event to notify changes to the PhysX Visual Debugger (PVD) Configuration.
        //! When triggered the event will send the newly applied PVD configuration.
        // using PvdConfigurationChangedEvent = AZ::Event<const PvdConfiguration&>;

        //! Event to notify changes to the Debug display data
        //! When triggered the event will send the newly applied Debug display data.
        using DebugDisplayDataChangedEvent = AZ::Event<const DebugDisplayData&>;

        //! Interface for interacting with the Box3D Debug options.
        class Box3DDebugInterface
        {
        public:
            AZ_TYPE_INFO(Box3DDebugInterface, "{2A186950-17B5-4BFB-B06D-BA83DB237C72}");

            Box3DDebugInterface() = default;
            virtual ~Box3DDebugInterface() = default;
            AZ_DISABLE_COPY_MOVE(Box3DDebugInterface);

            //! Initialize the system.
            //! @param config The configuration to use.
            virtual void Initialize(const DebugConfiguration& config) = 0;

            //! Update the current debug configuration
            //! @param config The configuration to use.
            virtual void UpdateDebugConfiguration(const DebugConfiguration& config) = 0;

            //! Get the current debug configuration
            virtual const DebugConfiguration& GetDebugConfiguration() const = 0;

            //! Get the Configuration options to use when connecting / interacting with the PhysX Visual Debugger (PVD)
            // virtual const PvdConfiguration& GetPhysXPvdConfiguration() const = 0;

            //! Get the debug display configuration of Box3D
            virtual const DebugDisplayData& GetDebugDisplayData() const = 0;

            //! Configure the visualization of colliders based on proximity from a camera.
            //! @param enabled to activate collider visualization by proximity or not.
            //! @param cameraPosition current position of the camera.
            //! @param radius radius around the camera position to visualize colliders.
            virtual void UpdateColliderProximityVisualization(const ColliderProximityVisualization& data) = 0;

            //! Open a connection to the PhysX Visual Debugger (PVD) using the configured settings.
            // virtual bool ConnectToPvd() = 0;

            //! Close a connection to the PhysX Visual Debugger (PVD).
            // virtual void DisconnectFromPvd() = 0;

            //! Register to receive an event when the Collider Proximity Visulization data changes.
            //! @param handler The handler to receive the event.
            void RegisterColliderProximityVisualizationChangedEvent(ColliderProximityVisualizationChangedEvent::Handler& handler) { handler.Connect(m_colliderProximityVisualizationChangedEvent); }

            //! Register to receive an event when the PhysX Visual Debugger (PVD) configuration changes.
            //! @param handler The handler to receive the event.
            // void RegisterPvdConfigurationChangedEvent(PvdConfigurationChangedEvent::Handler& handler) { handler.Connect(m_pvdConfigurationChangedEvent); }

            //! Register to receive an event when the debug display data changes.
            //! @param handler The handler to receive the event.
            void RegisterDebugDisplayDataChangedEvent(DebugDisplayDataChangedEvent::Handler& handler) { handler.Connect(m_debugDisplayDataChangedEvent); }

        protected:
            ColliderProximityVisualizationChangedEvent m_colliderProximityVisualizationChangedEvent;
            // PvdConfigurationChangedEvent m_pvdConfigurationChangedEvent;
            DebugDisplayDataChangedEvent m_debugDisplayDataChangedEvent;
        };
    } // namespace Debug
} // namespace B3
