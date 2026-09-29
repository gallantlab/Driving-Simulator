// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#include "Carla.h"
#include "CarlaDriveHUD.h"
#include "Vehicle/CarlaVehicleController.h"
#include "Vehicle/NavigationVehicleController.h"
#include "Game/CarlaPlayerState.h"

#define LOCTEXT_NAMESPACE "CarlaHUD"

//#define DEBUG		// log debug mesages
//#define NAV			//	is this a taxi driving experiment

ACarlaDriveHUD::ACarlaDriveHUD()
	: mph(FText::FromString(TEXT("mph")))
{
	static ConstructorHelpers::FObjectFinder <UFont> Font(TEXT("/Game/Baron_Neue_Font"));
	HUDFont = Font.Object;
	static ConstructorHelpers::FObjectFinder <UFont> Font2(TEXT("/Game/Baron_Neue_Font_Small"));
	HUDFontSmall = Font2.Object;
}

void ACarlaDriveHUD::DrawHUD()
{
	AHUD::DrawHUD();

	// Calculate ratio from 720p
#ifdef NAV
	ANavigationVehicleController* controller = Cast<ANavigationVehicleController>(GetOwningPawn() == nullptr ? nullptr : GetOwningPawn()->GetController());
#else
	ACarlaVehicleController* controller = Cast<ACarlaVehicleController>(GetOwningPawn() == nullptr ? nullptr : GetOwningPawn()->GetController());
#endif

	if (controller != nullptr)
	{
		speed = (int)(controller->GetPossessedVehicle()->GetVehicleForwardSpeed() * 0.0223704f); // mph conversion
		speed = speed > 0 ? speed : speed * -1;
		tens = speed / 10;
		ones = speed % 10;
		auto Text10 = FText::AsNumber(tens);
		auto Text1 = FText::AsNumber(ones);
		FCanvasTextItem onesText(FVector2D(Canvas->SizeX / 2.0 - 40, Canvas->SizeY - 80), Text1, HUDFont, FLinearColor::White);
		FCanvasTextItem mphText(FVector2D(Canvas->SizeX / 2.0, Canvas->SizeY - 65), mph, HUDFontSmall, FLinearColor::White);
		Canvas->DrawItem(mphText);
		Canvas->DrawItem(onesText);
		if (tens)
		{
			FCanvasTextItem tensText(FVector2D(Canvas->SizeX / 2.0 - 80, Canvas->SizeY - 80), Text10, HUDFont, FLinearColor::White);
			Canvas->DrawItem(tensText);
		}
		if (controller->isTTL())
		{
			FCanvasBoxItem TTLindicator(FVector2D(Canvas->SizeX - 2, Canvas->SizeY - 2), FVector2D(2, 2));
			Canvas->DrawItem(TTLindicator);
		}
#ifdef NAV
		if (controller->IsArrived())
		{
			// code to draw arrival message
		}
		else if (controller->NewDestination())
		{
			// code to draw new destination name
		}
#endif

#ifdef DEBUG
		FCanvasTextItem fps(FVector2D(Canvas->SizeX / 2.0 + 200, Canvas->SizeY - 65), FText::AsNumber((int)controller->GetPlayerState().GetFramesPerSecond()), HUDFontSmall, FLinearColor::White);
		Canvas->DrawItem(fps);
#endif
	}
}

#undef LOCTEXT_NAMESPACE