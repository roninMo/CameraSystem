// Fill out your copyright notice in the Description page of Project Settings.


#include "CameraComponents/BasePlayerCameraManager.h"

#include "Character/CharacterCameraLogic.h"
#include "Camera/CameraComponent.h"
#include "Camera/CameraActor.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

ABasePlayerCameraManager::ABasePlayerCameraManager(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bAlwaysApplyModifiers = true; // TODO: Investigate this
	PivotLagSpeed = FVector(3.4);

	// Camera values
	CameraOrientation = ECameraOrientation::Center;
	CameraStyle = CameraStyle_ThirdPerson;
	CrouchBlendDuration = 0.5;
	RotationLagSpeed = 6.4;
	OutOfBoundsLagSpeed = 43.0;
}


void ABasePlayerCameraManager::UpdateViewTarget(FTViewTarget& OutVT, float DeltaTime)
{
	// Don't update outgoing view target during an interpolation 
	// Based on the camera style update the view target information in prep for calcCamera/BlueprintUpdateCamera
	if ((PendingViewTarget.Target != NULL) && BlendParams.bLockOutgoing && OutVT.Equal(ViewTarget))
	{
		return;
	}

	// Update the character information
	Character = Character ? Character : Cast<ACharacterCameraLogic>(OutVT.Target);
	if (Character)
	{
		CameraStyle = Character->Execute_GetCameraStyle(Character);
		CameraOrientation = Character->Execute_GetCameraOrientation(Character);
	}

	PreviousView = OutVT.POV;
	bool bApplyModifiers = false;
	
	OutVT.POV.FOV = DefaultFOV;
	OutVT.POV.OrthoWidth = DefaultOrthoWidth;
	OutVT.POV.AspectRatio = DefaultAspectRatio;
	OutVT.POV.bConstrainAspectRatio = bDefaultConstrainAspectRatio;
	OutVT.POV.bUseFieldOfViewForLOD = true;
	OutVT.POV.ProjectionMode = bIsOrthographic ? ECameraProjectionMode::Orthographic : ECameraProjectionMode::Perspective;
	OutVT.POV.PostProcessSettings.SetBaseValues();
	OutVT.POV.PostProcessBlendWeight = 1.0f;
	OutVT.POV.PreviousViewTransform.Reset();
	
	if (const ACameraActor* CamActor = Cast<ACameraActor>(OutVT.Target))
	{
		// Viewing through a fixed camera actor.
		CamActor->GetCameraComponent()->GetCameraView(DeltaTime, OutVT.POV);
	}
	else
	{
		// If there's blueprint logic that takes precedence, use that instead
		FVector OutLocation;
		FRotator OutRotation;
		float OutFOV;
		 
		if (OutVT.Target && BlueprintUpdateCamera(OutVT.Target, OutLocation, OutRotation, OutFOV))
		{
			OutVT.POV.Location = OutLocation;
			OutVT.POV.Rotation = OutRotation;
			OutVT.POV.FOV = OutFOV;
			bApplyModifiers = true;
		}
		else if (CameraStyle == CameraStyle_FirstPerson)
		{
			FirstPersonCameraBehavior(DeltaTime, OutVT);
			bApplyModifiers = true;
		}
		else if (CameraStyle == CameraStyle_ThirdPerson)
		{
			ThirdPersonCameraBehavior(DeltaTime, OutVT);
		}
		else if (CameraStyle == CameraStyle_TargetLocking)
		{
			TargetLockCameraBehavior(DeltaTime, OutVT);
		}
		else if (CameraStyle == CameraStyle_Aiming)
		{
			ThirdPersonAimingCameraBehavior(DeltaTime, OutVT);
		}
		else if (CameraStyle == CameraStyle_Spectator)
		{
			SpectatorCameraBehavior(DeltaTime, OutVT);
			bApplyModifiers = true;
		}
		else if (CameraStyle == CameraStyle_Fixed)
		{
			// do not update, keep previous camera position by restoring
			// saved POV, in case CalcCamera changes it but still returns false
			OutVT.POV = PreviousView;
		}
		else
		{
			BP_UpdateViewTarget(OutVT, DeltaTime, bApplyModifiers);
		}

	}

	if (bApplyModifiers || bAlwaysApplyModifiers)
	{
		// Apply camera modifiers at the end (view shakes for example)
		ApplyCameraModifiers(DeltaTime, OutVT.POV);
	}

	// Synchronize the actor with the view target results // TODO: Why is the camera affecting the actor location, and is this causing net corrections?
	SetActorLocationAndRotation(OutVT.POV.Location, OutVT.POV.Rotation, false);

	UpdateCameraLensEffects(OutVT);
}


#pragma region Camera behaviors
void ABasePlayerCameraManager::FirstPersonCameraBehavior_Implementation(float DeltaTime, FTViewTarget& OutVT)
{
	// We're still going to use the camera calculations, the only difference is the spring arm length and we disable the rotation lag that's added to the third person camera
	UpdateViewTargetInternal(OutVT, DeltaTime);
	
	// Search for a camera location for this character and get it's information. Otherwise use the actors view point
	// While transitioning to first person you need to hide the character, just handle this during camera transition logic (OnCameraStyleSet) 
	// const FName CameraSocketName = GetCameraSocketName(); // <- I think having offsets that fix the arm is easier because camera arm transition smoothing are also added
	// if (!CameraSocketName.IsNone() && Character && Character->GetMesh())
	// {
	// 	const FTransform CameraLocation = Character->GetMesh()->GetSocketTransform(CameraSocketName);
	// 	OutVT.POV.Location = CameraLocation.GetLocation();
	// 	if (PCOwner)
	// 	{
	// 		OutVT.POV.Rotation = PCOwner->GetControlRotation();
	// 	}
	// }
	// else
	// {
	// 	// Simple first person, view through viewtarget's 'eyes'
	// 	OutVT.Target->GetActorEyesViewPoint(OutVT.POV.Location, OutVT.POV.Rotation);
	// }
}


