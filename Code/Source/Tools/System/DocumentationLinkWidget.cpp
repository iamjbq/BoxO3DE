#include <Tools/System/DocumentationLinkWidget.h>

namespace B3
{
    namespace Editor
    {
        DocumentationLinkWidget::DocumentationLinkWidget(const QString& format, const QString& address)
            : QLabel()
        {
            setText(format.arg(address));
            setTextInteractionFlags(Qt::TextBrowserInteraction);
            setOpenExternalLinks(true);
            setAlignment(Qt::AlignCenter);
            setContentsMargins(60, 7, 60, 7);
            setWordWrap(true);
            setStyleSheet(QString::fromUtf8("background-color: rgb(51, 51, 51);"));
        }
    }
}
