#pragma once

#include "Engine/Level.h"
#include "Engine/World.h"

namespace HearthwardActorQuery
{
// Preserve TActorIterator's OnlyActiveLevels semantics when querying components.
inline bool IsActiveLevel(const ULevel* Level)
{
    if(!Level)return false;
    const bool Visible=(Level->bIsVisible && !Level->bIsBeingRemoved)
        || Level->bIsAssociatingLevel || Level->bIsDisassociatingLevel;
    const FLevelCollection* Collection=Level->GetCachedLevelCollection();
    const FLevelCollection* Active=Level->OwningWorld?Level->OwningWorld->GetActiveLevelCollection():nullptr;
    return Visible && (!Active || Collection==Active || !Collection || Collection->GetType()==ELevelCollectionType::StaticLevels);
}
}
