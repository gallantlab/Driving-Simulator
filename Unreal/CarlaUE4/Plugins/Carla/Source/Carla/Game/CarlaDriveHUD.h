// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#pragma once

#include "CoreMinimal.h"
#include "Game/CarlaHUD.h"
#include "CarlaDriveHUD.generated.h"

/**
 * 
 */
UCLASS()
class CARLA_API ACarlaDriveHUD : public ACarlaHUD
{
	GENERATED_BODY()

public:
	ACarlaDriveHUD();

	virtual void DrawHUD() override;

	UPROPERTY(EditAnywhere)
	UFont* HUDFontSmall;

private:
	int speed, tens, ones;
	FText mph;
};
