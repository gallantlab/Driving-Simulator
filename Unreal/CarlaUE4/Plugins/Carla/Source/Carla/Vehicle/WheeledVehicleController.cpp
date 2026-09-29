// Copyright (c) 2017 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#include "Carla.h"
#include "WheeledVehicleController.h"
#include "Game/CarlaGameInstance.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/SpringArmComponent.h"

// =============================================================================
// -- Constructor and destructor -----------------------------------------------
// =============================================================================

AWheeledVehicleController::AWheeledVehicleController(const FObjectInitializer& ObjectInitializer) :
  Super(ObjectInitializer)
{
  bAutoManageActiveCameraTarget = false;

  // Create the spring arm component.
  SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm0"));
  SpringArm->TargetOffset = FVector(0.f, 0.f, 200.f);
  SpringArm->SetRelativeRotation(FRotator(-15.f, 0.f, 0.f));
  SpringArm->SetupAttachment(RootComponent);
  SpringArm->TargetArmLength = 650.0f;
  SpringArm->bEnableCameraRotationLag = true;
  SpringArm->CameraRotationLagSpeed = 7.f;
  SpringArm->bInheritPitch = false;
  SpringArm->bInheritRoll = false;
  SpringArm->bInheritYaw = true;

  // Do not collide, may clip into level.
  SpringArm->bDoCollisionTest = false;

  // Create the camera component.
  PlayerCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera0"));
  PlayerCamera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
  PlayerCamera->bUsePawnControlRotation = false;
  PlayerCamera->FieldOfView = 90.f;

  // Create the on-board camera component.
  OnBoardCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("OnBoardCamera0"));
  OnBoardCamera->SetRelativeLocation(FVector(160.f, 0.f, 120.f));
  OnBoardCamera->SetRelativeRotation(FRotator(0.f, 0.f, 0.f));
  OnBoardCamera->SetupAttachment(RootComponent);
  OnBoardCamera->bUsePawnControlRotation = false;
  OnBoardCamera->FieldOfView = 110.f;
}

AWheeledVehicleController::~AWheeledVehicleController() {}

// =============================================================================
// -- AActor -------------------------------------------------------------------
// =============================================================================

void AWheeledVehicleController::BeginPlay()
{
	Super::BeginPlay();
	EnableOnBoardCamera(bOnBoardCameraIsActive, true);
	const UCarlaSettings& settings = Cast<UCarlaGameInstance>(GetGameInstance())->GetCarlaSettings();
	bGovernorActive = settings.bGovernorOnPlayer;
	if (bGovernorActive)
		UE_LOG(LogCarla, Log, TEXT("Player vehicle speed will be limited to poster speed limits"));
	UE_LOG(LogCarla, Log, TEXT("Wheeled vehicle controller beginplay"));
}

// =============================================================================
// -- APlayerController --------------------------------------------------------
// =============================================================================

void AWheeledVehicleController::SetupInputComponent()
{
  Super::SetupInputComponent();
  if (InputComponent != nullptr) {
    // Camera movement.
    InputComponent->BindAxis("CameraZoom", this, &AWheeledVehicleController::ChangeCameraZoom);
    InputComponent->BindAxis("CameraUp", this, &AWheeledVehicleController::ChangeCameraUp);
    InputComponent->BindAxis("CameraRight", this, &AWheeledVehicleController::ChangeCameraRight);
    InputComponent->BindAction("ToggleCamera", IE_Pressed, this, &AWheeledVehicleController::ToggleCamera);
    InputComponent->BindAction("RestartLevel", IE_Pressed, this, &AWheeledVehicleController::RestartLevel);
    // Vehicle movement.
    InputComponent->BindAction("ToggleAutopilot", IE_Pressed, this, &AWheeledVehicleAIController::ToggleAutopilot);
	  InputComponent->BindAction("ResumePlayerControl", IE_Pressed, this, &AWheeledVehicleController::ResumePlayerControl);
    InputComponent->BindAxis("MoveForward", this, &AWheeledVehicleController::SetThrottleInput);
    InputComponent->BindAxis("MoveRight", this, &AWheeledVehicleController::SetSteeringInput);
	  InputComponent->BindAction("ToggleReverse", IE_Pressed, this, &AWheeledVehicleController::ToggleReverse);
    InputComponent->BindAxis("Brake", this, &AWheeledVehicleController::SetBrakeInput);
    InputComponent->BindAction("Handbrake", IE_Pressed, this, &AWheeledVehicleController::HoldHandbrake);
    InputComponent->BindAction("Handbrake", IE_Released, this, &AWheeledVehicleController::ReleaseHandbrake);

	// button inputs
	  InputComponent->BindAction("Button1", IE_Pressed, this, &AWheeledVehicleController::ToggleReverse);
//	  InputComponent->BindAxis("Button2Axis", this, &AWheeledVehicleController::SetBrakeInput);
  }
}

void AWheeledVehicleController::Possess(APawn *aPawn)
{
  Super::Possess(aPawn);
	AttachCamerasTo(aPawn);
}

void AWheeledVehicleController::CalcCamera(float DeltaTime, FMinimalViewInfo& OutResult)
{
  if (bOnBoardCameraIsActive) {
    OnBoardCamera->GetCameraView(DeltaTime, OutResult);
  } else {
    PlayerCamera->GetCameraView(DeltaTime, OutResult);
  }
}

// =============================================================================
// -- User input ---------------------------------------------------------------
// =============================================================================

