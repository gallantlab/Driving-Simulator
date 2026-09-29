// Copyright (c) 2017 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#include "Carla.h"
#include "SceneCaptureToDiskCamera.h"

#include "HighResScreenshot.h"
#include "Paths.h"

//#define VERBOSE_LOG

ASceneCaptureToDiskCamera::ASceneCaptureToDiskCamera(const FObjectInitializer& ObjectInitializer) :
		Super(ObjectInitializer),
		SaveToFolder(*FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("SceneCaptures"), GetName())),
		FileName("capture_%06d.png")
{}

void ASceneCaptureToDiskCamera::BeginPlay()
{
	Super::BeginPlay();

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 1.0f / CapturesPerSecond;

	CaptureFileNameCount = 0u;
}

void ASceneCaptureToDiskCamera::SetSaveFolder(const FString& folder)
{
	SaveToFolder = *FPaths::Combine(FPaths::ProjectSavedDir(), folder, GetName());
	UE_LOG(LogCarla, Log, TEXT("Save folder set to %s"), *SaveToFolder);
}

void ASceneCaptureToDiskCamera::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bCaptureScene)
	{
		const FString FilePath = FPaths::Combine(*SaveToFolder, FString::Printf(*FileName, CaptureFileNameCount));
#ifdef VERBOSE_LOG
		UE_LOG(LogCarla, Log, TEXT("DeltaTime %fs: Capture %s"), DeltaTime, *FilePath);
#endif
		SaveCaptureToDisk(FilePath);
		++CaptureFileNameCount;
	}
}

bool ASceneCaptureToDiskCamera::SaveCaptureToDisk(const FString& FilePath) const
{
	TArray <FColor> OutBMP;
	if (!ReadPixels(OutBMP))
		return false;
	for (FColor& color : OutBMP)
		color.A = 255;
	const FIntPoint DestSize(GetImageSizeX(), GetImageSizeY());
#ifdef VERBOSE_LOG
	UE_LOG(LogCarla, Log, TEXT("Saving image size %d x %d"), GetImageSizeX(), GetImageSizeY());
	UE_LOG(LogCarla, Log, TEXT("Image data size %d"), OutBMP.Num());
#endif
	FString ResultPath;
	FHighResScreenshotConfig& HighResScreenshotConfig = GetHighResScreenshotConfig();
	return HighResScreenshotConfig.SaveImage(FilePath, OutBMP, DestSize, &ResultPath);
}

bool ASceneCaptureToDiskCamera::SaveNextFrame()
{
	const FString FilePath = FPaths::Combine(*SaveToFolder, FString::Printf(*FileName, CaptureFileNameCount));
#ifdef VERBOSE_LOG
	UE_LOG(LogCarla, Log, TEXT("Camera %s saving frame %d"), *this->GetName(), CaptureFileNameCount);
#endif
	CaptureFileNameCount++;
	return SaveCaptureToDisk(FilePath);
}