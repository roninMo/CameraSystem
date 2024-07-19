// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PlayerCameraTypes.h"
#include "CameraComponents/CameraPlayerInterface.h"
#include "GameFramework/Character.h"
#include "CharacterCameraLogic.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(CameraLog, Log, All);

class UCameraComponent;
class UTargetLockSpringArm;

UCLASS()
class CHARACTERCAMERASYSTEM_API ACharacterCameraLogic : public ACharacter, public ICameraPlayerInterface
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	TObjectPtr<UCameraComponent> FollowCamera;
	

	/** Camera information */
	/** The current style of the camera that determines the behavior. The default styles are "Fixed", "Spectator", "FirstPerson", "ThirdPerson", "TargetLocking", and "Aiming". You can also add your own in the BasePlayerCameraManager class */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera") FName CameraStyle;

	/** These are based on the client, but need to be replicated for late joining clients, so we're using both RPC's and replication to achieve this */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera") ECameraOrientation CameraOrientation;
	
	UPROPERTY(BlueprintReadWrite) FVector TargetOffset;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera") float CameraOrientationTransitionSpeed;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera") FVector CameraOffset_FirstPerson;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera") FVector CameraOffset_Center;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera") FVector CameraOffset_Left;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera") FVector CameraOffset_Right;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera") float TargetArmLength;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera") float CameraLag;

	/** Camera post process settings */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Post Processing") FPostProcessSettings HideCamera; 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Post Processing") FPostProcessSettings DefaultCameraSettings;

	/** Camera Transition Replication interval values */
	/** This is true if the player has recently tried to transition between cameras, only edit this during the camera transition handles */
	UPROPERTY(BlueprintReadWrite, Transient) bool bCameraTransitionDelay;
	UPROPERTY(BlueprintReadWrite, Transient) FTimerHandle CameraTransitionDelayHandle;
	UPROPERTY(EditAnywhere, Transient) FVector CurrentCameraOffset;

	/** The interval for when the player is allowed to transition between camera styles. This is used for network purposes */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Transient, Category = "Camera", meta=(UIMin = 0.2, ClampMin = 0.1, UIMax = 1, ClampMax = 3)) float InputPressed_ReplicationInterval = 0.25;
	
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Target Locking")
	TObjectPtr<UTargetLockSpringArm> CameraArm;
	
	/** Target lock values */
	UPROPERTY(BlueprintReadWrite, Transient) TObjectPtr<AActor> CurrentTarget;
	UPROPERTY(BlueprintReadWrite, Transient) TArray<AActor*> TargetLockCharacters;
	UPROPERTY(BlueprintReadWrite, Transient) TArray<FTargetLockInformation> TargetLockData;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera|Target Locking") float TargetLockRadius;
	
	/** Controls how quickly the camera transitions between targets. @ref ACharacterCameraLogic's TargetLockTransitionSpeed value adjusts this */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera|Target Locking", meta=(ClampMin="0.0", ClampMax="1000.0", UIMin = "0.0", UIMax = "34.0")) float TargetLockTransitionSpeed = 2;
	
	/** Target lock Replication interval values */
	UPROPERTY(BlueprintReadWrite) bool bCurrentTargetDelay;
	UPROPERTY(BlueprintReadWrite) FTimerHandle CurrentTargetDelayHandle;
	
	/** Other */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Debug") bool bDebugCameraStyle;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Debug") bool bDebugTargetLocking;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Debug") FColor TargetRadiusColor;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Debug") FColor TargetColor;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Debug") float TargetRadiusDrawTime = 6.4f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Debug") float TargetDrawTime = 6.4f;
	

public:
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void Tick(float DeltaTime) override;
	ACharacterCameraLogic();

	
protected:
	virtual void BeginPlay() override;

	
