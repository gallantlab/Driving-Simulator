// Copyright (c) 2017 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#include "Carla.h"
#include "CarlaVehicleController.h"

#include "Sensor/Lidar.h"
#include "Sensor/SceneCaptureCamera.h"

#include "Components/BoxComponent.h"
#include "Game/CarlaGameInstance.h"
#include "GameFramework/Pawn.h"
#include "WheeledVehicleMovementComponent.h"
#include "CarlaWheeledVehicle.h"
#include "MapGen/RoadMap.h"

#define LOG3 FMath::Loge(3)

// =============================================================================
// -- Constructor and destructor -----------------------------------------------
// =============================================================================

ACarlaVehicleController::ACarlaVehicleController(const FObjectInitializer& ObjectInitializer) :
		Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;

	// find the beep
	static ConstructorHelpers::FObjectFinder<USoundCue> beepCueFinder(TEXT("/Game/Sounds/440Hz_Cue"));
	beepCue = beepCueFinder.Object;
	UE_LOG(LogFMRI, Log, TEXT("440HZ beep pointer is%s valid"), beepCue->IsValidLowLevelFast() ? TEXT("") : TEXT(" not"));

	// find the pickup sound
	static ConstructorHelpers::FObjectFinder<USoundCue> pickupFinder(TEXT("/Game/Sounds/pickup_Cue"));
	pickupCue = pickupFinder.Object;
	UE_LOG(LogFMRI, Log, TEXT("Pickup sound pointer is%s valid"), beepCue->IsValidLowLevelFast() ? TEXT("") : TEXT(" not"));
}

ACarlaVehicleController::~ACarlaVehicleController()
{}

// =============================================================================
// -- APlayerController --------------------------------------------------------
// =============================================================================

void ACarlaVehicleController::Possess(APawn* aPawn)
{
	Super::Possess(aPawn);

	if (IsPossessingAVehicle())
	{
		// Bind hit events.
		aPawn->OnActorHit.AddDynamic(this, &ACarlaVehicleController::OnCollisionEvent);
		// Get custom player state.
		CarlaPlayerState = Cast<ACarlaPlayerState>(PlayerState);
		check(CarlaPlayerState != nullptr);
		CarlaPlayerState->NetUpdateFrequency = 30;
		CarlaPlayerState->MinNetUpdateFrequency = 30;
		UE_LOG(LogCarla, Log, TEXT("Update frequency %f min frequency %f"), CarlaPlayerState->NetUpdateFrequency, CarlaPlayerState->MinNetUpdateFrequency);
		// We can set the bounding box already as it's not going to change.
		CarlaPlayerState->BoundingBoxTransform = GetPossessedVehicle()->GetVehicleBoundingBoxTransform();
		CarlaPlayerState->BoundingBoxExtent = GetPossessedVehicle()->GetVehicleBoundingBoxExtent();
	}
}

void ACarlaVehicleController::BeginPlay()
{
	Super::BeginPlay();

	ReloadSettings();
}

APawn* ACarlaVehicleController::GetVehiclePawn()
{
	return this->GetPawn();
}

// =============================================================================
// -- AActor -------------------------------------------------------------------
// =============================================================================

void ACarlaVehicleController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (IsPossessingAVehicle())
	{
		auto Vehicle = GetPossessedVehicle();
		CarlaPlayerState->UpdateTimeStamp(DeltaTime);
		const FVector PreviousSpeed = CarlaPlayerState->ForwardSpeed * CarlaPlayerState->GetOrientation();
		CarlaPlayerState->Transform = Vehicle->GetVehicleTransform();
		CarlaPlayerState->ForwardSpeed = Vehicle->GetVehicleForwardSpeed();
		const FVector CurrentSpeed = CarlaPlayerState->ForwardSpeed * CarlaPlayerState->GetOrientation();
		CarlaPlayerState->Acceleration = (CurrentSpeed - PreviousSpeed) / DeltaTime;
//		const auto& AutopilotControl = GetAutopilotControl();
//		CarlaPlayerState->Steer = AutopilotControl.Steer;
//		CarlaPlayerState->Throttle = AutopilotControl.Throttle;
//		CarlaPlayerState->Brake = AutopilotControl.Brake;
//		CarlaPlayerState->bHandBrake = AutopilotControl.bHandBrake;
		CarlaPlayerState->CurrentGear = Vehicle->GetVehicleCurrentGear();
		CarlaPlayerState->SpeedLimit = GetSpeedLimit();
		CarlaPlayerState->TrafficLightState = GetTrafficLightState();
		IntersectPlayerWithRoadMap();

		if (bAllowUserInput)
			secondsWithNoSteeringInput += DeltaTime;
		if (holdBrakes > 0)
		{
			holdBrakes -= DeltaTime;
			if (holdBrakes > 0)
				Vehicle->HoldHandbrake();
			else
			{
				Vehicle->ReleaseHandbrake();
				OnPlayerRegainControl();
			}
		}
		if (bAllowUserInput && (secondsBeforeAISteering > 0) && (secondsWithNoSteeringInput > secondsBeforeAISteering))
		{
			CarlaPlayerState->bIsAutoSteerOn = true;
			FVector direction;
			Vehicle->SetSteeringInput(CarlaPlayerState->ForwardSpeed > 0.1 ? CalcSteeringValue(direction) : 0);
		}
		if (CarlaPlayerState->ForwardSpeed < 0.01)
			secondsAtRest += DeltaTime;
		else
			secondsAtRest = 0;
		if (bRelinquishControlOnStop && secondsAtRest > 1.0) RelinquishPlayerControl();
	}
}

