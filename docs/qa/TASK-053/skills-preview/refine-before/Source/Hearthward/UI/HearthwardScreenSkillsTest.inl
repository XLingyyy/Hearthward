// PROTOTYPE_ONLY: native skill-tree checks inside an explicitly isolated PIE save pool.
#if !UE_BUILD_SHIPPING
#include "../Gameplay/HearthwardProgression.h"
namespace
{
void VerifySkillsPreview(UWorld* World)
{
    const FString Run=FPlatformMisc::GetEnvironmentVariable(TEXT("HEARTHWARD_TITLE_RUN"));
    FString Pool;FGuid PoolId;
    if(!World || World->WorldType!=EWorldType::PIE || !Run.StartsWith(TEXT("verify_"))
        || !FParse::Value(FCommandLine::Get(),TEXT("HearthwardSaveTestPool="),Pool) || !FGuid::Parse(Pool,PoolId) || !PoolId.IsValid()) return;
    auto Report=MakeShared<FJsonObject>(),Checks=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Captures,Layouts;bool Passed=true;
    auto Check=[&](const FString& Name,bool Result){Checks->SetBoolField(Name,Result);Passed&=Result;};
    auto* Controller=World->GetFirstPlayerController();auto* HUD=Controller?Cast<AHearthwardHUD>(Controller->GetHUD()):nullptr;
    auto* UI=HUD?HUD->Screen.Get():nullptr;Check(TEXT("screen_available"),UI && Controller->GetPawn());
    if(UI && Controller->GetPawn())
    {
        auto* Pawn=Controller->GetPawn().Get();auto* G=Pawn->FindComponentByClass<UHearthwardGameplayComponent>();
        auto* Bag=Pawn->FindComponentByClass<UHearthwardInventoryComponent>();auto* Settings=World->GetGameInstance()->GetSubsystem<UHearthwardPlayerSettings>();
        const FString GameplayBefore=G->SaveSnapshot(),FeedbackBefore=G->Feedback,InventoryBefore=Bag->DescribeInventory();
        const auto ComfortBefore=Settings->Comfort;const int32 SavesBefore=World->GetSubsystem<UHearthwardSaveSubsystem>()->GetPoints().Num();
        G->Skills.Reset();G->Experience=HearthwardData::Number(HearthwardData::Find(TEXT("levels"),TEXT("3")),TEXT("total_xp"));
        Settings->Comfort.TextScale=100;UI->OpenPage(TEXT("skills"));UI->ExecuteAction(TEXT("skill:strong"));
        Check(TEXT("fixture_can_change_skills"),G->CanChangeSkills());const int32 InitialPoints=G->SkillPoints();
        auto Snapshot=[&]()
        {TSharedPtr<FJsonObject> Object;FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(UI->DescribeHUDPreview()),Object);return Object;};
        auto Find=[&](const TSharedPtr<FJsonObject>& Object,const FString& Id)->TSharedPtr<FJsonObject>
        {for(const auto& V:Object->GetArrayField(TEXT("elements")))if(V->AsObject()->GetStringField(TEXT("id"))==Id)return V->AsObject();return nullptr;};
        auto Clean=[&](const FString& Name)
        {
            const auto Object=Snapshot();Layouts.Add(MakeShared<FJsonValueObject>(Object));
            int32 Nodes=0,Edges=0,Surfaces=0,Backdrops=0,ExpectedEdges=0;bool NoLeather=true,Inside=true;
            for(const auto& V:HearthwardData::Rows(TEXT("skills")))ExpectedEdges+=!HearthwardData::Text(V->AsObject(),TEXT("requires")).IsEmpty();
            for(const auto& V:Object->GetArrayField(TEXT("elements")))
            {
                const auto E=V->AsObject();const FString Type=E->GetStringField(TEXT("type")),Asset=E->GetStringField(TEXT("asset"));
                Nodes+=Type==TEXT("node");Edges+=Type==TEXT("connection");Surfaces+=Type==TEXT("skillsSurface");Backdrops+=Type==TEXT("skillsBackdrop");
                NoLeather&=Asset!=TEXT("leatherPanel") && Asset!=TEXT("darkTexture") && Asset!=TEXT("skillsOriginalArt0") && Asset!=TEXT("skillsOriginalArt1");
                Inside&=E->GetNumberField(TEXT("x"))>=0 && E->GetNumberField(TEXT("y"))>=0
                    && E->GetNumberField(TEXT("x"))+E->GetNumberField(TEXT("width"))<=1672.01
                    && E->GetNumberField(TEXT("y"))+E->GetNumberField(TEXT("height"))<=941.01;
            }
            Check(Name+TEXT("_all_original_nodes_and_edges"),Nodes==HearthwardData::Rows(TEXT("skills")).Num() && Edges==ExpectedEdges);
            Check(Name+TEXT("_four_trees_detail_and_matte_background"),Surfaces==5 && Backdrops==1 && NoLeather);
            Check(Name+TEXT("_elements_inside_viewport"),Inside);
            const auto Description=Find(Object,TEXT("skills.detail.description")),FirstLevel=Find(Object,TEXT("skills.detail.level.1"));
            Check(Name+TEXT("_description_and_levels_separated"),Description && FirstLevel && Description->GetNumberField(TEXT("y"))+Description->GetNumberField(TEXT("height"))+20<=FirstLevel->GetNumberField(TEXT("y"))+.01);
        };
        auto Capture=[&](const FString& Name,int32 Width=1672,int32 Height=941)
        {const FString File=Run+TEXT("-")+Name;const bool Done=UI->CaptureUI(File,Width,Height);Check(TEXT("capture_")+Name,Done);if(Done)Captures.Add(MakeShared<FJsonValueString>(File+TEXT(".png")));};
        Clean(TEXT("initial"));Capture(TEXT("skills-default"));
        Check(TEXT("original_budget_displayed"),Find(Snapshot(),TEXT("skills.header.points"))->GetStringField(TEXT("text"))==FString::FromInt(InitialPoints));
        const FString SelectionBefore=G->SaveSnapshot();
        for(const auto& V:HearthwardData::Rows(TEXT("skills")))
        {
            const auto R=V->AsObject();const FString Id=HearthwardData::Text(R,TEXT("id"));const auto Node=Find(Snapshot(),TEXT("skills.node.")+Id);
            Check(TEXT("node_hit_")+Id,Node && UI->ActionAt(FVector2D(Node->GetNumberField(TEXT("x"))+28,Node->GetNumberField(TEXT("y"))+28))==TEXT("skill:")+Id);
            Check(TEXT("node_selection_description_")+Id,UI->ExecuteAction(TEXT("skill:")+Id) && Find(Snapshot(),TEXT("skills.detail.description"))->GetStringField(TEXT("text"))==HearthwardData::Text(R,TEXT("description")));
        }
        Check(TEXT("selection_does_not_spend_or_change_gameplay"),G->SaveSnapshot()==SelectionBefore);
        UI->ExecuteAction(TEXT("skill:guard"));
        Check(TEXT("missing_prerequisite_blocks_learning"),!UI->ExecuteAction(TEXT("learn")) && G->Skills.FindRef(TEXT("guard"))==0 && G->SkillPoints()==InitialPoints && G->Feedback.Contains(TEXT("前置")));
        Check(TEXT("original_prerequisite_shown"),Find(Snapshot(),TEXT("skills.detail.prerequisite"))->GetStringField(TEXT("text"))==TEXT("前置技能：重击精通"));
        UI->ExecuteAction(TEXT("skill:strong"));
        Check(TEXT("root_learning_uses_original_cost"),UI->ExecuteAction(TEXT("learn")) && G->Skills.FindRef(TEXT("strong"))==1 && G->SkillPoints()==InitialPoints-1);
        UI->ExecuteAction(TEXT("skill:guard"));
        const FGeometry Geometry=FGeometry::MakeRoot(FVector2D(1672,941),FSlateLayoutTransform());
        UI->NativeOnKeyDown(Geometry,FKeyEvent(EKeys::F,FModifierKeysState(),0,false,0,0));
        Check(TEXT("native_learning_key_retained"),G->Skills.FindRef(TEXT("guard"))==1 && G->SkillPoints()==InitialPoints-2);
        const auto Learned=Snapshot();
        Check(TEXT("learned_node_rank_and_branch_total_updated"),Find(Learned,TEXT("skills.rank.guard"))->GetStringField(TEXT("text"))==TEXT("1/3") && Find(Learned,TEXT("skills.branch.0.progress"))->GetStringField(TEXT("text"))==TEXT("2 / 24"));
        Check(TEXT("learned_edge_uses_original_parent"),Find(Learned,TEXT("skills.edge.guard"))->GetNumberField(TEXT("green"))>Find(Learned,TEXT("skills.edge.guard"))->GetNumberField(TEXT("red")));
        Capture(TEXT("skills-learned"));
        UI->ExecuteAction(TEXT("skill:strong"));UI->ExecuteAction(TEXT("learn"));
        Check(TEXT("zero_points_blocks_learning"),!UI->ExecuteAction(TEXT("learn")) && G->SkillPoints()==0 && G->Skills.FindRef(TEXT("strong"))==2 && G->Feedback.Contains(TEXT("不足")));
        Check(TEXT("original_free_respec_refunds_points"),UI->ExecuteAction(TEXT("respec")) && G->Skills.IsEmpty() && G->SkillPoints()==InitialPoints);
        bool LearnedMaximum=true;for(int32 RankIndex=0;RankIndex<HearthwardData::Number(HearthwardData::Find(TEXT("skills"),TEXT("strong")),TEXT("maxRank"));++RankIndex)LearnedMaximum&=UI->ExecuteAction(TEXT("learn"));
        Check(TEXT("original_rank_limit_preserved"),LearnedMaximum && !UI->ExecuteAction(TEXT("learn")) && G->Skills.FindRef(TEXT("strong"))==3 && G->Feedback.Contains(TEXT("最高")));
        UI->ExecuteAction(TEXT("skill:scout"));Clean(TEXT("locked_detail"));Capture(TEXT("skills-locked-detail"));
        Settings->Comfort.TextScale=150;UI->Refresh();Clean(TEXT("text_150"));Capture(TEXT("skills-text-150"));
        Capture(TEXT("skills-720p"),1280,720);Capture(TEXT("skills-16x10"),2560,1600);Capture(TEXT("skills-ultrawide"),2560,1080);
        Check(TEXT("large_text_retains_clickable_trees"),UI->ActionAt({190,350})==TEXT("skill:strong"));
        Check(TEXT("settings_entry_retained"),UI->ExecuteAction(TEXT("page:settings")) && UI->GetPage()==TEXT("settings"));
        Check(TEXT("settings_back_returns_to_skills"),UI->ExecuteAction(TEXT("back")) && UI->GetPage()==TEXT("skills"));
        Check(TEXT("back_returns_to_game"),UI->ExecuteAction(TEXT("back")) && UI->GetPage()==TEXT("hud"));
        Settings->Comfort=ComfortBefore;G->Restore(GameplayBefore);G->Feedback=FeedbackBefore;
        Check(TEXT("gameplay_and_inventory_restored"),G->SaveSnapshot()==GameplayBefore && Bag->DescribeInventory()==InventoryBefore);
        Check(TEXT("save_nodes_unchanged"),World->GetSubsystem<UHearthwardSaveSubsystem>()->GetPoints().Num()==SavesBefore);UI->OpenPage(TEXT("title"));
    }
    Report->SetBoolField(TEXT("passed"),Passed);Report->SetObjectField(TEXT("checks"),Checks);Report->SetArrayField(TEXT("captures"),Captures);Report->SetArrayField(TEXT("layouts"),Layouts);
    FString Json;FJsonSerializer::Serialize(Report,TJsonWriterFactory<>::Create(&Json));
    FFileHelper::SaveStringToFile(Json,*(FPaths::ProjectSavedDir()/TEXT("TitleWheel")/Run/TEXT("skills-preview.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}
FAutoConsoleCommandWithWorld VerifySkillsPreviewCommand(TEXT("Hearthward.UI.VerifySkillsPreview"),TEXT("Verify skill-tree presentation in an explicit isolated PIE save pool."),FConsoleCommandWithWorldDelegate::CreateStatic(&VerifySkillsPreview));
}
#endif
