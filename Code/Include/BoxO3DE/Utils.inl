
namespace B3
{
    inline BodyData* Utils::GetUserData(const b3BodyId bodyId)
    {
        if (!b3Body_IsValid(bodyId) || b3Body_GetUserData(bodyId) == nullptr)
        {
            return nullptr;
        }

        BodyData* bodyData = static_cast<BodyData*>(b3Body_GetUserData(bodyId));
        if (!bodyData || !bodyData->IsValid())
        {
            AZ_Warning("Box3D::Utils::GetUserData", false, "The body data does not look valid and is not safe to use");
            return nullptr;
        }

        return bodyData;
    }

    inline Physics::Shape* Utils::GetUserData(const b3ShapeId shapeId)
    {
        return (B3_IS_NULL(shapeId)) ? nullptr : static_cast<Physics::Shape*>(b3Shape_GetUserData(shapeId));
    }
}
