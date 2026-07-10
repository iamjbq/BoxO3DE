
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/Asset/AssetCommon.h>
#include <QWidget>
#include <QScopedPointer>
#endif

namespace AzPhysics
{
    class CollisionConfiguration;
    struct SceneConfiguration;
}

namespace Ui
{
    class EditorWindowClass;
}

// This is defined in the core engine editor, so gonna try this first
namespace LyViewPane
{
    static const char* const Box3DConfigurationEditor = "Box3D Configuration";
}

namespace B3
{
    struct Box3DSystemConfiguration;
    namespace Debug
    {
        struct DebugConfiguration;
    }

    namespace Editor
    {
        /// Window pane wrapper for the Box3D Configuration Widget.
        ///
        class EditorWindow
            : public QWidget
        {
            Q_OBJECT
        public:
            AZ_CLASS_ALLOCATOR(EditorWindow, AZ::SystemAllocator);
            static void RegisterViewClass();

            explicit EditorWindow(QWidget* parent = nullptr);
            ~EditorWindow() override;

        private:
            static void SaveConfiguration(
                const B3::Box3DSystemConfiguration& box3DSystemConfiguration,
                // const B3::Debug::DebugConfiguration& box3DDebugConfiguration,
                const AzPhysics::SceneConfiguration& defaultSceneConfiguration);

            QScopedPointer<Ui::EditorWindowClass> m_ui;
        };
    }
};
