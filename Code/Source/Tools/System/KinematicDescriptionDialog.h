
#pragma once

#include <QDialog>

class QTreeView;

namespace Ui
{
    class KinematicDescriptionDialog;
}

namespace B3
{
    namespace Editor
    {
        /// Dialog for explaining the difference between Dynamic and Kinematic bodies.
        class KinematicDescriptionDialog : public QDialog
        {
            Q_OBJECT

        public:
            explicit KinematicDescriptionDialog(bool kinematicSetting, QWidget* parent = nullptr);
            ~KinematicDescriptionDialog();

            bool GetResult() const;

        private:
            void OnButtonClicked();
            void InitializeButtons();
            void UpdateDialogText();

            QScopedPointer<Ui::KinematicDescriptionDialog> m_ui;
            bool m_kinematicSetting = false;
        };
    } // namespace Editor
} // namespace B3

