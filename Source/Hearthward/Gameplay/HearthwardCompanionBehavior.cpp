#include "HearthwardCompanionBehavior.h"

#include "HearthwardCompanionCombatPolicy.h"
#include "HearthwardGameData.h"
#include "../AI/HearthwardNPCRoutine.h"
#include "../Companion/HearthwardCompanionFixture.h"
#include "../Companion/HearthwardCompanionNavigationComponent.h"
#include "Engine/World.h"

namespace
{
double BehaviorTune(const FString& Key)
{
    return HearthwardData::Number(HearthwardData::Catalog()->GetObjectField(TEXT("tuning")),Key);
}
}

FHearthwardCompanionBehaviorResult HearthwardCompanionBehavior::Tick(const FHearthwardCompanionBehaviorContext& Context)
{
    FHearthwardCompanionBehaviorResult Out;
    Out.EffectiveOrder=Context.RequestedOrder;

    auto* Companion=Context.Companion;
    auto* Player=Context.Player;
    if(!IsValid(Companion) || !IsValid(Player) || !Companion->Navigation)
        return Out;

    using P=EHearthwardCompanionPhase;
    const P Phase=Companion->GetPhase();
    if(Phase!=P::Idle && Phase!=P::Completed && Phase!=P::Cancelled)
    {
        Out.EffectiveOrder=TEXT("wait");
        Out.Reason=TEXT("TASK_OWNS_COMPANION");
        return Out;
    }

    if(Context.bRoutineEnabled && Context.RequestedOrder==TEXT("wait") && IsValid(Companion->Camp))
    {
        FHearthwardNPCRoutineContext RoutineContext;
        RoutineContext.Enabled=true;
        RoutineContext.bOrderIdle=true;
        RoutineContext.bPlayerInCombat=Context.bPlayerInCombat;
        RoutineContext.bPlayerDown=Context.bPlayerDown;
        RoutineContext.bTaskActive=false;
        RoutineContext.CompanionToCampDistance=FVector::Dist2D(Companion->GetActorLocation(),Companion->Camp->GetActorLocation());
        RoutineContext.GameSeconds=Context.GameSeconds;
        const auto Routine=HearthwardRoutine::Evaluate(RoutineContext);
        if(Routine.Active)
        {
            Out.RoutineActivity=Routine.Activity;
            Out.TacticalIntent=TEXT("routine");
            Out.Reason=TEXT("ROUTINE_")+Routine.Activity.ToString().ToUpper();
            const FVector Destination=Companion->Camp->GetActorLocation()+Routine.CampOffset;
            if(FVector::Dist2D(Companion->GetActorLocation(),Destination)>Routine.AcceptanceRadius)
                Companion->BlockReason=Companion->Navigation->MoveToLocation(Destination,Routine.MoveSpeed,Routine.AcceptanceRadius)
                    ?TEXT("自由活动 · ")+Routine.Activity.ToString():TEXT("营地路线暂时不可达");
            else
            {
                Companion->Navigation->Stop();
                Companion->BlockReason=TEXT("自由活动 · ")+Routine.Activity.ToString();
            }
            return Out;
        }
    }

    FHearthwardCompanionCombatObservation Observation;
    Observation.RequestedOrder=Context.RequestedOrder;
    Observation.PlayerHealthRatio=Context.PlayerHealthRatio;
    Observation.bPlayerInCombat=Context.bPlayerInCombat;
    Observation.PlayerPosition=Player->GetActorLocation();
    Observation.CompanionPosition=Companion->GetActorLocation();
    Observation.CompanionToPlayerDistance=FVector::Dist2D(Observation.CompanionPosition,Observation.PlayerPosition);
    Observation.CommandRange=BehaviorTune(TEXT("companionCommandRange"));
    Observation.ProtectHealthRatio=BehaviorTune(TEXT("companionProtectHealthRatio"));
    Observation.ProtectRadius=BehaviorTune(TEXT("companionProtectRadius"));
    Observation.RegroupThreatCount=FMath::RoundToInt(BehaviorTune(TEXT("companionRegroupThreatCount")));
    for(const auto& Source:Context.Threats)
    {
        if(!Source.Actor.IsValid())continue;
        FHearthwardCompanionThreat Threat;
        Threat.Id=Source.Id;
        Threat.Position=Source.Actor->GetActorLocation();
        Threat.RemainingHealth=Source.RemainingHealth;
        Observation.Threats.Add(Threat);
    }

    const auto Decision=HearthwardCombatPolicy::Evaluate(Observation);
    Out.TacticalIntent=HearthwardCombatPolicy::IntentName(Decision.Intent);
    Out.CombatTarget=Decision.Target;
    Out.Reason=Decision.Reason;

    if(Decision.Intent==EHearthwardCompanionTacticalIntent::Hold)
    {
        Companion->Navigation->Stop();
        Companion->BlockReason.Reset();
        return Out;
    }

    AActor* Destination=Player;
    if(Decision.Intent==EHearthwardCompanionTacticalIntent::Assist
        || Decision.Intent==EHearthwardCompanionTacticalIntent::Protect)
    {
        const auto* Target=Context.Threats.FindByPredicate([&](const auto& X){return X.Id==Decision.Target;});
        if(!Target || !Target->Actor.IsValid())
        {
            Out.TacticalIntent=TEXT("follow");
            Out.CombatTarget=NAME_None;
            Out.Reason=TEXT("TARGET_DISAPPEARED");
        }
        else
        {
            Destination=Target->Actor.Get();
        }
    }

    const bool bTargeting=!Out.CombatTarget.IsNone();
    const bool bRegroup=Decision.Intent==EHearthwardCompanionTacticalIntent::Regroup;
    const float StopDistance=bTargeting?BehaviorTune(TEXT("attackRange"))*.8f
        :bRegroup?BehaviorTune(TEXT("companionRegroupDistance")):BehaviorTune(TEXT("companionFollowDistance"));
    FVector Direction=Destination->GetActorLocation()-Companion->GetActorLocation();
    Direction.Z=0;

    if(bTargeting && Direction.Size()<=StopDistance)
    {
        FHitResult Hit;
        FCollisionQueryParams Query(SCENE_QUERY_STAT(HearthwardCompanionAttack),false,Companion);
        Query.AddIgnoredActor(Player);
        if(!Companion->GetWorld()->LineTraceSingleByChannel(
            Hit,Companion->GetActorLocation(),Destination->GetActorLocation(),ECC_Visibility,Query))
        {
            Companion->Navigation->Stop();
            Companion->BlockReason.Reset();
            if(Context.bCanAttack)
            {
                Out.bAttackCommitted=true;
                Out.DamageTarget=Out.CombatTarget;
            }
            return Out;
        }

        Companion->BlockReason=Companion->Navigation->MoveToActor(Destination,BehaviorTune(TEXT("companionMoveSpeed")),30)
            ?TEXT("正在绕行接近目标"):TEXT("目标不可达，请调整位置");
        return Out;
    }

    if(Direction.Size()>StopDistance)
    {
        const float Acceptance=FMath::Max(20.f,StopDistance-10.f);
        Companion->BlockReason=Companion->Navigation->MoveToActor(Destination,BehaviorTune(TEXT("companionMoveSpeed")),Acceptance)
            ?(Decision.Intent==EHearthwardCompanionTacticalIntent::Protect?TEXT("正在保护你，拦截近身威胁")
                :bTargeting?TEXT("正在接近威胁")
                :bRegroup?TEXT("压力过大，正在回撤会合"):TEXT(""))
            :TEXT("目标不可达，请调整位置");
    }
    else
    {
        Companion->Navigation->Stop();
        Companion->BlockReason.Reset();
    }
    return Out;
}
