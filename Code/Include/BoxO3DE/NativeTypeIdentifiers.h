
#pragma once

#include <AzCore/Math/Crc.h>

namespace B3
{
    namespace NativeTypeIdentifiers
    {
        static const AZ::Crc32 World = AZ_CRC_CE("Box3DWorld");
        static const AZ::Crc32 RigidBody = AZ_CRC_CE("Box3DRigidBody");
        static const AZ::Crc32 RigidBodyStatic = AZ_CRC_CE("Box3DRigidBodyStatic");

    } // namespace NativeTypeIdentifiers
} // namespace B3