//-------------------------------------------------------------------------------------//
// Camera																			   //
//-------------------------------------------------------------------------------------//
public:
	/** Handles transitioning between different camera styles and logic specific to each style */
	UFUNCTION(BlueprintCallable, Category = "Camera") virtual void OnCameraStyleSet();

	/** Blueprint function handling transitioning between different camera styles and logic specific to each style */
	UFUNCTION(BlueprintImplementableEvent, Category="Camera", meta = (DisplayName = "On Camera Style Set (Blueprint)"))
	void BP_OnCameraStyleSet();
	
	/**
	 * Sets the camera style, and calls the OnCameraStyleSet function for handling camera transitions and other logic specific to each style.
	 * @remarks OnCameraStyleSet should be called if TryActivateCameraTransition returns true
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera") virtual void SetCameraStyle_Implementation(FName Style) override;
	
	/** Returns true if there's no input replication delay for transitioning between different camera modes */
	UFUNCTION(BlueprintCallable, Category = "Camera") virtual bool AbleToActivateCameraTransition();

	
protected:
	/**
	 * This function is called on the server to update the camera state of the character based on the Camera style.
	 * It also handles target locking, but not necessarily sorting and updating the targets.
	 * @remarks Do not spam this please, it'll crash the server and cause other things to also break
	 */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Camera") virtual void Server_SetCameraStyle(FName Style);
	// TODO: First/Third person values should be sent to the server when a character becomes net relevant, I don't know if this still holds true for characters who join late (OnRep_Notify fixes this)
	
	/** Sets Camera transition delay to false to allow you to transition between camera styles */
	UFUNCTION(BlueprintCallable, Category = "Camera") virtual void ResetCameraTransitionDelay();

	
public:
	/** Sets the orientation of a third person camera. This is only valid for third person, and does not have any logic in the base class */
	UFUNCTION(BlueprintCallable, Category = "Camera|Orientation") virtual void SetCameraOrientation_Implementation(ECameraOrientation Orientation) override;

	/** Handles transitioning between different camera styles and logic specific to each style */
	UFUNCTION(BlueprintCallable, Category = "Camera") virtual void OnCameraOrientationSet();
	
	/** Blueprint function handling transitioning between different camera styles and logic specific to each style */
	UFUNCTION(BlueprintImplementableEvent, Category="Camera", meta = (DisplayName = "On Camera Orientation Set (Blueprint)"))
	void BP_OnCameraOrientationSet();

	
	/**
	 * Third person style movement where the character turns specific to where they're walking (bOrientRotationToMovement && !bUseControllerRotationYaw)
	 * @remarks If you update the character's rotation based on the player movement or the camera, you also need to update the movement component on whether it should allow air strafing
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera|Orientation") virtual void SetRotationToMovement();

	/**
	 * First person style movement where the character looks in the direction of the camera (!bOrientRotationToMovement && bUseControllerRotationYaw)
	 * @remarks If you update the character's rotation based on the player movement or the camera, you also need to update the movement component on whether it should allow air strafing
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera|Orientation") virtual void SetRotationToCamera();
	
	/** Adjusts the location of the third person camera */
	UFUNCTION(BlueprintCallable, Category = "Camera|Orientation") virtual void UpdateCameraArmSettings(FVector CameraLocation, float SpringArmLength, bool bEnableCameraLag, float LagSpeed = 0);

	/** Updates the camera's target offset to transition to the target offset */
	UFUNCTION(BlueprintCallable, Category = "Camera|Orientation") virtual void UpdateCameraSocketLocation(FVector Offset, float DeltaTime);

	
