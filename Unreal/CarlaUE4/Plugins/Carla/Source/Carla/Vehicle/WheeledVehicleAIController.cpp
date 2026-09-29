// Copyright (c) 2017 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#include "Carla.h"
#include "WheeledVehicleAIController.h"

#include "MapGen/RoadMap.h"
#include "Vehicle/CarlaWheeledVehicle.h"

#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "WheeledVehicleMovementComponent.h"

#include <algorithm>

// =============================================================================
// -- Static local methods -----------------------------------------------------
// =============================================================================

static bool RayTrace(const AActor &Actor, const FVector &Start, const FVector &End) {
  FHitResult OutHit;
  static FName TraceTag = FName(TEXT("VehicleTrace"));
  FCollisionQueryParams CollisionParams(TraceTag, true);
  CollisionParams.AddIgnoredActor(&Actor);

  const bool Success = Actor.GetWorld()->LineTraceSingleByObjectType(
        OutHit,
        Start,
        End,
        FCollisionObjectQueryParams(FCollisionObjectQueryParams::AllDynamicObjects),
        CollisionParams);

  return Success && OutHit.bBlockingHit;
}

static bool IsThereAnObstacleAhead(
    const ACarlaWheeledVehicle &Vehicle,
    const float Speed,
    const FVector &Direction)
{
  const auto ForwardVector = Vehicle.GetVehicleOrientation();
  const auto VehicleBounds = Vehicle.GetVehicleBoundingBoxExtent();

  const float Distance = (std::max)(50.0f, Speed * Speed); // why?

  const FVector StartCenter = Vehicle.GetActorLocation() + (ForwardVector * (250.0f + VehicleBounds.X / 2.0f)) + FVector(0.0f, 0.0f, 50.0f);
  const FVector EndCenter = StartCenter + Direction * (Distance + VehicleBounds.X / 2.0f);

  const FVector StartRight = StartCenter + (FVector(ForwardVector.Y, -ForwardVector.X, ForwardVector.Z) * 100.0f);
  const FVector EndRight = StartRight + Direction * (Distance + VehicleBounds.X / 2.0f);

  const FVector StartLeft = StartCenter + (FVector(-ForwardVector.Y, ForwardVector.X, ForwardVector.Z) * 100.0f);
  const FVector EndLeft = StartLeft + Direction * (Distance + VehicleBounds.X / 2.0f);

  return
      RayTrace(Vehicle, StartCenter, EndCenter) ||
      RayTrace(Vehicle, StartRight, EndRight) ||
      RayTrace(Vehicle, StartLeft, EndLeft);
}

template <typename T>
static void ClearQueue(std::queue<T> &Queue)
{
  std::queue<T> EmptyQueue;
  Queue.swap(EmptyQueue);
}

// =============================================================================
// -- Constructor and destructor -----------------------------------------------
// =============================================================================

AWheeledVehicleAIController::AWheeledVehicleAIController(const FObjectInitializer& ObjectInitializer) :
  Super(ObjectInitializer)
{
  RandomEngine = CreateDefaultSubobject<URandomEngine>(TEXT("RandomEngine"));

  PrimaryActorTick.bCanEverTick = true;
  PrimaryActorTick.TickGroup = TG_PrePhysics;
  intersections = TArray<ASpeedZoneBase*>();

	bAlwaysRelevant = true;
}

AWheeledVehicleAIController::~AWheeledVehicleAIController() {}

// =============================================================================
// -- APlayerController --------------------------------------------------------
// =============================================================================

void AWheeledVehicleAIController::Possess(APawn *aPawn)
{
  Super::Possess(aPawn);

  if (IsPossessingAVehicle()) {
    UE_LOG(LogCarla, Error, TEXT("Controller already possessing a vehicle!"));
    return;
  }
  Vehicle = Cast<ACarlaWheeledVehicle>(aPawn);
  check(Vehicle != nullptr);
  MaximumSteerAngle = Vehicle->GetMaximumSteerAngle();
  check(MaximumSteerAngle > 0.0f);
  ConfigureAutopilot(bAutopilotEnabled);

  // if this vehicle spawned in a speed zone, set the correct speed limit
	for (TActorIterator <ASpeedZoneBase> speedZone(GetWorld()); speedZone; ++speedZone)
	{
		if (!speedZone->bIsIntersection && Vehicle->IsOverlappingActor(*speedZone))
		{
			CurrentSpeedLimit = speedZone->speedLimit;
//			UE_LOG(LogCarla, Log, TEXT("Vehicle spawned into speed zone with limit %.0f"), CurrentSpeedLimit / 1.609);
		}
	}

  // tag the vehicle as the player
  TagPawn();
}

