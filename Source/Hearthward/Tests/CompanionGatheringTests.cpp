#include "../Save/HearthwardSaveGame.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompanionGatheringSaveInspection,"Hearthward.Companion078.InspectSavedGathering",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCompanionGatheringSaveInspection::RunTest(const FString&)
{
    FString Path;
    if(!FParse::Value(FCommandLine::Get(),TEXT("HearthwardInspectGatherSave="),Path))
    {AddInfo(TEXT("No optional local save supplied; inspection not run."));return true;}
    UHearthwardSaveGame* Pool=nullptr;FString Error;
    if(!TestTrue(TEXT("Read diagnostic copy without writing original"),HearthwardSave::Read(Path,Pool,Error)))
    {AddError(Error);return false;}
    for(const auto& Point:Pool->Points)
    {
        const auto& S=Point.World;
        AddInfo(FString::Printf(TEXT("GATHER_SAVE created=%s item=%s requested=%d delivered=%d acquired=%d carried=%d phase=%s reason=%s sourceWood=%d intent=%s sourceRef=%s"),
            *Point.Created.ToIso8601(),*S.Item.ToString(),S.Requested,S.Delivered,S.Acquired,S.Carried,
            *UEnum::GetValueAsString(S.Phase),*S.BlockReason,S.Resource.FindRef(TEXT("wood")),*S.AgentGoal.Intent.ToString(),*S.AgentGoal.SourceRef));
    }
    return true;
}
#endif
