#include "../AI/HearthwardLocalAISubsystem.h"
#include "../Companion/HearthwardCompanionFixture.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"

#if WITH_DEV_AUTOMATION_TESTS
struct FLocalAIReliabilityAccess
{
    static void Seed(UHearthwardLocalAISubsystem* AI,AActor* Speaker,AHearthwardCompanionFixture* Brother)
    {
        AI->PendingSpeaker=Speaker;AI->PendingCompanion=Brother;
        AI->Ticket=Brother->Request(Speaker,TEXT("原始委托"));
        AI->Input=TEXT("原始委托");AI->bPending=true;AI->Serial=41;
    }
    static uint64 Serial(const UHearthwardLocalAISubsystem* AI){return AI->Serial;}
    static FHearthwardCommandTicket Ticket(const UHearthwardLocalAISubsystem* AI){return AI->Ticket;}
    static void Fail(UHearthwardLocalAISubsystem* AI){AI->Fail(TEXT("诊断模型失败"));}
};
namespace
{
struct FLocalAIReliabilityWorld
{
    UWorld* World;
    AActor* Speaker;
    AHearthwardCompanionFixture* Brother;
    UHearthwardLocalAISubsystem* AI;
    FLocalAIReliabilityWorld()
    {
        UWorld::InitializationValues Values;Values.AllowAudioPlayback(false).RequiresHitProxies(false);
        World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Values);
        GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
        Speaker=World->SpawnActor<AActor>();Brother=World->SpawnActor<AHearthwardCompanionFixture>();
        Brother->InitializeCompanion(Brother->Bag,Speaker);
        AI=World->GetSubsystem<UHearthwardLocalAISubsystem>();
    }
    ~FLocalAIReliabilityWorld(){GEngine->DestroyWorldContext(World);World->DestroyWorld(false);}
};
struct FMissingAIBundle
{
    FString Original=FCommandLine::Get();
    FMissingAIBundle(){FCommandLine::Set(TEXT("HearthwardAIBundlePath=__Task087_Missing_Bundle__"));}
    ~FMissingAIBundle(){FCommandLine::Set(*Original);}
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLocalAIBusyRequestTest,"Hearthward.Iteration.Task087.BusyKeepsRequest",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLocalAIBusyRequestTest::RunTest(const FString&)
{
    FLocalAIReliabilityWorld F;FLocalAIReliabilityAccess::Seed(F.AI,F.Speaker,F.Brother);
    const auto Before=FLocalAIReliabilityAccess::Ticket(F.AI);const uint64 Serial=FLocalAIReliabilityAccess::Serial(F.AI);
    // The baseline must not start a real model when its duplicate-submission bug is reproduced.
    FMissingAIBundle Missing;
    TestFalse(TEXT("duplicate free text is rejected while busy"),F.AI->SubmitPlayerText(F.Speaker,F.Brother,TEXT("第二条委托")));
    TestTrue(TEXT("original request remains pending"),F.AI->IsBusy());
    TestEqual(TEXT("original input remains unchanged"),F.AI->GetLastInput(),FString(TEXT("原始委托")));
    TestEqual(TEXT("duplicate did not invalidate serial"),FLocalAIReliabilityAccess::Serial(F.AI),Serial);
    TestTrue(TEXT("original ticket remains current"),F.Brother->IsProposalCurrent(F.Speaker,Before));
    TestEqual(TEXT("busy state has a distinct reason"),F.AI->GetReasonCode(),FString(TEXT("REQUEST_BUSY")));
    TestTrue(TEXT("busy feedback explains cancel or wait"),F.AI->GetStatus().Contains(TEXT("取消")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLocalAIFailureInvalidationTest,"Hearthward.Iteration.Task087.FailureInvalidatesCallback",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLocalAIFailureInvalidationTest::RunTest(const FString&)
{
    FLocalAIReliabilityWorld F;FLocalAIReliabilityAccess::Seed(F.AI,F.Speaker,F.Brother);
    const auto Before=FLocalAIReliabilityAccess::Ticket(F.AI);const uint64 Serial=FLocalAIReliabilityAccess::Serial(F.AI);
    AddExpectedErrorPlain(TEXT("Local AI failure [MODEL_UNAVAILABLE]: 诊断模型失败"),EAutomationExpectedErrorFlags::Contains,1);
    FLocalAIReliabilityAccess::Fail(F.AI);
    TestTrue(TEXT("failure invalidates the captured callback serial"),FLocalAIReliabilityAccess::Serial(F.AI)!=Serial);
    TestFalse(TEXT("failure releases busy state"),F.AI->IsBusy());
    TestFalse(TEXT("failure discards only pending proposal"),F.Brother->IsProposalCurrent(F.Speaker,Before));
    TestEqual(TEXT("root failure reason is preserved"),F.AI->GetStatus(),FString(TEXT("诊断模型失败")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLocalAICancellationIsolationTest,"Hearthward.Iteration.Task087.CancelKeepsManualFallback",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLocalAICancellationIsolationTest::RunTest(const FString&)
{
    FLocalAIReliabilityWorld F;FLocalAIReliabilityAccess::Seed(F.AI,F.Speaker,F.Brother);
    const auto Ticket=FLocalAIReliabilityAccess::Ticket(F.AI);const auto Epoch=F.World->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch();
    const uint64 Serial=FLocalAIReliabilityAccess::Serial(F.AI);
    F.AI->CancelPending();
    TestFalse(TEXT("cancelled inference leaves no busy state or card"),F.AI->IsBusy() || F.AI->HasCandidate());
    TestTrue(TEXT("cancel invalidates captured serial"),FLocalAIReliabilityAccess::Serial(F.AI)!=Serial);
    TestFalse(TEXT("cancelled inference ticket cannot execute"),F.Brother->IsProposalCurrent(F.Speaker,Ticket));
    TestEqual(TEXT("cancel does not advance save timeline"),F.World->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch(),Epoch);
    TestTrue(TEXT("manual query works without model"),F.AI->QueryInventory(F.Speaker,F.Brother,TEXT("wood")));
    TestEqual(TEXT("manual query is labelled deterministic"),F.AI->GetReasonCode(),FString(TEXT("deterministic_fallback")));
    return true;
}
#endif