void AWheeledVehicleAIController::BeginPlay()
{
	Super::BeginPlay();

	const UCarlaSettings& settings = Cast<UCarlaGameInstance>(GetGameInstance())->GetCarlaSettings();

	DefaultSpeedLimit = settings.defaultSpeedLimit * 1.609;	// mph to kmh conversion
	if (CurrentSpeedLimit <= 0)		// begin play happens after possession
		CurrentSpeedLimit = DefaultSpeedLimit;

	impatience = settings.reincarnation;
	maxOffScreenTime = settings.offScreenLiveTime;

	// search for intersections
    for (TActorIterator <ASpeedZoneBase> speedZone(GetWorld()); speedZone; ++speedZone)
    {
    	if (speedZone->bIsIntersection)
    		intersections.Add(*speedZone);
    }

	UE_LOG(LogCarla, Log, TEXT("Controller begin play with speed limit %.2f km/h and %d intersections"), CurrentSpeedLimit, intersections.Num());
}

void AWheeledVehicleAIController::TagPawn()
{
  auto thisPawn = GetPawnOrSpectator();
//	UE_LOG(LogCarla, Log, TEXT("Controller %s, %s replicated"), *GetName(), GetIsReplicated() ? TEXT("is") : TEXT("isn't"));
//UE_LOG(LogCarla, Log, TEXT("Pawn %s, %s replicated"), *thisPawn->GetName(), thisPawn->GetIsReplicated() ? TEXT("is") : TEXT("isn't"));
  if (IsPossessingThePlayer())
  {
    thisPawn->Tags.Add(PlayerTag);
//    UE_LOG(LogCarla, Log, TEXT("Player pawn tagged"));
  }
  else
  {
    thisPawn->Tags.Add(NPCTag);
//    UE_LOG(LogCarla, Log, TEXT("NPC pawn tagged"));
  }
}

void AWheeledVehicleAIController::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);

	TickAutopilotController(DeltaTime);

	if (bAutopilotEnabled)
	{
		Vehicle->ApplyVehicleControl(AutopilotControl);
	}
}

// =============================================================================
// -- Autopilot ----------------------------------------------------------------
// =============================================================================

void AWheeledVehicleAIController::ConfigureAutopilot(const bool Enable, const bool ClearPlannedLocations)
{
  bAutopilotEnabled = Enable;
  // Reset state.
  Vehicle->SetSteeringInput(0.0f);
  Vehicle->SetThrottleInput(0.0f);
  Vehicle->SetBrakeInput(0.0f);
  Vehicle->SetReverse(false);
  Vehicle->SetHandbrakeInput(false);
  TrafficLightState = ETrafficLightState::Green;
  if (ClearPlannedLocations) ClearQueue(TargetLocations);
  Vehicle->SetAIVehicleState(
      bAutopilotEnabled ?
          ECarlaWheeledVehicleState::FreeDriving :
          ECarlaWheeledVehicleState::AutopilotOff);
}

// =============================================================================
// -- Traffic ------------------------------------------------------------------
// =============================================================================

void AWheeledVehicleAIController::SetFixedRoute(const TArray <FVector>& Locations, const bool bOverwriteCurrent,
												int newDirection)
{
	if (bOverwriteCurrent)
	{
		ClearQueue(TargetLocations);
	}
	for (auto& Location : Locations)
	{
		TargetLocations.emplace(Location);
	}
//	UE_LOG(LogCarla, Log, TEXT("Fixed route set in controller"));
}

// =============================================================================
// -- AI -----------------------------------------------------------------------
// =============================================================================

