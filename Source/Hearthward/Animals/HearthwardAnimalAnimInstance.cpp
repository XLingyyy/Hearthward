#include "HearthwardAnimalAnimInstance.h"
#include "HearthwardAnimalMotionComponent.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimNode_SequencePlayer.h"
#include "Animation/AnimSequence.h"
#include "AnimNodes/AnimNode_TwoWayBlend.h"

namespace
{
struct FAnimalBlend : FAnimNode_TwoWayBlend
{
    FAnimalBlend(){bAlwaysUpdateChildren=true;}
};
struct FAnimalAnimProxy : FAnimInstanceProxy
{
    FAnimNode_SequencePlayer_Standalone Players[2];
    FAnimalBlend Blend;
    int32 Revision=-1,Slot=0;
    float Target=0;
    explicit FAnimalAnimProxy(UAnimInstance* Instance):FAnimInstanceProxy(Instance){}
    virtual void Initialize(UAnimInstance* Instance) override
    {
        Blend.A.SetLinkNode(&Players[0]);Blend.B.SetLinkNode(&Players[1]);
        FAnimInstanceProxy::Initialize(Instance);
    }
    virtual FAnimNode_Base* GetCustomRootNode() override{return &Blend;}
    virtual void PreUpdate(UAnimInstance* Instance,float Delta) override
    {
        FAnimInstanceProxy::PreUpdate(Instance,Delta);
        const auto* Owner=Instance->GetOwningActor();
        const auto* Motion=Owner?Owner->FindComponentByClass<UHearthwardAnimalMotionComponent>():nullptr;
        if(!Motion || !Motion->Ready())return;
        if(Revision!=Motion->StateRevision)
        {
            if(Revision<0)
            {
                for(auto& P:Players){P.SetSequence(Motion->Sequence());P.SetLoopAnimation(Motion->Loops());P.SetAccumulatedTime(0);}
            }
            else Slot=1-Slot;
            Players[Slot].SetSequence(Motion->Sequence());Players[Slot].SetLoopAnimation(Motion->Loops());Players[Slot].SetAccumulatedTime(0);
            Target=float(Slot);Revision=Motion->StateRevision;
        }
        Players[Slot].SetPlayRate(Motion->PlayRate);
        Blend.Alpha=FMath::FInterpConstantTo(Blend.Alpha,Target,Delta,1.f/.14f);
    }
};
}
FAnimInstanceProxy* UHearthwardAnimalAnimInstance::CreateAnimInstanceProxy(){return new FAnimalAnimProxy(this);}
void UHearthwardAnimalAnimInstance::DestroyAnimInstanceProxy(FAnimInstanceProxy* Proxy){delete Proxy;}
