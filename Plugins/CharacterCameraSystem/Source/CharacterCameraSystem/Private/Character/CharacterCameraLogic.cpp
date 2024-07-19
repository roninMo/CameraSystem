// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/CharacterCameraLogic.h"

#include "Camera/CameraComponent.h"
#include "CameraComponents/TargetLockSpringArm.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Logging/StructuredLog.h"

DEFINE_LOG_CATEGORY(CameraLog);


ACharacterCameraLogic::ACharacterCameraLogic()
{
	// Component logic
	PrimaryActorTick.TickGroup = TG_DuringPhysics;
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	SetReplicates(true);

	// Camera components
	CameraArm = CreateDefaultSubobject<UTargetLockSpringArm>(TEXT("Camera Arm"));
	CameraArm->SetupAttachment(RootComponent); // Attach this to the mesh because if we attach this to the root, whenever we crouch the springArm/Camera will move along with it, which is not intended
	CameraArm->SocketOffset = FVector(0, 0, 100); // Align the camera to the side of the character
	CameraArm->TargetArmLength = 340; // Distance from the character
	CameraArm->bUsePawnControlRotation = true; // Allows us to rotate the camera boom along with our controller when we're adding mouse input
	CameraArm->ProbeSize = 16.4;
	CameraArm->bEnableCameraLag = true;
	CameraArm->CameraLagSpeed = 2.3;
	CameraArm->CameraLagMaxDistance = 100.0;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	FollowCamera->SetupAttachment(CameraArm, USpringArmComponent::SocketName); // Attaches the camera to the camera's spring arm socket
	
	// Camera information
	CameraStyle = CameraStyle_ThirdPerson;
	CameraOrientation = ECameraOrientation::Center;

	TargetArmLength = 340;
	CameraLag = 2.3;
	CameraOrientationTransitionSpeed = 3.4;
	CameraOffset_FirstPerson = FVector(10.0, 0.0, 64);
	CameraOffset_Center = FVector(0.0, 0.0, 100.0);
	CameraOffset_Left = FVector(0.0, -64.0, 100.0);
	CameraOffset_Right = FVector(0.0, 64.0, 100.0);

	TargetLockRadius = 1200;
	TargetLockTransitionSpeed = 6.4;
}


void ACharacterCameraLogic::BeginPlay()
{
	Super::BeginPlay();

	OnCameraStyleSet();
	OnCameraOrientationSet();
	SetTargetLockTransitionSpeed(TargetLockTransitionSpeed);
}


void ACharacterCameraLogic::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}


void ACharacterCameraLogic::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (TargetOffset != CameraArm->SocketOffset)
	{
		UpdateCameraSocketLocation(TargetOffset, DeltaTime);
	}
}




#pragma region Camera
void ACharacterCameraLogic::SetCameraStyle_Implementation(const FName Style)
{
	if (AbleToActivateCameraTransition())
	{
		CameraStyle = Style;
		Server_SetCameraStyle(Style);
	}
}

void ACharacterCameraLogic::Server_SetCameraStyle_Implementation(const FName Style)
{
	// TODO: add logic to prevent spamming
	CameraStyle = Style;
	OnCameraStyleSet();
}


void ACharacterCameraLogic::ResetCameraTransitionDelay() { bCameraTransitionDelay = false; }
bool ACharacterCameraLogic::AbleToActivateCameraTransition()
{
	if (bCameraTransitionDelay) return false;
		
	GetWorldTimerManager().SetTimer(
		CameraTransitionDelayHandle,
		this,
		&ACharacterCameraLogic::ResetCameraTransitionDelay,
		FMath::Clamp(InputPressed_ReplicationInterval, 0.2, 1.0),
		false
	);
	bCameraTransitionDelay = true;
	return true;
}