void AWheeledVehicleAIController::TickAutopilotController(float deltaTime)
{
#if WITH_EDITOR
  if (Vehicle == nullptr) { // This happens in simulation mode in editor.
    bAutopilotEnabled = false;
    return;
  }
#endif // WITH_EDITOR

  check(Vehicle != nullptr);

  if (RoadMap == nullptr) {
    UE_LOG(LogCarla, Error, TEXT("Controller doesn't have a road map!"));
    return;
  }

  FVector Direction;

  float Steering;
	if (!TargetLocations.empty())
	{
		Steering = GoToNextTargetLocation(Direction);
	}
	else
	{
		Steering = CalcSteeringValue(Direction);
	}

  	// Speed in km/h.
	const double Speed = Vehicle->GetVehicleForwardSpeed() * 0.036f;

	// if not really moving, start counting
	// if not possessing the place
	if (!IsPossessingThePlayer())
	{
		// note: both this class (the AI controller) and the CarlaVehicleController (the player controller)
		// make use of secondsAtRest. Because this statement is gated inside IsPossessingThePlayer,
		// secondsAtRest should not be incremented twice in each tick.
		// this is required here to despawn vehicles in case of non-movement
		if (Speed < 0.1)
			secondsAtRest += deltaTime;
		else
			secondsAtRest = 0;

		// don't unspawn at start
		if (liveTime <= maxOffScreenTime)
			liveTime += deltaTime;

		// if waiting for too long, mark self for destruction
		// or if has not been rendered for a while
		if (secondsAtRest > impatience)
			DespawnVehicle(EDespawnReason::NotMoving);
		else if (!Vehicle->WasRecentlyRendered(maxOffScreenTime) && liveTime >= maxOffScreenTime)
			DespawnVehicle(EDespawnReason::OffScreen);
	}

	float Throttle;
	if (TrafficLightState != ETrafficLightState::Green)
	{
		Vehicle->SetAIVehicleState(ECarlaWheeledVehicleState::WaitingForRedLight);
		Throttle = Stop(Speed);
	}
	else if (IsThereAnObstacleAhead(*Vehicle, Speed, Direction))
	{
		Vehicle->SetAIVehicleState(ECarlaWheeledVehicleState::ObstacleAhead);
		Throttle = Stop(Speed);
	}
	else
	{
		Throttle = Move(Speed);
	}

  if (Throttle < 0.001f) {
    AutopilotControl.Brake = 1.0f;
    AutopilotControl.Throttle = 0.0f;
  } else {
    AutopilotControl.Brake = 0.0f;
    AutopilotControl.Throttle = Throttle;
  }
  AutopilotControl.Steer = Steering;
}

float AWheeledVehicleAIController::GoToNextTargetLocation(FVector &Direction)
{
  // Get middle point between the two front wheels.
  const auto CurrentLocation = [&](){
    const auto &Wheels = Vehicle->GetVehicleMovementComponent()->Wheels;
    check((Wheels.Num() > 1) && (Wheels[0u] != nullptr) && (Wheels[1u] != nullptr));
    return (Wheels[0u]->Location + Wheels[1u]->Location) / 2.0f;
  }();

  const auto Target = [&](){
    const auto &Result = TargetLocations.front();
    return FVector{Result.X, Result.Y, CurrentLocation.Z};
  }();

	if (Target.Equals(CurrentLocation, 80.0f))
	{
		TargetLocations.pop();
		if (!TargetLocations.empty())
		{
			return GoToNextTargetLocation(Direction);
		}
		else
		{
//			UE_LOG(LogCarla, Log, TEXT("Path following finished"));
			OnFixedRouteFinished();
			return CalcSteeringValue(Direction);
		}
	}

  Direction = (Target - CurrentLocation).GetSafeNormal();

  const FVector &Forward = GetPawn()->GetActorForwardVector();

  float dirAngle = Direction.UnitCartesianToSpherical().Y;
  float actorAngle = Forward.UnitCartesianToSpherical().Y;

  dirAngle *= (180.0f / PI);
  actorAngle *= (180.0 / PI);

  float angle = dirAngle - actorAngle;

  if (angle > 180.0f) { angle -= 360.0f;} else if (angle < -180.0f) {
    angle += 360.0f;
  }

  float Steering = 0.0f;
  if (angle < -MaximumSteerAngle) {
    Steering = -1.0f;
  } else if (angle > MaximumSteerAngle) {
    Steering = 1.0f;
  } else {
    Steering += angle / MaximumSteerAngle;
  }

  Vehicle->SetAIVehicleState(ECarlaWheeledVehicleState::FollowingFixedRoute);
  return Steering;
}

