
#pragma once

#include <QLabel>

namespace B3
{
    namespace Editor
    {
        class DocumentationLinkWidget
            : public QLabel
        {
        public:
            explicit DocumentationLinkWidget(const QString& linkFormat, const QString& linkAddress);
        };
    }
}
