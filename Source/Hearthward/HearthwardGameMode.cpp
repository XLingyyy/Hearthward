#include "HearthwardGameMode.h"
#include "HearthwardCharacter.h"

AHearthwardGameMode::AHearthwardGameMode()
{
    DefaultPawnClass = AHearthwardCharacter::StaticClass();
}
