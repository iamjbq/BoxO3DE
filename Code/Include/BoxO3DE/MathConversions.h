
#pragma once

#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Matrix3x3.h>

#include <box3d/box3d.h>

AZ_FORCE_INLINE b3Vec3 Box3DMathConvert(const AZ::Vector3& inVec)
{
    return { inVec.GetX(), inVec.GetY(), inVec.GetZ() };
}

AZ_FORCE_INLINE AZ::Vector3 Box3DMathConvert(const b3Vec3& inVec)
{
    return AZ::Vector3(inVec.x, inVec.y, inVec.z);
}

AZ_FORCE_INLINE b3Quat Box3DMathConvert(const AZ::Quaternion& inQuat)
{
    const b3Vec3 v = { inQuat.GetX(), inQuat.GetY(), inQuat.GetZ() };
    return { v, inQuat.GetW() };
}

AZ_FORCE_INLINE AZ::Quaternion Box3DMathConvert(const b3Quat& inQuat)
{
    return AZ::Quaternion(inQuat.v.x, inQuat.v.y, inQuat.v.z, inQuat.s);
}

AZ_FORCE_INLINE AZ::Matrix3x3 Box3DMathConvert(const b3Matrix3& inMat)
{
    return AZ::Matrix3x3::CreateFromRows(
        Box3DMathConvert(inMat.cx),
        Box3DMathConvert(inMat.cy),
        Box3DMathConvert(inMat.cz)
        );
}

AZ_FORCE_INLINE b3Matrix3 Box3DMathConvert(const AZ::Matrix3x3& inMat)
{
    b3Matrix3 outMat;
    outMat.cx = Box3DMathConvert(inMat.GetBasisX());
    outMat.cy = Box3DMathConvert(inMat.GetBasisY());
    outMat.cz = Box3DMathConvert(inMat.GetBasisZ());
    
    return outMat;
}

//! Convert Transform to a rotated and translated Jolt Matrix
// AZ_FORCE_INLINE b3Matrix3 Box3DMathConvert(const AZ::Transform& inTransform)
// {
//     return b3Tr
// }

// Why is there only Jolt->AZ direction for this one?
// AZ_FORCE_INLINE AZ::Aabb Box3DMathConvert(const JPH::AABox& bounds)
// {
//     // check that the Jolt bounds are valid, otherwise CreateFromMinMax will assert.
//     if (bounds.IsValid())
//     {
//         return AZ::Aabb::CreateFromMinMax(Box3DMathConvert(bounds.mMin), Box3DMathConvert(bounds.mMax));
//     }
//     return AZ::Aabb::CreateNull();
// }
//
// AZ_FORCE_INLINE AZ::Transform Box3DMathConvert(const JPH::Vec3& position, const JPH::Quat& rotation)
// {
//     return AZ::Transform::CreateFromQuaternionAndTranslation(Box3DMathConvert(rotation), Box3DMathConvert(position));
// }
