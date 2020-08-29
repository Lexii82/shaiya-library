#include <shaiya/World.hpp>
#include <shaiya/utils/Hook.hpp>

#include <shaiya/models/CUser.hpp>
#include <shaiya/hooks/character/ItemSetSynergy.hpp>

/**
 * The hook point for the ItemEquipmentAddedHook
 */
constexpr const auto ItemEquipmentAddEntry  = 0x461640;

/**
 * The return address for the ItemEquipmentAddedHook
 */
constexpr const auto ItemEquipmentAddReturn = 0x461646;

/**
 * The hook point for the ItemEquipmentRemovedHook
 */
constexpr const auto ItemEquipmentRemEntry  = 0x461D10;

/**
 * The return address for the ItemEquipmentRemovedHook
 */
constexpr const auto ItemEquipmentRemReturn = 0x461D16;

/**
 * The address of the CUser::EnterWorld function
 */
constexpr const auto CUserEnterWorld        = 0x455B00;

/**
 * The function definition for the CUser::EnterWorld function
 */
typedef void (__thiscall* CUserEnterWorldFunc)(CUser*);

/**
 * The original CUser::EnterWorld function.
 */
CUserEnterWorldFunc originalCUserEnterWorld = nullptr;

/**
 * The address of the CUser::LeaveWorld function
 */
constexpr const auto CUserLeaveWorld        = 0x413E90;

/**
 * The function definition for the CUser::LeaveWorld function
 */
typedef void (__stdcall* CUserLeaveWorldFunc)(CUser*);

/**
 * The original CUser::LeaveWorld function
 */
CUserLeaveWorldFunc originalCUserLeaveWorld = nullptr;

/**
 * Gets executed when an item is added to the users equipment.
 * edi = CUser*
 */
void __declspec(naked) itemEquipmentAddedHook()
{
    __asm
    {

        // Save the current state of the registers
        pushad
        pushfd

        // Initialise a call frame
        push ebp
        mov ebp,esp
        push edi
        call ItemSetSynergy::applyWornSynergies
        add esp,0x4 // Remove the call argument from the frame
        mov esp,ebp
        pop ebp

        // Restore the state of the registers
        popfd
        popad

        // Re-write the function header that we overwrote
        sub esp,0x1C
        push ebx
        mov ebx,eax
        jmp ItemEquipmentAddReturn
    }
}

/**
 * Gets executed when an item is removed from the user's equipment.
 * edx = CUser*
 */
void __declspec(naked) itemEquipmentRemovedHook()
{
    __asm
    {
        // Save the current state of the registers
        pushad
        pushfd

        // Initialise a call frame
        push ebp
        mov ebp,esp
        push edx
        call ItemSetSynergy::applyWornSynergies
        add esp,0x4 // Remove the call argument from the frame
        mov esp,ebp
        pop ebp

        // Restore the state of the registers
        popfd
        popad

        // Re-write the function header that we overwrote
        push ebp
        mov ebp,esp
        and esp,-8
        jmp ItemEquipmentRemReturn
    }
}

/**
 * Gets executed when a user enters the game world.
 * @param user  The user.
 */
void userEnterWorld(CUser* user)
{
    ItemSetSynergy::applyWornSynergies(user);
    originalCUserEnterWorld(user);
}

/**
 * Gets executed when a user leaves the game world.
 * @param user  The user.
 */
void __stdcall userLeaveWorld(CUser* user)
{
    ItemSetSynergy::removeSynergies(user);
    originalCUserLeaveWorld(user);
}

/**
 * Initialises the character specific hooks.
 */
template <> void World::hook<HookType::Character>()
{
    // Load the item synergies
    ItemSetSynergy::parse("Data/setitem.csv");

    // Load the user enter/leave hooks
    originalCUserEnterWorld = (CUserEnterWorldFunc) memory(CUserEnterWorld, 6, (DWORD) userEnterWorld);
    originalCUserLeaveWorld = (CUserLeaveWorldFunc) memory(CUserLeaveWorld, 8, (DWORD) userLeaveWorld);

    // Load the equipment hooks
    memory(ItemEquipmentAddEntry, 6, (DWORD) itemEquipmentAddedHook);
    memory(ItemEquipmentRemEntry, 6, (DWORD) itemEquipmentRemovedHook);
}