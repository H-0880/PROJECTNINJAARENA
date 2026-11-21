#include "NinjaFighterAnimInstance.h"

#include "Animation/AnimInstanceProxy.h"
#include "AnimNodes/AnimNode_RetargetPoseFromMesh.h"

namespace
{
struct FNinjaFighterRetargetNode final : FAnimNode_RetargetPoseFromMesh
{
	virtual void Evaluate_AnyThread(FPoseContext& Output) override
	{
		FAnimNode_RetargetPoseFromMesh::Evaluate_AnyThread(Output);
		FCompactPose& Pose = Output.Pose;
		bool bAnimated = false;
		for (const FCompactPoseBoneIndex Bone : Pose.ForEachBoneIndex())
		{
			if (!Pose[Bone].Equals(Pose.GetRefPose(Bone)))
			{
				bAnimated = true;
				break;
			}
		}
		if (!bAnimated)
		{
			return;
		}

		// The retargeter solves with unit scale and restores the FBX bone scales afterwards.
		// Convert the local offsets back to the scaled parent space before they are composed.
		TArray<FVector, TInlineAllocator<128>> ComponentScales;
		ComponentScales.SetNumUninitialized(Pose.GetNumBones());
		for (const FCompactPoseBoneIndex Bone : Pose.ForEachBoneIndex())
		{
			const FCompactPoseBoneIndex Parent = Pose.GetParentBoneIndex(Bone);
			const FVector ParentScale = Parent.GetInt() == INDEX_NONE ? FVector::OneVector : ComponentScales[Parent.GetInt()];
			FTransform& Transform = Pose[Bone];
			Transform.SetTranslation(Transform.GetTranslation() * FTransform::GetSafeScaleReciprocal(ParentScale));
			ComponentScales[Bone.GetInt()] = Transform.GetScale3D() * ParentScale;
		}
	}
};

class FNinjaFighterAnimProxy final : public FAnimInstanceProxy
{
public:
	explicit FNinjaFighterAnimProxy(UAnimInstance* Instance)
		: FAnimInstanceProxy(Instance)
	{
	}

	FNinjaFighterRetargetNode Retarget;

private:
	virtual FAnimNode_Base* GetCustomRootNode() override { return &Retarget; }
	virtual void GetCustomNodes(TArray<FAnimNode_Base*>& OutNodes) override { OutNodes.Add(&Retarget); }
};
}

void UNinjaFighterAnimInstance::SetRetargetAsset(UIKRetargeter* Asset)
{
	RetargetAsset = Asset;
	GetProxyOnGameThread<FNinjaFighterAnimProxy>().Retarget.IKRetargeterAsset = Asset;
}

FAnimInstanceProxy* UNinjaFighterAnimInstance::CreateAnimInstanceProxy()
{
	auto* Proxy = new FNinjaFighterAnimProxy(this);
	Proxy->Retarget.IKRetargeterAsset = RetargetAsset;
	return Proxy;
}
