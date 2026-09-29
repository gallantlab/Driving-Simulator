// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#include "Carla.h"
#include "CircuitTrackBoxBase.h"


// Sets default values
ACircuitTrackBoxBase::ACircuitTrackBoxBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
//	PrimaryActorTick.bCanEverTick = false;
	RootComponent = ObjectInitializer.CreateDefaultSubobject<USceneComponent>(this, TEXT("SceneRootComponent"));
	RootComponent->SetMobility(EComponentMobility::Static);

}

void ACircuitTrackBoxBase::BeginPlay()
{
	TArray<AActor *> Children = TArray<AActor *>();
	GetAllChildActors(Children);	// because the trigger box is attached to the subclass (stretchy triggerbox)
	for (int j = 0; j < Children.Num(); j++)
	{
		// yay anti-patterns
		// VS throws an error here. gcc doesn't. be like VS. Call anti-patterns out.
#ifndef _WIN32
		if ((triggerBox = Cast<ATriggerBox>(Children[j]))) break;
#else
		triggerBox = Cast<ATriggerBox>(Children[j]);
		if (triggerBox) break;
#endif
	}
	if (!triggerBox)
		UE_LOG(LogCarla, Error, TEXT("No triggerbox for this circuit track box. something is wrong."))
	Children.Empty();
}

bool ACircuitTrackBoxBase::TriggerBoxOverlaps(const AActor *other) const
{
	if (!triggerBox) return false;
	return triggerBox->IsOverlappingActor(other);
}