// =============================================================================
// -- Events -------------------------------------------------------------------
// =============================================================================

void ACarlaVehicleController::OnCollisionEvent(AActor* Actor, AActor* OtherActor, FVector NormalImpulse, const FHitResult& Hit)
{
	// Register collision only if we are moving faster than 1 km/h.
	check(IsPossessingAVehicle());
	if (FMath::Abs(GetPossessedVehicle()->GetVehicleForwardSpeed() * 0.036f) > 1.0f)
		CarlaPlayerState->RegisterCollision(Actor, OtherActor, NormalImpulse, Hit);
}

// =============================================================================
// -- Other --------------------------------------------------------------------
// =============================================================================

void ACarlaVehicleController::IntersectPlayerWithRoadMap()
{
	auto RoadMap = GetRoadMap();
	if (RoadMap == nullptr)
	{
		UE_LOG(LogCarla, Error, TEXT("Controller doesn't have a road map!"));
		return;
	}

	check(IsPossessingAVehicle());
	auto Vehicle = GetPossessedVehicle();
	constexpr float ChecksPerCentimeter = 0.1f;
	const auto* BoundingBox = Vehicle->GetVehicleBoundingBox();
	check(BoundingBox != nullptr);
	auto Result = RoadMap->Intersect(
			BoundingBox->GetComponentTransform(),
			BoundingBox->GetUnscaledBoxExtent(),
			ChecksPerCentimeter);

	CarlaPlayerState->OffRoadIntersectionFactor = Result.OffRoad;
	CarlaPlayerState->OtherLaneIntersectionFactor = Result.OppositeLane;
}


void ACarlaVehicleController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if (InputComponent)
	{
		// TTL info
		InputComponent->BindAction("TTL", IE_Pressed, this, &ACarlaVehicleController::TTLdown);
		InputComponent->BindAction("TTL", IE_Released, this, &ACarlaVehicleController::TTLup);

		// Beep info - ephys
		InputComponent->BindAction("PlayBeep", IE_Pressed, this, &ACarlaVehicleController::PlayBeep);
		InputComponent->BindAction("PlayBeep", IE_Released, this, &ACarlaVehicleController::TTLup);
	}
}

void ACarlaVehicleController::TTLdown()
{
	UE_LOG(LogFMRI, Log, TEXT("TTL on %s"), *FDateTime::Now().ToString());
	CarlaPlayerState->TTL = true;
	CarlaPlayerState->TotalTTLs++;
}

void ACarlaVehicleController::TTLup()
{
	UE_LOG(LogFMRI, Log, TEXT("TTL off %s"), *FDateTime::Now().ToString());
	CarlaPlayerState->TTL = false;
}

bool ACarlaVehicleController::isTTL() const
{
	return CarlaPlayerState->isTTL();
}


void ACarlaVehicleController::PlayBeep()
{
	if (beepCue)
	{
		UE_LOG(LogFMRI, Log, TEXT("Playing beep"));
		UGameplayStatics::PlaySound2D(GetWorld(), beepCue);
		CarlaPlayerState->PlayBeep = true;
		GetWorld()->GetTimerManager().SetTimer(beepTimer, this, &ACarlaVehicleController::OnBeepEnd, 0.5, false);
		TTLdown();	// also include the TTL marker for this
	}
}


void ACarlaVehicleController::PlayPickup()
{
	if (pickupCue)
	{
		UE_LOG(LogFMRI, Log, TEXT("Playing pickup sound"));
		UGameplayStatics::PlaySound2D(GetWorld(), pickupCue);
	}
}


void ACarlaVehicleController::SetSecondsToStartOfRun(float seconds)
{
	UE_LOG(LogFMRI, Log, TEXT("First TTL at time %f seconds"), seconds);
	CarlaPlayerState->secondsToStartOfRun = seconds;
	isFirstTTLInRun = false;
}