//--------------------------------------------------------------------------------------------------------------------------//
// Target Locking																											//
//--------------------------------------------------------------------------------------------------------------------------//
public:
	/**
	 * Sorts through the target lock characters and sets the next active target. Finds how far away each target is, and their orientation to the player.
	 * The array is sorted from left to right, and the active target is selected based on the next target's direction
	 * @remarks There's also just overrides for handling this logic uniquely for each situation
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera|Target Locking") virtual void AdjustCurrentTarget(
		UPARAM(ref) TArray<AActor*>& ActorsToIgnore,
		EPreviousTargetLockOrientation NextTargetDirection = EPreviousTargetLockOrientation::Right,
		float Radius = 640.0f
	);

	/** Logic once the target lock character has been updated */
	UFUNCTION(BlueprintCallable, Category = "Camera|Target Locking") virtual void OnTargetLockCharacterUpdated();
	// TODO: Add logic on target transitions to clear invalid target handles

	/** Blueprint function handling transitioning between different target lock characters */
	UFUNCTION(BlueprintImplementableEvent, Category="Camera|Target Locking", meta = (DisplayName = "On Target Lock Character Updated (Blueprint)"))
	void BP_OnTargetLockCharacterUpdated();
	
	
protected:
	/**
	 * Function that's called intermittently after calculating the target lock data on the client. The camera handles this by default, 
	 * however there's a slight offset while calculating the target for other clients if it's null on the server
	 */
	UFUNCTION(Server, Unreliable, BlueprintCallable, Category="Camera|Target Locking") virtual void Server_SetTargetLockData(AActor* Target);

	/** Checks if it's able to update the active target for the server, and if so updates the information */
	UFUNCTION(BlueprintCallable, Category="Camera|Target Locking") virtual void TrySetServerCurrentTarget();

	/** Resets the target lock delay to allow transition between targets */
	UFUNCTION(BlueprintCallable, Category="Camera|Target Locking") virtual void ResetCurrentTargetDelay();
	
	/** Clears the array of target lock characters */
	UFUNCTION(BlueprintCallable, Category="Camera|Target Locking") virtual void ClearTargetLockCharacters(UPARAM(ref) TArray<AActor*>& ActorsToIgnore);
	
	
//-------------------------------------------------------------------------------------//
// Utility																			   //
//-------------------------------------------------------------------------------------//
public:
	/** Returns the camera style */
	virtual FName GetCameraStyle_Implementation() const override;
	
	/** Returns the camera orientation */
	virtual ECameraOrientation GetCameraOrientation_Implementation() const override;

	/** Returns the camera offset based on the camera's style and orientation */
	UFUNCTION(BlueprintCallable, Category="Camera|Utility") virtual FVector GetCameraOffset(FName Style, ECameraOrientation Orientation) const;
	
	/** Checks if the character's rotation is oriented towards the camera, and returns true if so */
	UFUNCTION(BlueprintCallable, Category="Camera|Utility") virtual bool IsRotationOrientedToCamera() const;

	/** Returns player's camera location */
	UFUNCTION(BlueprintCallable, Category="Camera|Utility") virtual FVector GetCameraLocation();

	/** Returns the camera arm's length */
	UFUNCTION(BlueprintCallable, Category="Camera|Utility") virtual float GetCameraArmLength() const;

	/** Updates the target lock transition speed for the character and the camera arm */
	UFUNCTION(BlueprintCallable, Category="Camera|Utility") virtual void SetTargetLockTransitionSpeed(float Speed);
	
public:
	UFUNCTION() virtual TArray<AActor*>& GetTargetLockCharactersReference();
	UFUNCTION(BlueprintCallable, Category="Camera|Utility") virtual TArray<AActor*> GetTargetLockCharacters() const;
	UFUNCTION(BlueprintCallable, Category="Camera|Utility") bool IsTargetLocking() const;
	UFUNCTION(BlueprintCallable, Category="Camera|Utility") AActor* GetCurrentTarget() const;


protected:
	UFUNCTION(BlueprintCallable, Category="Camera|Utility") virtual void SetCurrentTarget(AActor* Target);
	UFUNCTION(BlueprintCallable, Category="Camera|Utility") virtual void SetTargetLockCharacters(UPARAM(ref) TArray<AActor*>& TargetCharacters);
	
	
};