void ACharacterCameraLogic::OnCameraStyleSet()
{
	if (CameraStyle == CameraStyle_FirstPerson)
	{
		SetRotationToCamera();
		UpdateCameraArmSettings(CameraOffset_FirstPerson, 0, false);
	}
	else if (CameraStyle == CameraStyle_TargetLocking)
	{
		SetRotationToMovement();
		UpdateCameraArmSettings(GetCameraOffset(Execute_GetCameraStyle(this), Execute_GetCameraOrientation(this)), TargetArmLength, true, CameraLag);
	}
	else if (CameraStyle == CameraStyle_ThirdPerson)
	{
		SetRotationToMovement();
		UpdateCameraArmSettings(GetCameraOffset(Execute_GetCameraStyle(this), Execute_GetCameraOrientation(this)), TargetArmLength, true, CameraLag);
	}

	// If was or is transitioning to target locking
	OnTargetLockCharacterUpdated();

	// blueprint logic
	BP_OnCameraStyleSet();
}


void ACharacterCameraLogic::SetCameraOrientation_Implementation(const ECameraOrientation Orientation)
{
	CameraOrientation = Orientation;
	OnCameraOrientationSet();
}


void ACharacterCameraLogic::OnCameraOrientationSet()
{
	FVector CameraLocation = CameraOffset_FirstPerson;
	float ArmLength = 0;
	bool bEnableCameraLag = false;
	float LagSpeed = 0;

	if (CameraStyle == CameraStyle_ThirdPerson || CameraStyle == CameraStyle_TargetLocking)
	{
		CameraLocation = GetCameraOffset(CameraStyle, CameraOrientation);
		ArmLength = TargetArmLength;
		bEnableCameraLag = true;
		LagSpeed = CameraLag;
	}
	
	UpdateCameraArmSettings(CameraLocation, ArmLength, bEnableCameraLag, LagSpeed);

	// blueprint logic
	BP_OnCameraOrientationSet();
}


void ACharacterCameraLogic::SetRotationToMovement()
{
	if (!GetCharacterMovement()) return;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	bUseControllerRotationYaw = false;
}


void ACharacterCameraLogic::SetRotationToCamera()
{
	if (!GetCharacterMovement()) return;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	bUseControllerRotationYaw = true;
}


void ACharacterCameraLogic::UpdateCameraArmSettings(const FVector CameraLocation, const float SpringArmLength, const bool bEnableCameraLag, const float LagSpeed)
{
	CameraArm->TargetArmLength = SpringArmLength; // 340.0;
	CameraArm->bEnableCameraLag = bEnableCameraLag; // true;
	CameraArm->CameraLagSpeed = LagSpeed; // 2.3;
	TargetOffset = CameraLocation;
}


void ACharacterCameraLogic::UpdateCameraSocketLocation(const FVector Offset, const float DeltaTime)
{
	CameraArm->SocketOffset = UKismetMathLibrary::VInterpTo(CameraArm->SocketOffset, Offset, DeltaTime, CameraOrientationTransitionSpeed);
}
#pragma endregion




