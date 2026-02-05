// Fill out your copyright notice in the Description page of Project Settings.


#include "FaceTracker.h"
#include "Components/BillboardComponent.h"

AFaceTracker::AFaceTracker()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	PrimaryActorTick.bCanEverTick = false;

#if WITH_EDITORONLY_DATA
	struct FConstructorStatics
	{
		ConstructorHelpers::FObjectFinderOptional<UTexture2D> SpriteTexture;
		FConstructorStatics() : SpriteTexture(TEXT("/Engine/EditorResources/EmptyActor")) {}
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
#endif
}

void AFaceTracker::SetGravity(const FVector& InGravity)
{
	FRotator Rotation = RootComponent->GetComponentRotation();
	Rotation.Roll = FMath::RadiansToDegrees(FMath::Atan2(-InGravity.Y, InGravity.Z));
	Rotation.Pitch = FMath::RadiansToDegrees(FMath::Atan2(InGravity.X, FMath::Sqrt(InGravity.Y * InGravity.Y + InGravity.Z * InGravity.Z)));

	const FVector Location = RootComponent->GetComponentLocation();
	const FTransform NewTransform = FTransform(Rotation, Location);
	RootComponent->SetWorldTransform(NewTransform);
}

void AFaceTracker::SetEyePosition(const FVector& InPosition)
{
	EyePosition = RootComponent->GetComponentTransform().TransformPosition(InPosition);
	bTracked = true;
}

void AFaceTracker::SetTrackState(bool bInTracked)
{
	bTracked = bInTracked;
}

FVector AFaceTracker::GetWorldEyePosition() const
{
	return EyePosition;
}
