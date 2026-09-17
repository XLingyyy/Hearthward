#include "HearthwardGameMode.h"
#include "HearthwardCharacter.h"
#include "UI/HearthwardHUD.h"

AHearthwardGameMode::AHearthwardGameMode()
{
    DefaultPawnClass = AHearthwardCharacter::StaticClass();
    HUDClass = AHearthwardHUD::StaticClass();
}