#pragma region Target Locking
void ACharacterCameraLogic::AdjustCurrentTarget(TArray<AActor*>& ActorsToIgnore, EPreviousTargetLockOrientation NextTargetDirection, float Radius)
{
	if (TargetLockCharacters.Num() == 0)
	{
		if (bDebugTargetLocking)
		{
			UE_LOGFMT(CameraLog, Error, "{0}: There are no more characters within {1}'s target lock range!", *UEnum::GetValueAsString(GetLocalRole()), *GetName());
		}
		
		bCurrentTargetDelay = false;
		SetCurrentTarget(nullptr);
		TrySetServerCurrentTarget();
		if (CameraStyle == CameraStyle_TargetLocking)
		{
			Execute_SetCameraStyle(this, CameraStyle_ThirdPerson);
			OnCameraStyleSet();
		}
		return;	
	}
	
	if (TargetLockCharacters.Num() == 1)
	{
		if (TargetLockCharacters[0] == this) return;
		SetCurrentTarget(TargetLockCharacters[0]);
		TrySetServerCurrentTarget();
		return;
	}
	
	const FVector PlayerLocation = GetActorLocation();
	const FRotator BaseAimRotation = GetBaseAimRotation();
	const FRotator PlayerRotation = FRotator(0.0f, BaseAimRotation.Yaw, BaseAimRotation.Roll);

	// TODO: Update this to also account for how close the players are to the character
	// Calculate the distance from the character and the angle from it's forward vector
	TargetLockData.Empty();
	for (AActor* Target : TargetLockCharacters)
	{
		if (Target == this) continue;
		
		FTargetLockInformation TargetLockInfo;
		TargetLockInfo.Target = Target;

		FVector PlayerToTarget = TargetLockInfo.Target->GetActorLocation() - PlayerLocation;
		TargetLockInfo.DistanceToTarget = PlayerToTarget.Length(); 

		const FRotator PlayerToTargetRotation = PlayerToTarget.Rotation();
		const FRotator DeltaRotation = UKismetMathLibrary::NormalizedDeltaRotator(PlayerToTargetRotation, PlayerRotation);
		TargetLockInfo.AngleFromForwardVector = DeltaRotation.Yaw; // Negative is to the right, positive is to the left
		TargetLockData.Add(TargetLockInfo);
		
		if (bDebugTargetLocking)
		{
			UE_LOGFMT(CameraLog, Warning, "{0}: target {1}, RotationFromCharacter: {2}, DistanceToTarget: {3}",
				*UEnum::GetValueAsString(GetLocalRole()), *GetNameSafe(TargetLockInfo.Target), TargetLockInfo.AngleFromForwardVector, TargetLockInfo.DistanceToTarget
			);
		}
	}
	
	// Adjust the array of the characters from left to right (180, -180)
	TargetLockData.Sort([](const FTargetLockInformation& PreviousTargetData, const FTargetLockInformation& CurrentTargetData) {
		return PreviousTargetData.AngleFromForwardVector > CurrentTargetData.AngleFromForwardVector;
	});
	for (auto Target: TargetLockData)
	{
		if (bDebugTargetLocking) UE_LOGFMT(CameraLog, Log, "Adjusted Target List: {0}, YawOffset: {1}", *GetNameSafe(Target.Target), Target.AngleFromForwardVector);
	}
	
	int32 CurrentTargetIndex = 0;
	float ClosestToCharacterYaw = 340.0f;
	for (int32 Index = 0; Index != TargetLockData.Num(); ++Index)
	{
		if (CurrentTarget)
		{
			if (TargetLockData[Index].Target == CurrentTarget)
			{
				CurrentTargetIndex = Index;
				break;
			}
		}
		else
		{
			// Find the target closest to where the character is looking
			const float TargetToCharacterYaw = TargetLockData[Index].AngleFromForwardVector < 0.0f ? -1 * TargetLockData[Index].AngleFromForwardVector : TargetLockData[Index].AngleFromForwardVector;
			if (TargetToCharacterYaw < ClosestToCharacterYaw)
			{
				CurrentTargetIndex = Index;
				ClosestToCharacterYaw = TargetToCharacterYaw;
			}
		}
	}
	
	// This is for navigating between the previous or next target
	FTargetLockInformation NextTarget = TargetLockData[CurrentTargetIndex];
	if (CurrentTarget)
	{
		if (NextTargetDirection == EPreviousTargetLockOrientation::Right)
		{
			if (TargetLockData.IsValidIndex(CurrentTargetIndex - 1)) NextTarget = TargetLockData[CurrentTargetIndex - 1];
			else NextTarget = TargetLockData.Last();
		}
		else
		{
			if (TargetLockData.IsValidIndex(CurrentTargetIndex + 1)) NextTarget = TargetLockData[CurrentTargetIndex + 1];
			else NextTarget = TargetLockData[0];
		}
	}

	if (bDebugTargetLocking)
	{
		UE_LOGFMT(CameraLog, Log, "{0}: NextTarget: {1}, YawOffset: {2}", *UEnum::GetValueAsString(GetLocalRole()), *GetNameSafe(NextTarget.Target), NextTarget.AngleFromForwardVector);
	}
	
	SetCurrentTarget(NextTarget.Target);
	TrySetServerCurrentTarget();
}


void ACharacterCameraLogic::Server_SetTargetLockData_Implementation(AActor* Target)
{
	SetCurrentTarget(Target);
	OnTargetLockCharacterUpdated();
}