void ACarlaVehicleController::SetFixedRoute(const TArray<FVector> &Locations,
											bool bOverwriteCurrent, int newDirection)
{
//	UE_LOG(LogCarla, Log, TEXT("Carla vehicle controller set fixed route"));
	if (newDirection == 0)
	{
//		UE_LOG(LogCarla, Log, TEXT("Carla vehicle controller not relinquishing player control"));
		return;    // this is coming from a regular route planner, so we ignore
	}

	Super::SetFixedRoute(Locations, bOverwriteCurrent, newDirection);

	UE_LOG(LogFMRI, Log, TEXT("Controller will relinquish control after player stops"))
	bRelinquishControlOnStop = true;
}

void ACarlaVehicleController::OnFixedRouteFinished()
{
	UE_LOG(LogCarla, Log, TEXT("Controller resuming human control"));
	EnableUserInput(true);
	SetAutopilot(false);
	bMoveSlow = false;
	// hold brakes so vehicle doesn't just roll
	holdBrakes = 0.5;
	CarlaPlayerState->bIsUnderPlayerControl = true;
}

void ACarlaVehicleController::SetSteeringInput(float value)
{
	if (steerMidpoint > 1)
		Super::SetSteeringInput(value * steerSensitivity * (1.0 - 1.0 / (1 + FMath::Exp(steerExponent * (CarlaPlayerState->ForwardSpeed - steerMidpoint)))));
	else
		Super::SetSteeringInput(value * steerSensitivity);

	CarlaPlayerState->Steer = value;

	if (value != lastSteering)
	{
		secondsWithNoSteeringInput = 0;
		CarlaPlayerState->bIsAutoSteerOn = false;
	}
	lastSteering = value;
}

void ACarlaVehicleController::SetThrottleInput(float value)
{
	if (throttleMidpoint > 1) // non-zero but we don't want to do a float zero comparison
		Super::SetThrottleInput(value * throttleSensitivity * (1.0 - 1.0 / (1 + FMath::Exp(throttleExponent * (CarlaPlayerState->ForwardSpeed - throttleMidpoint)))));
	else
		Super::SetThrottleInput(value * throttleSensitivity);
	CarlaPlayerState->Throttle = value;
	CarlaPlayerState->DesiredThrottle = value;
}

void ACarlaVehicleController::SetBrakeInput(float value)
{
	if (brakeMidpoint > 1)
		Super::SetBrakeInput(value * brakeSensitivity * (1.0 - 1.0 / (1 + FMath::Exp(brakeExponent * (CarlaPlayerState->ForwardSpeed - brakeMidpoint)))));
	else
		Super::SetBrakeInput(value * brakeSensitivity);
	CarlaPlayerState->Brake = value;
}

void ACarlaVehicleController::HoldHandbrake()
{
	Super::HoldHandbrake();
	CarlaPlayerState->bHandBrake = true;
}
void ACarlaVehicleController::ReleaseHandbrake()
{
	Super::ReleaseHandbrake();
	CarlaPlayerState->bHandBrake = false;
}

void ACarlaVehicleController::RelinquishPlayerControl()
{
	// relinquish human control
	UE_LOG(LogCarla, Log, TEXT("Controller relinquishing human control"));

	bRelinquishControlOnStop = false;
	EnableUserInput(false);
	SetAutopilot(true, false);
	bMoveSlow = true;

	CarlaPlayerState->bIsUnderPlayerControl = false;
}

void ACarlaVehicleController::ResumePlayerControl()
{
	Super::ResumePlayerControl();
	CarlaPlayerState->bIsUnderPlayerControl = true;
}

float ACarlaVehicleController::SteeringValue() const
{
	return CarlaPlayerState->Steer;
}

void ACarlaVehicleController::ReloadSettings()
{
	UCarlaGameInstance* gameInstance = Cast<UCarlaGameInstance>(GetWorld()->GetGameInstance());
	auto& CarlaSettings = gameInstance->GetCarlaSettings();
	secondsBeforeAISteering = CarlaSettings.SecondsBeforeAISteering;

	steerSensitivity = CarlaSettings.SteerSensitivity;
	brakeSensitivity = CarlaSettings.BrakeSensitivity;
	throttleSensitivity = CarlaSettings.ThrottleSensitivity;

	// need to convert the quartile to an exponent multiplier - without anything, log(3) is the quartile
	// multiply all by 44.7 cms to 1 mph
	steerExponent = -1 * LOG3 / (CarlaSettings.SteerQuartile * 44.7);
	brakeExponent = -1 * LOG3 / (CarlaSettings.BrakeQuartile * 44.7);
	throttleExponent = -1 * LOG3 / (CarlaSettings.ThrottleQuartile * 44.7);

	steerMidpoint = CarlaSettings.SteerMidpoint * 44.7;
	brakeMidpoint = CarlaSettings.BrakeMidpoint * 44.7;
	throttleMidpoint = CarlaSettings.ThrottleMidpoint * 44.7;
}
