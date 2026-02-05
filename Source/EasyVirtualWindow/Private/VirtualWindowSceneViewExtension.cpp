// Fill out your copyright notice in the Description page of Project Settings.


#include "VirtualWindowSceneViewExtension.h"

FVirtualWindowSceneViewExtension::FVirtualWindowSceneViewExtension(const FAutoRegister& AutoReg, UWorld* InWorld)
	: FWorldSceneViewExtension(AutoReg, InWorld)
	, WindowPosition(100.0, 0.0, 0.0)
	, WindowWidth(100.0)
	, WindowHeight(100.0)
{
}

void FVirtualWindowSceneViewExtension::SetupViewPoint(APlayerController* Player, FMinimalViewInfo& InViewInfo)
{
	if (!bModifyView)
	{
		return;
	}

	InViewInfo.Location = ViewTransform.GetLocation();
	InViewInfo.Rotation = ViewTransform.GetRotation().Rotator();
}

void FVirtualWindowSceneViewExtension::SetupViewProjectionMatrix(FSceneViewProjectionData& InOutProjectionData)
{
	if (!bModifyView)
	{
		return;
	}

	const FVector WindowPositionVS = FTransform(WindowPosition).GetRelativeTransform(ViewTransform).GetLocation();
	const FMatrix& InProjectionMatrix = InOutProjectionData.ProjectionMatrix;
	const float AspectRatio = InProjectionMatrix.M[0][0] / InProjectionMatrix.M[1][1];
	const float Near = InOutProjectionData.GetNearPlaneFromProjectionMatrix();
	const float Scale = Near / WindowPositionVS.X;
	const float Left = Scale * (WindowPositionVS.Y - WindowWidth / 2.0);
	const float Right = Scale * (WindowPositionVS.Y + WindowWidth / 2.0);
	const float Bottom = Scale * WindowPositionVS.Z;
	const float Top = Scale * (WindowPositionVS.Z + AspectRatio * WindowWidth);

	FMatrix ProjectionMatrix = FMatrix(EForceInit::ForceInitToZero);
	ProjectionMatrix.M[0][0] = 2.0 * Near / (Right - Left);
	ProjectionMatrix.M[2][0] = -(Right + Left) / (Right - Left);
	ProjectionMatrix.M[1][1] = 2.0 * Near / (Top - Bottom);
	ProjectionMatrix.M[2][1] = -(Top + Bottom) / (Top - Bottom);
	ProjectionMatrix.M[3][2] = Near;
	ProjectionMatrix.M[2][3] = 1.0;
	InOutProjectionData.ProjectionMatrix = ProjectionMatrix;

	if (false)
	{
		const FMatrix Matrix = InOutProjectionData.ProjectionMatrix;
		UE_LOG(LogTemp, Log, TEXT("Virtual Window Scene Extension SetupViewProjectionMatrix."));
		UE_LOG(LogTemp, Log, TEXT("[ %.3f, %.3f, %.3f, %.3f"), Matrix.M[0][0], Matrix.M[0][1], Matrix.M[0][2], Matrix.M[0][3]);
		UE_LOG(LogTemp, Log, TEXT("  %.3f, %.3f, %.3f, %.3f"), Matrix.M[1][0], Matrix.M[1][1], Matrix.M[1][2], Matrix.M[1][3]);
		UE_LOG(LogTemp, Log, TEXT("  %.3f, %.3f, %.3f, %.3f"), Matrix.M[2][0], Matrix.M[2][1], Matrix.M[2][2], Matrix.M[2][3]);
		UE_LOG(LogTemp, Log, TEXT("  %.3f, %.3f, %.3f, %.3f]"), Matrix.M[3][0], Matrix.M[3][1], Matrix.M[3][2], Matrix.M[3][3]);
	}
}