void ACharacterCameraLogic::OnTargetLockCharacterUpdated()
{
	if (CameraStyle != CameraStyle_TargetLocking)
	{
		SetCurrentTarget(nullptr);
	}

	CameraArm->UpdateTargetLockOffset(FVector(0, 0, 25));
	
	// blueprint logic
	BP_OnTargetLockCharacterUpdated();
}


void ACharacterCameraLogic::ResetCurrentTargetDelay() { bCurrentTargetDelay = false; }
void ACharacterCameraLogic::TrySetServerCurrentTarget()
{
	if (bCurrentTargetDelay)
	{
		return;
	}
	
	Server_SetTargetLockData(GetCurrentTarget());
	GetWorldTimerManager().SetTimer(
		CurrentTargetDelayHandle,
		this,
		&ACharacterCameraLogic::ResetCurrentTargetDelay,
		FMath::Clamp(InputPressed_ReplicationInterval, 0.4, 1.0),
		false
	);
	
	bCurrentTargetDelay = true;
}


void ACharacterCameraLogic::ClearTargetLockCharacters(TArray<AActor*>& ActorsToIgnore)
{
	if (ActorsToIgnore.IsEmpty()) TargetLockCharacters.Empty();
	else
	{
		TArray<AActor*> TargetsToRemove;
		TMap<AActor*, bool> HashMap; // Quick access to targets that should not be removed
		
		for (AActor* ActorToIgnore : ActorsToIgnore) HashMap.Add(ActorToIgnore, true);
		for (AActor* Target : TargetLockCharacters)
		{
			if (HashMap.Contains(Target)) continue;
			TargetsToRemove.Add(Target);
		}

		for (AActor* Target : TargetsToRemove) TargetLockCharacters.Remove(Target);
	}	
}
#pragma endregion 




#pragma region Utility
FName ACharacterCameraLogic::GetCameraStyle_Implementation() const
{
	return CameraStyle;
}


ECameraOrientation ACharacterCameraLogic::GetCameraOrientation_Implementation() const
{
	return CameraOrientation;
}


FVector ACharacterCameraLogic::GetCameraOffset(const FName Style, const ECameraOrientation Orientation) const
{
	if (Style == CameraStyle_FirstPerson) return CameraOffset_FirstPerson;
	if (Orientation == ECameraOrientation::Center) return CameraOffset_Center;
	if (Orientation == ECameraOrientation::LeftShoulder) return CameraOffset_Left;
	return CameraOffset_Right;
}


bool ACharacterCameraLogic::IsRotationOrientedToCamera() const
{
	if (!GetCharacterMovement()) return false;
	return GetCharacterMovement()->bOrientRotationToMovement == false && bUseControllerRotationYaw;
}


FVector ACharacterCameraLogic::GetCameraLocation()
{
	if (FollowCamera) return FollowCamera->GetComponentLocation();
	return GetActorLocation();
}


float ACharacterCameraLogic::GetCameraArmLength() const
{
	return CameraArm->TargetArmLength;
}


void ACharacterCameraLogic::SetTargetLockTransitionSpeed(const float Speed)
{
	TargetLockTransitionSpeed = Speed;
	if (CameraArm) CameraArm->TargetLockTransitionSpeed = Speed;
}


TArray<AActor*> ACharacterCameraLogic::GetTargetLockCharacters() const
{
	return TargetLockCharacters;
}


TArray<AActor*>& ACharacterCameraLogic::GetTargetLockCharactersReference()
{
	return TargetLockCharacters;
}


bool ACharacterCameraLogic::IsTargetLocking() const
{
	return CameraStyle == CameraStyle_TargetLocking;
}


AActor* ACharacterCameraLogic::GetCurrentTarget() const
{
	return CurrentTarget;
}


void ACharacterCameraLogic::SetCurrentTarget(AActor* Target)
{
	CurrentTarget = Target;
}


void ACharacterCameraLogic::SetTargetLockCharacters(TArray<AActor*>& TargetCharacters)
{
	TargetLockCharacters = TargetCharacters;
}
#pragma endregion 
