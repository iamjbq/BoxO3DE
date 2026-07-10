
#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzFramework/Physics/Character.h>
#include <AzFramework/Physics/Ragdoll.h>
#include <AzFramework/Physics/SimulatedBodies/RigidBody.h>
#include <AzFramework/Physics/SimulatedBodies/StaticRigidBody.h>

#include <box3d/box3d.h>

namespace AzPhysics
{
    struct SimulatedBody;
}

namespace B3
{
    /// userData is the custom data pointer that Box3D provides for applications to attach
    ///private data. The Box3D Gem requires that this userData points to BodyData objects.
    class BodyData
    {
    public:
        BodyData() = default;
        BodyData(b3BodyId bodyId);
        BodyData(BodyData&& bodyData);
        BodyData& operator=(BodyData&& bodyData);

        void Invalidate();
        AZ::EntityId GetEntityId() const;
        void SetEntityId(AZ::EntityId entityId);

        AzPhysics::SimulatedBodyHandle GetBodyHandle() const;

        AzPhysics::RigidBody* GetRigidBody() const;
        void SetRigidBody(AzPhysics::RigidBody* rigidBody);

        AzPhysics::StaticRigidBody* GetRigidBodyStatic() const;
        void SetRigidBodyStatic(AzPhysics::StaticRigidBody* rigidBody);

        Physics::Character* GetCharacter() const;
        void SetCharacter(Physics::Character* character);

        Physics::RagdollNode* GetRagdollNode() const;
        void SetRagdollNode(Physics::RagdollNode* ragdollNode);

        AzPhysics::SimulatedBody* GetArticulationLink();
        void SetArticulationLink(AzPhysics::SimulatedBody* articulationLink);

        AzPhysics::SimulatedBody* GetSimulatedBody() const;

        bool IsValid() const;

    private:
        using BodyIdUniquePtr = AZStd::unique_ptr<b3BodyId, AZStd::function<void(b3BodyId*)>>;

        ///This is an arbitary value used to verify the cast from void* userdata pointer on a pxActor to ActorData
        ///is safe. If m_sanity does not have this value, then it is not safe to use the casted pointer.
        ///Helps to debug if someone is setting userData pointer to something other than this class during development
        static constexpr AZ::u32 SanityValue = 0xba5eba11;

        AZ::u32 m_sanity = SanityValue;
        BodyIdUniquePtr m_bodyIdPtr;

        struct Payload
        {
            AZ::EntityId m_entityId;
            // Possible references, only one of them is not nullptr
            AzPhysics::RigidBody* m_rigidBody = nullptr;
            AzPhysics::StaticRigidBody* m_staticRigidBody = nullptr;
            Physics::Character* m_character = nullptr;
            Physics::RagdollNode* m_ragdollNode = nullptr;
            AzPhysics::SimulatedBody* m_articulationLink = nullptr;
            void* m_externalUserData = nullptr;
        };

        Payload m_payload;
    };
} // namespace B3

#include <BoxO3DE/UserDataTypes.inl>
