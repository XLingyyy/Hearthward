#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "HearthwardCharacter.generated.h"

class UInputAction;
class UInputMappingContext;
class UHearthwardTimedActionComponent;
struct FInputActionValue;

UCLASS()
class HEARTHWARD_API AHearthwardCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AHearthwardCharacter();
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    UPROPERTY(VisibleAnywhere, Category="Hearthward|Actions")
    TObjectPtr<UHearthwardTimedActionComponent> TimedAction;

    void Move(const FInputActionValue& Value);
    void Look(const FInputActionValue& Value);

    UPROPERTY()
    TObjectPtr<UInputAction> MoveAction;

    UPROPERTY()
    TObjectPtr<UInputAction> LookAction;

    UPROPERTY()
    TObjectPtr<UInputMappingContext> InputMapping;
};
