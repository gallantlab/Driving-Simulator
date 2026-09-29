// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#include "Carla.h"
#include "CircuitHUDBase.h"



UCircuitHUDBase::UCircuitHUDBase(const FObjectInitializer &objectInitializer)
	: Super(objectInitializer)
{
	circuitPlayerController = nullptr;
}


FString UCircuitHUDBase::GetDisplayText() const
{
	if (circuitPlayerController)
	{
		if (!circuitPlayerController->IsOnCourse())
			return OFF_COURSE;

	}
	return FString("");
}


void UCircuitHUDBase::FindController()
{
	Super::FindController();
	circuitPlayerController = Cast<ACircuitPlayerController>(controller);
}