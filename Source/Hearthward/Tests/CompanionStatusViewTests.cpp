#include "Misc/AutomationTest.h"
#include "../UI/HearthwardPresentationReadModels.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHearthwardPersonalStatusViewTest,"Hearthward.Iteration.Task086.PersonalStatus",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FHearthwardPersonalStatusViewTest::RunTest(const FString&)
{
    FHearthwardCompanionCommand Command;const FGuid Epoch=FGuid::NewGuid();
    const auto Ticket=Command.Request(Epoch);
    if(!TestEqual(TEXT("accept quantity 32"),Command.Accept(Ticket,Epoch,TEXT("wood"),32,
        {TEXT("collect"),TEXT("return"),TEXT("deposit")}),EHearthwardProposalResult::Accepted))return false;
    Command.RecordAcquisition(20);Command.RecordDelivery(Ticket,14);Command.Carried-=14;
    auto V=HearthwardPresentation::ProjectCompanion(Command,EHearthwardCompanionPhase::Returning,Epoch);
    const FString Text=HearthwardPresentation::CompanionWorkText(V);
    TestTrue(TEXT("delivered amount is displayed separately"),Text.Contains(TEXT("已交付 14 / 32")));
    TestTrue(TEXT("carried six is not added to delivered"),Text.Contains(TEXT("携带 6")));
    TestTrue(TEXT("remaining means not yet delivered"),Text.Contains(TEXT("尚未交付 18")));
    TestTrue(TEXT("known destination only"),Text.Contains(TEXT("目的地：营地仓储")));
    TestFalse(TEXT("carrying does not create terminal state"),V.Terminal);
    TestEqual(TEXT("formatting preserves authoritative delivery"),Command.GetDelivered(),14);
    TestEqual(TEXT("formatting preserves authoritative cargo"),Command.GetCarried(),6);
    V=HearthwardPresentation::ProjectCompanion(Command,EHearthwardCompanionPhase::WaitingAtCamp,Epoch,TEXT("实际资源不足：指定采集点已采尽"));
    TestTrue(TEXT("depleted source is explained without automatic replacement"),
        HearthwardPresentation::CompanionWorkText(V).Contains(TEXT("该来源再次可采后继续")));
    Command.Cancel();
    V=HearthwardPresentation::ProjectCompanion(Command,EHearthwardCompanionPhase::Cancelled,Epoch);
    TestTrue(TEXT("cancelled record remains visible"),V.HasTask && V.Terminal);
    TestTrue(TEXT("cancelled text retains cargo semantics"),HearthwardPresentation::CompanionWorkText(V).Contains(TEXT("已取得物资保留")));
    TestFalse(TEXT("cancelled record has no resume"),V.CanResume);
    TestFalse(TEXT("cancelled record has no cancel"),V.CanCancel);
    FHearthwardCompanionCommand Completed;const auto Done=Completed.Request(Epoch);
    Completed.Accept(Done,Epoch,TEXT("wood"),32,{TEXT("collect"),TEXT("return"),TEXT("deposit")});
    Completed.RecordAcquisition(32);Completed.RecordDelivery(Done,32);Completed.Carried=0;
    const auto End=HearthwardPresentation::ProjectCompanion(Completed,EHearthwardCompanionPhase::Completed,Epoch);
    TestTrue(TEXT("completed record is explicitly terminal"),End.HasTask && End.Terminal
        && HearthwardPresentation::CompanionWorkText(End).Contains(TEXT("委托完成")));
    TestTrue(TEXT("missing source is not rendered as zero progress"),
        !HearthwardPresentation::CompanionWorkText({}).Contains(TEXT("0 / 0")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHearthwardPartyStatusViewTest,"Hearthward.Iteration.Task086.PartyStatus",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FHearthwardPartyStatusViewTest::RunTest(const FString&)
{
    FHearthwardCampWorkView V;V.Available=V.HasTeam=V.AssignedBrother=V.Enabled=true;
    V.Job=TEXT("wood");V.Workers={0,2};V.CompletedBatches=7;
    V.Status=TEXT("等待资源刷新");V.BrotherStatus=TEXT("移动中，尚未计入岗位劳动力");
    const FString Text=HearthwardPresentation::WorkPartyText(V);
    TestTrue(TEXT("clan workers and brother use separate slots"),Text.Contains(TEXT("2名族人 · 弟弟另占1个名额")));
    TestTrue(TEXT("completed batches become actual wood units"),Text.Contains(TEXT("岗位累计入库 14 份")));
    TestTrue(TEXT("resource wait remains visible"),Text.Contains(TEXT("等待资源刷新")));
    TestTrue(TEXT("moving brother is not shown working"),Text.Contains(TEXT("尚未计入岗位劳动力")) && !Text.Contains(TEXT("已到岗工作")));
    TestFalse(TEXT("continuous team has no completion percentage"),Text.Contains(TEXT("%")));
    V.Enabled=false;V.Status=TEXT("已暂停");V.BrotherStatus=TEXT("已暂停，尚未计入劳动力");
    TestTrue(TEXT("pause preserves cumulative deposit"),HearthwardPresentation::WorkPartyText(V).Contains(TEXT("入库 14 份")));
    V.CompletedBatches.Reset();
    TestTrue(TEXT("unknown deposit is explicitly unknown"),HearthwardPresentation::WorkPartyText(V).Contains(TEXT("累计入库量暂不可用")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHearthwardWorkStatusActionBindingTest,"Hearthward.Iteration.Task086.StatusActionBinding",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FHearthwardWorkStatusActionBindingTest::RunTest(const FString&)
{
    FHearthwardCompanionWorkView Personal;Personal.Epoch=FGuid::NewGuid();Personal.CommandId=FGuid::NewGuid();
    const FString Resume=HearthwardPresentation::PersonalActionToken(TEXT("agentRetryPath"),Personal);
    const FString Cancel=HearthwardPresentation::PersonalActionToken(TEXT("cancelTask"),Personal);
    TestNotEqual(TEXT("resume and cancel stay distinct"),Resume,Cancel);
    Personal.CommandId=FGuid::NewGuid();
    TestNotEqual(TEXT("new command rejects the old card token"),Resume,HearthwardPresentation::PersonalActionToken(TEXT("agentRetryPath"),Personal));
    const FString BeforeLoad=HearthwardPresentation::PersonalActionToken(TEXT("cancelTask"),Personal);
    Personal.Epoch=FGuid::NewGuid();
    TestNotEqual(TEXT("new timeline rejects pre-load cancellation"),BeforeLoad,HearthwardPresentation::PersonalActionToken(TEXT("cancelTask"),Personal));
    FHearthwardCampWorkView Team;Team.Epoch=FGuid::NewGuid();Team.Region=TEXT("camp_wood");Team.Workers={0,2};
    const FString Stop=HearthwardPresentation::WorkPartyActionToken(TEXT("dialogue.stopTeam"),Team);
    TestNotEqual(TEXT("team resume and pause stay distinct"),Stop,HearthwardPresentation::WorkPartyActionToken(TEXT("dialogue.resumeTeam"),Team));
    Team.Workers={0,3};
    TestNotEqual(TEXT("changed workers invalidate old team card"),Stop,HearthwardPresentation::WorkPartyActionToken(TEXT("dialogue.stopTeam"),Team));
    Team.Epoch=FGuid::NewGuid();
    TestNotEqual(TEXT("loaded team requires a fresh action token"),Stop,HearthwardPresentation::WorkPartyActionToken(TEXT("dialogue.stopTeam"),Team));
    return true;
}
#endif
