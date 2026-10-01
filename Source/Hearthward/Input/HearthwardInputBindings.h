#pragma once
#include "CoreMinimal.h"
#include "InputCoreTypes.h"
struct FHearthwardKeyBinding
{
    FKey Key,Modifier;
    bool operator==(const FHearthwardKeyBinding& Other) const { return Key==Other.Key && Modifier==Other.Modifier; }
    FString Encode() const;
    FString Label() const;
    static FHearthwardKeyBinding Decode(const FString& Text);
    bool Matches(FKey Pressed,bool Shift,bool Control,bool Alt) const;
    bool Held(const class APlayerController* Player) const;
};
struct FHearthwardInputDefinition
{
    FName Id;
    FString Label;
    TArray<FName> Contexts;
    FName SharedPriority;
    FHearthwardKeyBinding Primary,Secondary;
    bool Essential=false;
};
using FHearthwardBindings=TMap<FName,TArray<FHearthwardKeyBinding>>;
namespace HearthwardInput
{
    HEARTHWARD_API const TArray<FHearthwardInputDefinition>& Definitions();
    HEARTHWARD_API FHearthwardBindings Defaults();
    HEARTHWARD_API FHearthwardBindings Load();
    HEARTHWARD_API void Save(const FHearthwardBindings& Bindings);
    HEARTHWARD_API FString Validate(const FHearthwardBindings& Bindings);
    HEARTHWARD_API bool Matches(const FHearthwardBindings& Bindings,FName Id,FKey Key,bool Shift,bool Control,bool Alt);
    HEARTHWARD_API bool Held(const FHearthwardBindings& Bindings,FName Id,const APlayerController* Player);
    HEARTHWARD_API FString Label(const FHearthwardBindings& Bindings,FName Id);
}
