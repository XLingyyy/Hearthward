#include "HearthwardInventoryComponent.h"
#include "Engine/Engine.h"
#include "GameFramework/Pawn.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"

#if !UE_BUILD_SHIPPING
namespace
{
UHearthwardInventoryComponent* FindInventory(UWorld* World)
{
    APawn* Pawn = World ? UGameplayStatics::GetPlayerPawn(World, 0) : nullptr;
    return Pawn ? Pawn->FindComponentByClass<UHearthwardInventoryComponent>() : nullptr;
}

void ShowInventory(UWorld* World)
{
    const auto* Inventory = FindInventory(World);
    if (!Inventory) return;
    FString Message = FString::Printf(TEXT("Inventory %.2f / %.2f"), Inventory->GetWeight(), Inventory->GetCapacity());
    for (const auto& Item : HearthwardBasicItems())
        Message += FString::Printf(TEXT(" | %s %d"), *Item.Id.ToString(), Inventory->GetItemCount(Item.Id));
    UE_LOG(LogTemp, Display, TEXT("Hearthward.Inventory: %s"), *Message);
    if (GEngine) GEngine->AddOnScreenDebugMessage(5008, 8.0f, FColor::Cyan, Message);
}

void ChangeInventory(const TArray<FString>& Args, UWorld* World, bool Add)
{
    auto* Inventory = FindInventory(World);
    if (!Inventory) return;
    int64 Count = 0;
    bool Valid = Args.Num() == 2 && !Args[1].IsEmpty();
    if (Valid)
    {
        for (TCHAR Digit : Args[1])
        {
            if (Digit < TEXT('0') || Digit > TEXT('9')) { Valid = false; break; }
            Count = Count * 10 + (Digit - TEXT('0'));
            if (Count > MAX_int32) { Valid = false; break; }
        }
    }
    if (!Valid || Count == 0)
    {
        UE_LOG(LogTemp, Display, TEXT("Hearthward.Inventory: expected <wood|stone|ore|meat|arrow> <positive int32 count>"));
        return;
    }
    const FName Item(*Args[0]);
    const auto Result = Add ? Inventory->TryAdd(Item, static_cast<int32>(Count)) : Inventory->TryRemove(Item, static_cast<int32>(Count));
    const FString Message = UEnum::GetValueAsString(Result);
    UE_LOG(LogTemp, Display, TEXT("Hearthward.Inventory result: %s"), *Message);
    if (GEngine) GEngine->AddOnScreenDebugMessage(5009, 8.0f, FColor::Cyan, Message);
    ShowInventory(World);
}

FAutoConsoleCommandWithWorld QueryCommand(TEXT("Hearthward.Inventory"), TEXT("Show local inventory."),
    FConsoleCommandWithWorldDelegate::CreateStatic(&ShowInventory));
FAutoConsoleCommandWithWorldAndArgs AddCommand(TEXT("Hearthward.Inventory.Add"), TEXT("Development grant: <item> <count>."),
    FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) { ChangeInventory(Args, World, true); }));
FAutoConsoleCommandWithWorldAndArgs RemoveCommand(TEXT("Hearthward.Inventory.Remove"), TEXT("Development removal: <item> <count>."),
    FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) { ChangeInventory(Args, World, false); }));
}
#endif