void ABasePlayerCameraManager::ThirdPersonCameraBehavior_Implementation(float DeltaTime, FTViewTarget& OutVT)
{
	UpdateViewTargetInternal(OutVT, DeltaTime);
	// Target Lock logic for third person is tied to the spring arm component
}



void ABasePlayerCameraManager::ThirdPersonAimingCameraBehavior_Implementation(float DeltaTime, FTViewTarget& OutVT)
{
	UpdateViewTargetInternal(OutVT, DeltaTime);
	// Center the character's camera to the specific weapon they're aiming with
}


void ABasePlayerCameraManager::TargetLockCameraBehavior_Implementation(float DeltaTime, FTViewTarget& OutVT)
{
	UpdateViewTargetInternal(OutVT, DeltaTime);
	// Target lock behavior is handled during the Camera Arm's update logic to handle smoothing and transitions properly
}


void ABasePlayerCameraManager::SpectatorCameraBehavior_Implementation(float DeltaTime, FTViewTarget& OutVT)
{
	UpdateViewTargetInternal(OutVT, DeltaTime);
}


void ABasePlayerCameraManager::BP_UpdateViewTarget_Implementation(FTViewTarget& OutVT, float DeltaTime, bool& bApplyModifiers)
{
	UpdateViewTargetInternal(OutVT, DeltaTime);
}
#pragma endregion 


FVector ABasePlayerCameraManager::CalculateCameraDrag(FVector Current, FVector Target, FRotator CameraRotation, float DeltaTime)
{
	CameraRotation.Pitch = 0.0f;
	CameraRotation.Roll = 0.0f;

	const FQuat CameraRotationQuaternion = CameraRotation.Quaternion();
	const FVector UnRotatedCurrentLocation = UKismetMathLibrary::Quat_UnrotateVector(CameraRotationQuaternion, Current);
	const FVector UnRotatedTargetLocation = UKismetMathLibrary::Quat_UnrotateVector(CameraRotationQuaternion, Target);

	// TODO: Adjust the pivot lag speed to handle faster speeds
	const FVector CameraDragLocation = FVector(
		UKismetMathLibrary::FInterpTo(UnRotatedCurrentLocation.X, UnRotatedTargetLocation.X, DeltaTime, PivotLagSpeed.X),
		UKismetMathLibrary::FInterpTo(UnRotatedCurrentLocation.Y, UnRotatedTargetLocation.Y, DeltaTime, PivotLagSpeed.Y),
		UKismetMathLibrary::FInterpTo(UnRotatedCurrentLocation.Z, UnRotatedTargetLocation.Z, DeltaTime, PivotLagSpeed.Z)
	);

	UKismetMathLibrary::Quat_RotateVector(CameraRotationQuaternion, CameraDragLocation);
	return CameraDragLocation;
}


FName ABasePlayerCameraManager::GetCameraSocketName()
{
	if (CameraStyle == CameraStyle_FirstPerson) return CameraSocketFirstPerson;
	return CameraSocketThirdPerson;
}


void ABasePlayerCameraManager::InAirCrouchLogic(FTViewTarget& OutVT, float DeltaTime)
{
	if (Character)
	{
		const UCharacterMovementComponent* MovementComponent = Character->GetCharacterMovement();
		const FVector TargetCrouchOffset = FVector(0, 0, MovementComponent->GetCrouchedHalfHeight() - Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
		FVector Offset = FMath::Lerp(FVector::ZeroVector, TargetCrouchOffset, FMath::Clamp(CrouchBlendTime / CrouchBlendDuration, 0.f, 1.f));

		if (MovementComponent->IsCrouching())
		{
			CrouchBlendTime = FMath::Clamp(CrouchBlendTime + DeltaTime, 0.f, CrouchBlendDuration);
			Offset -= TargetCrouchOffset;
		}
		else
		{
			CrouchBlendTime = FMath::Clamp(CrouchBlendTime - DeltaTime, 0.f, CrouchBlendDuration);
		}

		OutVT.POV.Location += Offset;
	}
}


void ABasePlayerCameraManager::SetViewTarget(AActor* NewViewTarget, const FViewTargetTransitionParams TransitionParams)
{
	Super::SetViewTarget(NewViewTarget, TransitionParams);

	if (NewViewTarget == nullptr)
	{
		NewViewTarget = PCOwner;
	}
	
	Character = Cast<ACharacterCameraLogic>(NewViewTarget);
	if (Character)
	{
		CameraStyle = Character->Execute_GetCameraStyle(Character);
		CameraOrientation = Character->Execute_GetCameraOrientation(Character);
	}
	else
	{
		CameraOrientation = ECameraOrientation::Center;
		CameraStyle = CameraStyle_None;
	}
}
