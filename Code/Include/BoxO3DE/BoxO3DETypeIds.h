
#pragma once

namespace B3
{
    // System Component TypeIds
    inline constexpr const char* BoxO3DESystemComponentTypeId = "{C3DEFBBA-272F-40E2-B5B0-16C6771CAC8E}";
    inline constexpr const char* BoxO3DEEditorSystemComponentTypeId = "{FA177023-20C6-4F27-9619-1E2E6781E019}";

    // Module derived classes TypeIds
    inline constexpr const char* BoxO3DEModuleInterfaceTypeId = "{F1EC15D2-06F1-4226-9381-561F2C7792C0}";
    inline constexpr const char* BoxO3DEModuleTypeId = "{10536007-73D7-451A-A499-C51A15280F2E}";
    // The Editor Module by default is mutually exclusive with the Client Module
    // so they use the Same TypeId
    inline constexpr const char* BoxO3DEEditorModuleTypeId = BoxO3DEModuleTypeId;

    // Interface TypeIds
    inline constexpr const char* BoxO3DERequestsTypeId = "{A45D38C2-7C2A-46E9-AB6B-E3275B32C2B6}";
} // namespace B3