float AWheeledVehicleAIController::CalcSteeringValue(FVector &direction)
{
  float steering = 0;
  FVector BoxExtent = Vehicle->GetVehicleBoundingBoxExtent();
  FVector forward = Vehicle->GetActorForwardVector();

  FVector rightSensorPosition(BoxExtent.X / 2.0f, (BoxExtent.Y / 2.0f) + 100.0f, 0.0f);
  FVector leftSensorPosition(BoxExtent.X / 2.0f, -(BoxExtent.Y / 2.0f) - 100.0f, 0.0f);

  float forwardMagnitude = BoxExtent.X / 2.0f;

  float Magnitude = (float) sqrt(pow((double) leftSensorPosition.X, 2.0) + pow((double) leftSensorPosition.Y, 2.0));

  //same for the right and left
  float offset = FGenericPlatformMath::Acos(forwardMagnitude / Magnitude);

  float actorAngle = forward.UnitCartesianToSpherical().Y;

  float sinR = FGenericPlatformMath::Sin(actorAngle + offset);
  float cosR = FGenericPlatformMath::Cos(actorAngle + offset);

  float sinL = FGenericPlatformMath::Sin(actorAngle - offset);
  float cosL = FGenericPlatformMath::Cos(actorAngle - offset);

  rightSensorPosition.Y = sinR * Magnitude;
  rightSensorPosition.X = cosR * Magnitude;

  leftSensorPosition.Y = sinL * Magnitude;
  leftSensorPosition.X = cosL * Magnitude;

  FVector rightPositon = GetPawn()->GetActorLocation() + FVector(rightSensorPosition.X, rightSensorPosition.Y, 0.0f);
  FVector leftPosition = GetPawn()->GetActorLocation() + FVector(leftSensorPosition.X, leftSensorPosition.Y, 0.0f);

  FRoadMapPixelData rightRoadData = RoadMap->GetDataAt(rightPositon);
  if (!rightRoadData.IsRoad()) { steering -= 0.2f;}

  FRoadMapPixelData leftRoadData = RoadMap->GetDataAt(leftPosition);
  if (!leftRoadData.IsRoad()) { steering += 0.2f;}

  FRoadMapPixelData roadData = RoadMap->GetDataAt(GetPawn()->GetActorLocation());
  if (!roadData.IsRoad()) {
    steering = -1;
  } else if (roadData.HasDirection()) {

    direction = roadData.GetDirection();
    FVector right = rightRoadData.GetDirection();
    FVector left = leftRoadData.GetDirection();

    forward.Z = 0.0f;

    float dirAngle = direction.UnitCartesianToSpherical().Y;
    float rightAngle = right.UnitCartesianToSpherical().Y;
    float leftAngle = left.UnitCartesianToSpherical().Y;

    dirAngle *= (180.0f / PI);
    rightAngle *= (180.0 / PI);
    leftAngle *= (180.0 / PI);
    actorAngle *= (180.0 / PI);

    float min = dirAngle - 90.0f;
    if (min < -180.0f) { min = 180.0f + (min + 180.0f);}

    float max = dirAngle + 90.0f;
    if (max > 180.0f) { max = -180.0f + (max - 180.0f);}

    if (dirAngle < -90.0 || dirAngle > 90.0) {
      if (rightAngle < min && rightAngle > max) { steering -= 0.2f;}
      if (leftAngle < min && leftAngle > max) { steering += 0.2f;}
    } else {
      if (rightAngle < min || rightAngle > max) { steering -= 0.2f;}
      if (leftAngle < min || leftAngle > max) { steering += 0.2f;}
    }

    float angle = dirAngle - actorAngle;

    if (angle > 180.0f) { angle -= 360.0f;} else if (angle < -180.0f) {
      angle += 360.0f;
    }

    if (angle < -MaximumSteerAngle) {
      steering = -1.0f;
    } else if (angle > MaximumSteerAngle) {
      steering = 1.0f;
    } else {
      steering += angle / MaximumSteerAngle;
    }
  }

  Vehicle->SetAIVehicleState(ECarlaWheeledVehicleState::FreeDriving);
  return steering;
}

float AWheeledVehicleAIController::Stop(const float Speed) {
  return (Speed >= 1.0f ? -Speed / GetCurrentMaxAllowedSpeed() : 0.0f);
}

float AWheeledVehicleAIController::Move(const float Speed) {
  if (Speed >= GetCurrentMaxAllowedSpeed()) {
    return Stop(Speed);
  } else if (Speed >= GetCurrentMaxAllowedSpeed() - 10.0f) {
    return 0.5f;
  } else {
    return 1.0f;
  }
}


bool AWheeledVehicleAIController::IsVehicleInIntersection() const
{
	if (Vehicle)
	{
		for (int i = 0; i < intersections.Num(); i++)
			if (Vehicle->IsOverlappingActor(intersections[i]))
				return true;
	}
	return false;
}


void AWheeledVehicleAIController::DespawnVehicle(EDespawnReason reason)
{
	UE_LOG(LogFMRI, Log, TEXT("Controller id %d is despawning vehicle id %d "), GetUniqueID(), Vehicle->GetUniqueID());
	switch (reason)
	{
		case EDespawnReason::NotMoving:
			UE_LOG(LogFMRI, Log, TEXT("This vehicle has not moved in %d seconds"), (int)impatience);
			break;
		case EDespawnReason::OffScreen:
			UE_LOG(LogFMRI, Log, TEXT("This vehicle has not been rendered in %d seconds"), (int)maxOffScreenTime);
			break;
		case EDespawnReason::DespawnTrigger:
			UE_LOG(LogFMRI, Log, TEXT("This vehicle encountered a despawn trigger"));
			break;
	}

	Cast<UCarlaGameInstance>(GetGameInstance())->AddOneVehicleToRespawnQueue(Vehicle);	// queue reincarnation up
	this->UnPossess();
	this->Destroy();
}