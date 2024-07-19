// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SpringArmComponent.h"
#include "TargetLockSpringArm.generated.h"

class ACharacterCameraLogic;
/**
 * 
 */
UCLASS()
class CHARACTERCAMERASYSTEM_API UTargetLockSpringArm : public USpringArmComponent
{
	GENERATED_BODY()

public:
	/** Controls how quickly the camera transitions between targets. @ref ACharacterCameraLogic's SetTargetLockTransitionSpeed value adjusts this */
	UPROPERTY(BlueprintReadWrite, Category="Target Locking", meta=(ClampMin="0.0", ClampMax="1000.0", UIMin = "0.0", UIMax = "34.0")) float TargetLockTransitionSpeed = 6.4;

	/** Adds an offset to the target lock aim location to help with the camera looking up to each target. @ref ACharacterCameraLogic has values for different camera modes that adjust this when the style updates */
	UPROPERTY(BlueprintReadWrite, Category="Target Locking") FVector TargetLockOffset = FVector(0.0, 0.0, 34.0);

	/** The current target lock character, derived from @ref ACharacterCameraLogic's target lock logic */
	UPROPERTY(BlueprintReadWrite) TObjectPtr<AActor> CurrentTarget;


protected:
	UPROPERTY(BlueprintReadWrite) TObjectPtr<ACharacterCameraLogic> Character;
	UPROPERTY(BlueprintReadWrite) bool bTargetTransition;

	
public:
	/** Updates the target lock offset */
	UFUNCTION(BlueprintCallable) virtual void UpdateTargetLockOffset(FVector Offset);
	
	
protected:
	virtual void UpdateDesiredArmLocation(bool bDoTrace, bool bDoLocationLag, bool bDoRotationLag, float DeltaTime) override;
	
	
};
