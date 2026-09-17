#pragma once

#include "Modules/ModuleManager.h"

class FA3GamePlayableModule final : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};
