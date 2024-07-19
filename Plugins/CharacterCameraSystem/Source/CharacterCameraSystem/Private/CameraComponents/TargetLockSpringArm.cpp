// Fill out your copyright notice in the Description page of Project Settings.


#include "CameraComponents/TargetLockSpringArm.h"

#include "Character/CharacterCameraLogic.h"
#include "PhysicsEngine/PhysicsSettings.h"


void UTargetLockSpringArm::UpdateDesiredArmLocation(bool bDoTrace, bool bDoLocationLag, bool bDoRotationLag, float DeltaTime)
{
	FRotator DesiredRotation = GetTargetRotation();

	// If our ViewTarget is simulating using physics, we may need to clamp delta time
	if (bClampToMaxPhysicsDeltaTime)
	{
		// Use the same max time step cap as the physics system to avoid camera jitter when the ViewTarget simulates less time than the camera
		DeltaTime = FMath::Min(DeltaTime, UPhysicsSettings::Get()->MaxPhysicsDeltaTime);
	}
	
	// If the player is target locking an enemy, update the rotation to face the target 
	Character = Character ? Character : Cast<ACharacterCameraLogic>(GetOwner());
	if (Character && Character->Execute_GetCameraStyle(Character) == CameraStyle_TargetLocking)
	{
		AActor* Target = Character->GetCurrentTarget();
		// The initial transition to a target should be interpolated like so
		if (CurrentTarget != Target)
		{
			CurrentTarget = Target;
			bTargetTransition = true;
		}
		
		if (CurrentTarget)
		{
			// If they just selected a target or are transitioning between targets we're going to add interpolation which is going to cause some lag until it finishes the transition
			FVector TargetLocation = CurrentTarget->GetActorLocation() + TargetLockOffset;
			FRotator TargetRotation = (TargetLocation - PreviousDesiredLoc).Rotation();
			if (bTargetTransition)
			{
				DesiredRotation = FRotator(FMath::QInterpTo(FQuat(PreviousDesiredRot), FQuat(TargetRotation), DeltaTime, TargetLockTransitionSpeed));
				if (DesiredRotation.Equals(TargetRotation, 0.4)) bTargetTransition = false;
			}
			else DesiredRotation = TargetRotation;

			// Also update the pawn control rotation to avoid drunken movement inputs from the character
			AController* PlayerController = Character->GetController();
			if (PlayerController)
			{
				// Character->SetActorRotation(DesiredRotation); // Vertical movement should be smoothed out here, otherwise this is going to mess up the players rotation
				PlayerController->SetControlRotation(DesiredRotation);
			}
		}
	}

	// Apply 'lag' to rotation if desired
	if(bDoRotationLag)
	{
		if (bUseCameraLagSubstepping && DeltaTime > CameraLagMaxTimeStep && CameraRotationLagSpeed > 0.f)
		{
			const FRotator ArmRotStep = (DesiredRotation - PreviousDesiredRot).GetNormalized() * (1.f / DeltaTime);
			FRotator LerpTarget = PreviousDesiredRot;
			float RemainingTime = DeltaTime;
			while (RemainingTime > UE_KINDA_SMALL_NUMBER)
			{
				const float LerpAmount = FMath::Min(CameraLagMaxTimeStep, RemainingTime);
				LerpTarget += ArmRotStep * LerpAmount;
				RemainingTime -= LerpAmount;

				DesiredRotation = FRotator(FMath::QInterpTo(FQuat(PreviousDesiredRot), FQuat(LerpTarget), LerpAmount, CameraRotationLagSpeed));
				PreviousDesiredRot = DesiredRotation;
			}
		}
		else
		{
			DesiredRotation = FRotator(FMath::QInterpTo(FQuat(PreviousDesiredRot), FQuat(DesiredRotation), DeltaTime, CameraRotationLagSpeed));
		}
	}
	PreviousDesiredRot = DesiredRotation;

	
	// Get the spring arm 'origin', the target we want to look at
	FVector ArmOrigin = GetComponentLocation() + TargetOffset;
	// We lag the target, not the actual camera position, so rotating the camera around does not have lag
	FVector DesiredLoc = ArmOrigin;
	if (bDoLocationLag)
	{
		if (bUseCameraLagSubstepping && DeltaTime > CameraLagMaxTimeStep && CameraLagSpeed > 0.f)
		{
			const FVector ArmMovementStep = (DesiredLoc - PreviousDesiredLoc) * (1.f / DeltaTime);
			FVector LerpTarget = PreviousDesiredLoc;

			float RemainingTime = DeltaTime;
			while (RemainingTime > UE_KINDA_SMALL_NUMBER)
			{
				const float LerpAmount = FMath::Min(CameraLagMaxTimeStep, RemainingTime);
				LerpTarget += ArmMovementStep * LerpAmount;
				RemainingTime -= LerpAmount;

				DesiredLoc = FMath::VInterpTo(PreviousDesiredLoc, LerpTarget, LerpAmount, CameraLagSpeed);
				PreviousDesiredLoc = DesiredLoc;
			}
		}
		else
		{
			DesiredLoc = FMath::VInterpTo(PreviousDesiredLoc, DesiredLoc, DeltaTime, CameraLagSpeed);
		}

		// Clamp distance if requested
		bool bClampedDist = false;
		if (CameraLagMaxDistance > 0.f)
		{
			const FVector FromOrigin = DesiredLoc - ArmOrigin;
			if (FromOrigin.SizeSquared() > FMath::Square(CameraLagMaxDistance))
			{
				DesiredLoc = ArmOrigin + FromOrigin.GetClampedToMaxSize(CameraLagMaxDistance);
				bClampedDist = true;
			}
		}		

		#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		if (bDrawDebugLagMarkers)
		{
			DrawDebugSphere(GetWorld(), ArmOrigin, 5.f, 8, FColor::Green);
			DrawDebugSphere(GetWorld(), DesiredLoc, 5.f, 8, FColor::Yellow);

			const FVector ToOrigin = ArmOrigin - DesiredLoc;
			DrawDebugDirectionalArrow(GetWorld(), DesiredLoc, DesiredLoc + ToOrigin * 0.5f, 7.5f, bClampedDist ? FColor::Red : FColor::Green);
			DrawDebugDirectionalArrow(GetWorld(), DesiredLoc + ToOrigin * 0.5f, ArmOrigin,  7.5f, bClampedDist ? FColor::Red : FColor::Green);
		}
		#endif
	}

	PreviousArmOrigin = ArmOrigin;
	PreviousDesiredLoc = DesiredLoc;

	
	// Now offset camera position back along our rotation
	DesiredLoc -= DesiredRotation.Vector() * TargetArmLength;
	// Add socket offset in local space
	DesiredLoc += FRotationMatrix(DesiredRotation).TransformVector(SocketOffset);

	// Do a sweep to ensure we are not penetrating the world
	FVector ResultLoc;
	if (bDoTrace && (TargetArmLength != 0.0f))
	{
		bIsCameraFixed = true;
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(SpringArm), false, GetOwner());

		FHitResult Result;
		GetWorld()->SweepSingleByChannel(Result, ArmOrigin, DesiredLoc, FQuat::Identity, ProbeChannel, FCollisionShape::MakeSphere(ProbeSize), QueryParams);
		
		UnfixedCameraPosition = DesiredLoc;

		ResultLoc = BlendLocations(DesiredLoc, Result.Location, Result.bBlockingHit, DeltaTime);

		if (ResultLoc == DesiredLoc) 
		{	
			bIsCameraFixed = false;
		}
	}
	else
	{
		ResultLoc = DesiredLoc;
		bIsCameraFixed = false;
		UnfixedCameraPosition = ResultLoc;
	}

	// Form a transform for new world transform for camera
	FTransform WorldCamTM(DesiredRotation, ResultLoc);
	// Convert to relative to component
	FTransform RelCamTM = WorldCamTM.GetRelativeTransform(GetComponentTransform());

	// Update socket location/rotation
	RelativeSocketLocation = RelCamTM.GetLocation();
	RelativeSocketRotation = RelCamTM.GetRotation();

	UpdateChildTransforms();
	
	if (Character && Character->Execute_GetCameraStyle(Character) != CameraStyle_TargetLocking)
	{
		CurrentTarget = nullptr;
		bTargetTransition = false;
	}
}


void UTargetLockSpringArm::UpdateTargetLockOffset(FVector Offset)
{
	TargetLockOffset = Offset;
}
