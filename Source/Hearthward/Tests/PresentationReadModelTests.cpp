#include "../UI/HearthwardPresentationReadModels.h"
#include "../Camp/HearthwardCampSubsystem.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
FHearthwardCompanionCommand PartialCommand(FGuid Epoch)
{
    FHearthwardCompanionCommand C;const auto Ticket=C.Request(Epoch);
    C.Accept(Ticket,Epoch,TEXT("wood"),32,{TEXT("collect"),TEXT("return"),TEXT("deposit")});
    C.RecordAcquisition(20);C.Carried-=14;C.RecordDelivery(Ticket,14);return C;
}
struct FPresentationWorld
{
    UWorld* World;
    FPresentationWorld()
    {
        UWorld::InitializationValues Values;Values.AllowAudioPlayback(false).RequiresHitProxies(false);
        World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Values);
        GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    }
    ~FPresentationWorld(){GEngine->DestroyWorldContext(World);World->DestroyWorld(false);}
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPresentationPersonalReadTest,"Hearthward.Iteration.Task085.PersonalReadOnly",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FPresentationPersonalReadTest::RunTest(const FString&)
{
    using P=EHearthwardCompanionPhase;
    const auto Epoch=FGuid::NewGuid();auto Command=PartialCommand(Epoch);const auto Ticket=Command.GetActive();
    const auto View=HearthwardPresentation::ProjectCompanion(Command,P::WaitingAtCamp,Epoch,TEXT("实际资源不足"));
    TestTrue(TEXT("actual task available"),View.Available && View.HasTask);
    TestEqual(TEXT("delivered is actual delivery"),View.Delivered.GetValue(),14);
    TestEqual(TEXT("carried remains separate"),View.Carried.GetValue(),6);
    TestEqual(TEXT("acquired remains its own counter"),View.Acquired.GetValue(),20);
    TestEqual(TEXT("remaining is undelivered rather than unacquired"),View.RemainingToDeliver.GetValue(),18);
    TestEqual(TEXT("blocking reason preserved"),View.BlockReason,FString(TEXT("实际资源不足")));
    TestTrue(TEXT("blocked phase offers existing operations"),View.CanResume && View.CanCancel && !View.Terminal);
    TestEqual(TEXT("read did not credit delivery"),Command.GetDelivered(),14);
    TestEqual(TEXT("read did not consume cargo"),Command.GetCarried(),6);
    TestTrue(TEXT("read did not replace or cancel command"),Command.IsCurrent(Epoch) && Command.GetActive().Matches(Ticket));

    FHearthwardCompanionCommand Returning;const auto ReturnTicket=Returning.Request(Epoch);
    Returning.Accept(ReturnTicket,Epoch,TEXT("wood"),5,{TEXT("collect"),TEXT("return"),TEXT("deposit")});
    Returning.RecordAcquisition(5);
    const auto Carrying=HearthwardPresentation::ProjectCompanion(Returning,P::Returning,Epoch);
    TestFalse(TEXT("full carried target never completes before delivery"),Carrying.Terminal);
    TestEqual(TEXT("all five remain undelivered"),Carrying.RemainingToDeliver.GetValue(),5);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPresentationLifecycleTest,"Hearthward.Iteration.Task085.TimelineAndOperations",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FPresentationLifecycleTest::RunTest(const FString&)
{
    using P=EHearthwardCompanionPhase;
    const auto Epoch=FGuid::NewGuid();auto Command=PartialCommand(Epoch);
    const auto Before=HearthwardPresentation::ProjectCompanion(Command,P::WaitingAtCamp,Epoch);
    const auto Pending=Command.Request(Epoch);Command.DiscardPending(Pending);
    const auto Discarded=HearthwardPresentation::ProjectCompanion(Command,P::WaitingAtCamp,Epoch);
    TestTrue(TEXT("discarding a proposal retains current command identity"),Discarded.Matches(Epoch,Before.CommandId));
    TestEqual(TEXT("discarding a proposal retains progress"),Discarded.Delivered.GetValue(),Before.Delivered.GetValue());
    TestEqual(TEXT("discarding a proposal retains cargo"),Discarded.Carried.GetValue(),Before.Carried.GetValue());
    const auto OtherEpoch=FGuid::NewGuid();
    TestFalse(TEXT("old view cannot authorize new timeline"),Before.Matches(OtherEpoch,Before.CommandId));
    TestFalse(TEXT("old view cannot authorize replacement command"),Before.Matches(Epoch,FGuid::NewGuid()));
    const auto Stale=HearthwardPresentation::ProjectCompanion(Command,P::WaitingAtCamp,OtherEpoch);
    TestFalse(TEXT("old command is unavailable in a different timeline"),Stale.Available);
    TestFalse(TEXT("stale count is unknown rather than zero"),Stale.Delivered.IsSet());
    Command.Cancel();const auto Cancelled=HearthwardPresentation::ProjectCompanion(Command,P::Cancelled,Epoch);
    TestTrue(TEXT("cancelled record remains an explicit terminal record"),Cancelled.HasTask && Cancelled.Terminal);
    TestFalse(TEXT("cancelled record offers no execution actions"),Cancelled.CanCancel || Cancelled.CanResume);
    TestEqual(TEXT("cancel does not delete physical cargo count"),Cancelled.Carried.GetValue(),6);
    TestEqual(TEXT("cancelled phase has distinct label"),HearthwardPresentation::CompanionPhaseText(P::Cancelled),FString(TEXT("委托已取消")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPresentationMissingCountsTest,"Hearthward.Iteration.Task085.MissingAndInvalidCounts",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FPresentationMissingCountsTest::RunTest(const FString&)
{
    const auto Epoch=FGuid::NewGuid();FHearthwardCompanionCommand Empty;
    const auto Missing=HearthwardPresentation::ProjectCompanion(Empty,EHearthwardCompanionPhase::Idle,Epoch);
    TestTrue(TEXT("no task is a known idle state"),Missing.Available && !Missing.HasTask);
    TestFalse(TEXT("no task has no fabricated quantity"),Missing.Requested.IsSet() || Missing.RemainingToDeliver.IsSet());
    auto Command=PartialCommand(Epoch);Command.Acquired=-1;
    const auto Invalid=HearthwardPresentation::ProjectCompanion(Command,EHearthwardCompanionPhase::WaitingAtCamp,Epoch);
    TestFalse(TEXT("inconsistent quantities are unavailable"),Invalid.Available);
    TestFalse(TEXT("invalid quantity is not converted to a completed zero remainder"),Invalid.RemainingToDeliver.IsSet() || Invalid.Terminal);
    TestEqual(TEXT("projection never repairs authority"),Command.Acquired,-1);
    TestFalse(TEXT("missing actors do not authorize actions"),HearthwardPresentation::ReadCompanion(nullptr,nullptr).CanCancel);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPresentationWorldReadTest,"Hearthward.Iteration.Task085.WorldSources",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FPresentationWorldReadTest::RunTest(const FString&)
{
    FPresentationWorld Fixture;auto* World=Fixture.World;
    auto* Storage=World->GetSubsystem<UHearthwardStorageSubsystem>();auto* Camp=World->GetSubsystem<UHearthwardCampSubsystem>();
    Camp->State.AddCamp(TEXT("camp"),FVector::ZeroVector);
    auto* Region=Camp->State.Regions.FindByPredicate([](const auto& R){return R.Id==TEXT("camp_wood");});
    if(!TestNotNull(TEXT("existing wood region"),Region))return false;
    Region->Workers={2,5};Region->Brother=true;Region->Enabled=false;Region->Completed=7;
    const auto Before=Camp->State.Snapshot();const int32 Stock=Storage->GetItemCount(TEXT("wood"));
    const auto Team=HearthwardPresentation::ReadWorkParty(Camp,Storage,TEXT("camp"));
    TestTrue(TEXT("existing paused team is available"),Team.Available && Team.HasTeam);
    TestEqual(TEXT("actual workers copied"),Team.Workers.Num(),2);
    TestEqual(TEXT("completed value remains batches"),Team.CompletedBatches.GetValue(),7);
    TestFalse(TEXT("assigned brother is not counted as working while paused"),Team.BrotherWorking);
    TestEqual(TEXT("projection did not change production or workers"),Camp->State.Snapshot(),Before);
    TestEqual(TEXT("projection did not produce inventory"),Storage->GetItemCount(TEXT("wood")),Stock);
    auto* Brother=World->SpawnActor<AHearthwardCompanionFixture>();
    const auto Live=HearthwardPresentation::ReadCompanion(Brother,Storage);
    TestTrue(TEXT("real idle companion can be read"),Live.Available && !Live.HasTask);
    Brother->Destroy();const auto Destroyed=HearthwardPresentation::ReadCompanion(Brother,Storage);
    TestFalse(TEXT("destroying source invalidates new reads"),Destroyed.Available);
    TestFalse(TEXT("destroyed source does not yield known progress"),Destroyed.Delivered.IsSet());
    TestFalse(TEXT("destroyed source cannot execute retained view"),HearthwardPresentation::CanApplyPersonalAction(Live,Brother,Storage,nullptr,false));
    return true;
}
#endif
