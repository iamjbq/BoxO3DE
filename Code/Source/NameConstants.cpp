#include <NameConstants.h>

namespace B3
{
    namespace UXNameConstants
    {
        const AZStd::string& GetBox3DDocsRoot()
        {
            static const AZStd::string val = "https://github.com/erincatto/box3d/tree/main/docs";
            return val;
        }
    } // namespace UXNameConstants
}
