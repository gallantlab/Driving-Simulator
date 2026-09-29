// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#include "Carla.h"
#include "SpeedZoneBase.h"


// Sets default values

ASpeedZoneBase::ASpeedZoneBase(const FObjectInitializer &ObjectInitializer)
: Super(ObjectInitializer)
{
	RootComponent =	ObjectInitializer.CreateDefaultSubobject<USceneComponent>(this, TEXT("SceneRootComponent"));
	RootComponent->SetMobility(EComponentMobility::Static);

	PrimaryActorTick.bCanEverTick = false;
}
