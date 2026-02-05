// Fill out your copyright notice in the Description page of Project Settings.


#include "VirtualWindow.h"
#include "Components/BillboardComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "OSCServer.h"
#include "OSCMessage.h"
#include "VirtualWindowSceneViewExtension.h"
#include "FaceTracker.h"

AVirtualWindow::AVirtualWindow()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	PrimaryActorTick.bCanEverTick = true;

#if WITH_EDITORONLY_DATA
	struct FConstructorStatics
	{
		ConstructorHelpers::FObjectFinderOptional<UTexture2D> SpriteTexture;
		ConstructorHelpers::FObjectFinderOptional<UStaticMesh> PlaneMesh;
		FConstructorStatics() : SpriteTexture(TEXT("/Engine/EditorResources/EmptyActor"))
			, PlaneMesh(TEXT("/Engine/BasicShapes/Plane"))
		{}
	};
	static FConstructorStatics ConstructorStatics;

	SpriteComponent = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Sprite"));
	if (SpriteComponent)
	{
		SpriteComponent->Sprite = ConstructorStatics.SpriteTexture.Get();
		SpriteComponent->SetRelativeScale3D(FVector(0.1f, 0.1f, 0.1f));
		SpriteComponent->bHiddenInGame = true;
		SpriteComponent->bIsScreenSizeScaled = true;
		SpriteComponent->SetUsingAbsoluteScale(true);
		SpriteComponent->SetupAttachment(RootComponent);
	}

	PlaneComponent = CreateEditorOnlyDefaultSubobject<UStaticMeshComponent>(TEXT("WindowPlane"));
	if (PlaneComponent)
	{
		PlaneComponent->SetStaticMesh(ConstructorStatics.PlaneMesh.Get());
		PlaneComponent->bHiddenInGame = true;
		PlaneComponent->SetupAttachment(RootComponent);
	}
#endif
}

void AVirtualWindow::BeginPlay()
{
	Super::BeginPlay();

	VirtualWindowExtension = FVirtualWindowSceneViewExtension::GetExtension(GetWorld());
	VirtualWindowExtension->SetWindowPositionAndSize(RootComponent->GetComponentLocation(), WindowWidth / RealVirtualRate, WindowHeight / RealVirtualRate);

	TArray<AActor*> FoundActors{};
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AFaceTracker::StaticClass(), FoundActors);
	for (AActor* Actor : FoundActors)
	{
		FaceTrackers.Push(Cast<AFaceTracker>(Actor));
	}

	OSCServer = NewObject<UOSCServer>(this);
	const int32 Port = 11125;
	bool bSuccess = OSCServer->SetAddress(TEXT("0.0.0.0"), Port);
	if (!bSuccess)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to oscserver setaddress"));
		return;
	}

	OSCServer->OnOscMessageReceived.AddDynamic(this, &AVirtualWindow::OnOSCMessageReceived);
	OSCServer->Listen();
}

void AVirtualWindow::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (OSCServer) {
		OSCServer->Stop();
		OSCServer = nullptr;
	}

	VirtualWindowExtension = nullptr;

	Super::EndPlay(EndPlayReason);
}

void AVirtualWindow::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!VirtualWindowExtension.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("VirtualWindowExtension Not Activated."));
		return;
	}

	FVector EyePosition{};
	bool bTracked{};
	CalcEyePosition(EyePosition, bTracked);
	if (!bTracked)
	{
		return;
	}

	const FTransform ViewTransform = FTransform(RootComponent->GetComponentQuat(), EyePosition);
	VirtualWindowExtension->SetViewTransform(ViewTransform);
	VirtualWindowExtension->EnableModifyView(bEnable);
}

void AVirtualWindow::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

#if WITH_EDITORONLY_DATA
	if (PlaneComponent)
	{
		FQuat PlaneRotation = FRotator(90.0, 0.0, 0.0).Quaternion();
		FVector PlaneTranslation = FVector(0.0, 0.0, WindowHeight / 2.0);
		FVector PlaneScale = FVector(WindowHeight / 100.0, WindowWidth / 100.0, 1.0);
		PlaneComponent->SetRelativeTransform(FTransform(PlaneRotation, PlaneTranslation, PlaneScale));
	}
#endif
}

void AVirtualWindow::CalcEyePosition(FVector& OutEyePosition, bool& bOutTracked)
{
	FScopeLock Lock(&Mutex);

	bOutTracked = false;

	FVector Weights{};
	for (const auto& FaceTracker : FaceTrackers)
	{
		if (!FaceTracker->bTracked)
		{
			continue;
		}

		const FVector Weight = FVector::One();
		OutEyePosition += Weight * FaceTracker->GetWorldEyePosition();
		Weights += Weight;
		bOutTracked = true;
	}

	if (!bOutTracked)
	{
		return;
	}

	OutEyePosition.X /= Weights.X;
	OutEyePosition.Y /= Weights.Y;
	OutEyePosition.Z /= Weights.Z;
}

void AVirtualWindow::OnOSCMessageReceived(const FOSCMessage& Message, const FString& IPAddress, int32 Port)
{
	const FString DeviceId = Message.GetAddress().GetContainer(1);
	TObjectPtr<AFaceTracker>* FaceTracker = FaceTrackers.FindByPredicate([&](TObjectPtr<AFaceTracker>& Tracker) {return Tracker->DeviceId == DeviceId; });
	if (!FaceTracker)
	{
		UE_LOG(LogTemp, Warning, TEXT("FaceTracker#%s is not spawned"), *DeviceId);
		return;
	}

	const FString Method = Message.GetAddress().GetMethod();
	const auto& Data = Message.GetArgumentsChecked();

	if (Method == TEXT("eye-position")) {
		FScopeLock Lock(&Mutex);
		const FVector EyePosition = FVector(Data[0].GetFloat(), Data[1].GetFloat(), Data[2].GetFloat());
		(*FaceTracker)->SetEyePosition(EyePosition);
	}
	else if (Method == TEXT("lost")) {
		FScopeLock Lock(&Mutex);
		(*FaceTracker)->SetTrackState(false);
	}
	else if (Method == TEXT("gravity")) {
		FScopeLock Lock(&Mutex);
		const FVector Gravity = FVector(Data[0].GetFloat(), Data[1].GetFloat(), Data[2].GetFloat());
		(*FaceTracker)->SetGravity(Gravity);
	}
	else {
		UE_LOG(LogTemp, Warning, TEXT("Unknown OSC Message %s"), *Method);
	}
}