void AWheeledVehicleController::EnableUserInput(const bool On)
{
  bAllowUserInput = On;
  if (bAllowUserInput) {
    Super::SetAutopilot(false);
  }
}

void AWheeledVehicleController::SetAutopilot(bool Enable, bool ClearPlannedLocations)
{
	UE_LOG(LogCarla, Log, TEXT("WheeledVehicleController set autopilot %s %sclearing planned locations"), Enable ? TEXT("on") : TEXT("off"), ClearPlannedLocations ? TEXT("") : TEXT("not "))
	Super::SetAutopilot(Enable, ClearPlannedLocations);
	bAllowUserInput = !Enable;
}


void AWheeledVehicleController::ResumePlayerControl()
{
	UE_LOG(LogCarla, Log, TEXT("Manually resuming player control"))
	bMoveSlow = false;
	EnableUserInput(true);
}


// =============================================================================
// -- Camera movement methods --------------------------------------------------
// =============================================================================

bool AWheeledVehicleController::AttachCamerasTo(APawn *aPawn)
{
	return SpringArm->AttachToComponent(
			aPawn->GetRootComponent(),
			FAttachmentTransformRules::KeepRelativeTransform) &&
	OnBoardCamera->AttachToComponent(
			aPawn->GetRootComponent(),
			FAttachmentTransformRules::KeepRelativeTransform);
}

void AWheeledVehicleController::AWheeledVehicleController::ChangeCameraZoom(float Value)
{
  SpringArm->TargetArmLength = FMath::Clamp(SpringArm->TargetArmLength + Value, 150.0f, 2e4f);
}

void AWheeledVehicleController::ChangeCameraUp(float Value)
{
  auto Rotation = SpringArm->GetRelativeTransform().Rotator();
  Rotation.Pitch = FMath::Clamp(Rotation.Pitch - Value, -85.0f, 0.0f);
  SpringArm->SetRelativeRotation(Rotation);
}

void AWheeledVehicleController::ChangeCameraRight(float Value)
{
  auto Rotation = SpringArm->GetRelativeTransform().Rotator();
  Rotation.Yaw -= Value;
  SpringArm->SetRelativeRotation(Rotation);
}

void AWheeledVehicleController::EnableOnBoardCamera(const bool bEnable, const bool bForce)
{
  if (bForce || (bOnBoardCameraIsActive != bEnable)) {
    bOnBoardCameraIsActive = bEnable;
    if (bEnable) {
      PlayerCamera->Deactivate();
      OnBoardCamera->Activate();
    } else {
      OnBoardCamera->Deactivate();
      PlayerCamera->Activate();
    }
  }
}

// =============================================================================
// -- Vehicle movement methods -------------------------------------------------
// =============================================================================

void AWheeledVehicleController::SetThrottleInput(const float Value)
{
	if (bAllowUserInput && !IsAutopilotEnabled())
	{
		if (!bGovernorActive) // no hard speed regulation
			GetPossessedVehicle()->SetThrottleInput(Value);
		else
		{
			double currentSpeed = GetPossessedVehicle()->GetVehicleForwardSpeed() * 0.036f;
			float maxAllowableThrottle = currentSpeed >= GetSpeedLimit()- 5 * 1.609 ? 0.70f : 1.0f;	// within 5 MPH of speed limit
			if (currentSpeed >= GetSpeedLimit())
				maxAllowableThrottle = 0.0f;
			GetPossessedVehicle()->SetThrottleInput(Value < maxAllowableThrottle ? Value : maxAllowableThrottle);
		}
	}
}

void AWheeledVehicleController::SetSteeringInput(const float Value)
{
  if (bAllowUserInput && !IsAutopilotEnabled()) {
    GetPossessedVehicle()->SetSteeringInput(Value);
  }
}

void AWheeledVehicleController::SetBrakeInput(const float Value)
{
  if (bAllowUserInput && !IsAutopilotEnabled()) {
    GetPossessedVehicle()->SetBrakeInput(Value);
  }
}

void AWheeledVehicleController::ToggleReverse()
{
  if (bAllowUserInput && !IsAutopilotEnabled()) {
    GetPossessedVehicle()->ToggleReverse();
  }
}

void AWheeledVehicleController::HoldHandbrake()
{
  if (bAllowUserInput && !IsAutopilotEnabled()) {
    GetPossessedVehicle()->HoldHandbrake();
  }
}

void AWheeledVehicleController::ReleaseHandbrake()
{
  if (bAllowUserInput && !IsAutopilotEnabled()) {
    GetPossessedVehicle()->ReleaseHandbrake();
  }
}

// Increments the random seeds on a reset to ensure different but repeatable behaviour
void AWheeledVehicleController::RestartLevel()
{
	ACarlaGameModeBase* gameMode = Cast<ACarlaGameModeBase>(GetWorld()->GetAuthGameMode());
	if (gameMode && gameMode->bIsSettingsMenuActive)
	{
		UE_LOG(LogFMRI, Log, TEXT("Not restarting with settings menu is open"));
		return;
	}

  UCarlaGameInstance* gameInstance = Cast<UCarlaGameInstance>(GetWorld()->GetGameInstance());
  gameInstance->ClearVehicleSpawnerPointer();
  gameInstance->ClearPedestrianSpawnerPointer();
  auto& CarlaSettings = gameInstance->GetCarlaSettings();
  CarlaSettings.SeedVehicles++;
  CarlaSettings.SeedPedestrians++;
  Super::RestartLevel();
}