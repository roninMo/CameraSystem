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

	/**** Camera information ****/
	/** The current style of the camera that determines the behavior. The default styles are "Fixed", "Spectator", "FirstPerson", "ThirdPerson", "TargetLocking", and "Aiming". You can also add your own in the BasePlayerCameraManager class */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera") FName CameraStyle;

	/** These are based on the client, but need to be replicated for late joining clients, so we're using both RPC's and replication to achieve this */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera") ECameraOrientation CameraOrientation;

	/** The target camera location that we interp to during transitions between different camera orientations */
	UPROPERTY(BlueprintReadWrite) FVector TargetOffset;
	
	/** The camera orientation transition speed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta=(ClampMin="0.0", UIMin = "0.0", UIMax = "34.0")) float CameraOrientationTransitionSpeed;

	/** Controls how quickly the camera transitions between targets. @ref ACharacterCameraLogic's TargetLockTransitionSpeed value adjusts this */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta=(ClampMin="0.0", UIMin = "0.0", UIMax = "34.0")) float TargetLockTransitionSpeed;
	
	/** The first person camera's location */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera") FVector CameraOffset_FirstPerson;
	
	/** The third person camera's default location */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera") FVector CameraOffset_Center;
	
	/** The third person camera's left side location */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera") FVector CameraOffset_Left;
	
	/** The third person camera's right side location */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera") FVector CameraOffset_Right;
	
	/** The camera lag of the arm. @remarks This overrides the value of the camera arm's lag */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta=(ClampMin="0.0", UIMin = "0.0", UIMax = "10.0")) float CameraLag;
	
	/** The target arm length of the camera arm. @remarks This overrides the value of the camera arm's target arm length */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera") float TargetArmLength;

	
	/**** Camera Transition Replication interval values ****/
	/** This is true if the player has recently tried to transition between cameras, only edit this during the camera transition handles */
	UPROPERTY(BlueprintReadWrite, Transient, Category = "Camera|Networking") bool bCameraTransitionDelay;

	/** This is a handle for camera transition delay for remote procedure calls, to prevent network issues. This helps the server camera rotations be in sync with the client. */
	UPROPERTY(BlueprintReadWrite, Transient, Category = "Camera|Networking") FTimerHandle CameraTransitionDelayHandle;

	/** The current camera offset, updated by TargetCameraOffset value during camera orientation transitions. Don't edit this directly, just let it do it's own thing */
	UPROPERTY(BlueprintReadWrite, Transient, Category = "Camera|Networking") FVector CurrentCameraOffset;

	/** The interval for when the player is allowed to transition between camera styles. This is used for network purposes */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Transient, Category = "Camera|Networking", meta=(UIMin = 0.2, ClampMin = 0.1, UIMax = 1, ClampMax = 3)) float InputPressed_ReplicationInterval = 0.25;

	
	/**** Camera post process settings ****/
	/** Hide camera */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Post Processing") FPostProcessSettings HideCamera;
	
	/** Default camera settings */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Post Processing") FPostProcessSettings DefaultCameraSettings;

	
protected:
	/**** Target lock values ****/
	/** The camera arm for handling camera smoothing and target lock logic. There's a lot of functionality that comes out of the box that helps with smooth camera logic, I suggest you leverage this to handle your camera logic */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Target Locking")
	TObjectPtr<UTargetLockSpringArm> CameraArm;
	
	/** The current target the player is focusing on */
	UPROPERTY(BlueprintReadWrite, Transient, Category = "Camera|Target Locking") TObjectPtr<AActor> CurrentTarget;

	/** The list of target lock characters */
	UPROPERTY(BlueprintReadWrite, Transient, Category = "Camera|Target Locking") TArray<AActor*> TargetLockCharacters;
	
	/** Metadata about each target lock character, used during AdjustCurrentTarget() to find the next target to transition to. Create your custom logic for how you transition to other targets there */
	UPROPERTY(BlueprintReadWrite, Transient, Category = "Camera|Target Locking") TArray<FTargetLockInformation> TargetLockData;
	
	/**** Target lock Replication interval values ****/
	/** There's a delay between when the server sends the information to the server on the current target (because it isn't required, and only slightly affects the camera rotation). When the target lock updates, this helps updating the info */
	UPROPERTY(BlueprintReadWrite, Category = "Camera|Target Locking|Networking") bool bCurrentTargetDelay;

	/** The handle for whether the current target lock character should be sent to the server */
	UPROPERTY(BlueprintReadWrite, Category = "Camera|Target Locking|Networking") FTimerHandle CurrentTargetDelayHandle;

	
	/**** Other ****/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Debug") bool bDebugCameraStyle;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Debug") bool bDebugCameraOrientation;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Debug") bool bDebugTargetLocking;
	

public:
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void Tick(float DeltaTime) override;
	ACharacterCameraLogic(const FObjectInitializer& ObjectInitializer);

	
protected:
	virtual void BeginPlay() override;

	
//-------------------------------------------------------------------------------------//
// Camera																			   //
//-------------------------------------------------------------------------------------//
public:
	/**
	 * Sets the camera style, and calls the OnCameraStyleSet function for handling camera transitions and other logic specific to each style.
	 * The default styles are "Fixed", "Spectator", "FirstPerson", "ThirdPerson", "TargetLocking", and "Aiming"
	 * 
	 * @remark Overriding this functions removes the default logic for transitioning between styles
	 * @remark OnCameraStyleSet should be called if TryActivateCameraTransition returns true
	 */
	virtual void SetCameraStyle_Implementation(FName Style) override;
	
	/** Handles transitioning between different camera styles and logic specific to each style */
	UFUNCTION(BlueprintCallable, Category = "Camera|Style") virtual void OnCameraStyleSet();

	/** Blueprint function handling transitioning between different camera styles and logic specific to each style */
	UFUNCTION(BlueprintImplementableEvent, Category="Camera|Style", meta = (DisplayName = "On Camera Style Set"))
	void BP_OnCameraStyleSet();
	
	/** Returns true if there's no input replication delay for transitioning between different camera modes */
	UFUNCTION(BlueprintCallable, Category = "Camera|Style") virtual bool IsAbleToActivateCameraTransition();

	
protected:
	/**
	 * This function is called on the server to update the camera state of the character based on the Camera style.
	 * It also handles target locking, but not necessarily sorting and updating the targets.
	 * @remarks There's handles to prevent this from constantly being invoked, the purpose is to keep the camera rotation on the server in sync with the character
	 */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Camera|Style") virtual void Server_SetCameraStyle(FName Style);
	// TODO: First/Third person values should be sent to the server when a character becomes net relevant, I don't know if this still holds true for characters who join late (OnRep_Notify fixes this)
	
	/** Sets Camera transition delay to false to allow you to transition between camera styles */
	UFUNCTION(BlueprintCallable, Category = "Camera|Style") virtual void ResetCameraTransitionDelay();

	
public:
	/**
	 * Sets the player's camera orientation.
	 * 
	 * @remark Overriding this functions removes the default logic for transitioning between orientations
	 * @remark OnCameraOrientation should be called if TryActivateCameraOrientation returns true
	 */
	virtual void SetCameraOrientation_Implementation(ECameraOrientation Orientation) override;

	/** Handles transitioning between different camera styles and logic specific to each style */
	UFUNCTION(BlueprintCallable, Category = "Camera|Orientation") virtual void OnCameraOrientationSet();
	
	/** Blueprint function handling transitioning between different camera styles and logic specific to each style */
	UFUNCTION(BlueprintImplementableEvent, Category = "Camera|Orientation", meta = (DisplayName = "On Camera Orientation Set"))
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
	 * 
	 * @remarks Overriding this functions removes the default logic for transitioning between targets
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Camera|Target Locking") void AdjustCurrentTarget(
		UPARAM(ref) TArray<AActor*>& ActorsToIgnore,
		EPreviousTargetLockOrientation NextTargetDirection = EPreviousTargetLockOrientation::Right,
		float Radius = 640.0f
	);
	virtual void AdjustCurrentTarget_Implementation(
	   UPARAM(ref) TArray<AActor*>& ActorsToIgnore,
	   EPreviousTargetLockOrientation NextTargetDirection = EPreviousTargetLockOrientation::Right,
	   float Radius = 640.0f
   );

	/**
	 * Logic once the target lock character has been updated
	 * @remarks There's also a blueprint event for adding logic to this
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera|Target Locking") virtual void OnTargetLockCharacterUpdated();
	// TODO: Add logic on target transitions to clear invalid target handles

	/** Blueprint function handling transitioning between different target lock characters */
	UFUNCTION(BlueprintImplementableEvent, Category = "Camera|Target Locking", meta = (DisplayName = "On Target Lock Character Updated"))
	void BP_OnTargetLockCharacterUpdated();
	
	
protected:
	/**
	 * Function that's called intermittently after calculating the target lock data on the client. The camera handles this by default, 
	 * however there's a slight offset while calculating the target for other clients if it's null on the server
	 */
	UFUNCTION(Server, Unreliable, BlueprintCallable, Category = "Camera|Target Locking") virtual void Server_SetTargetLockData(AActor* Target);

	/** Checks if it's able to update the active target for the server, and if so updates the information */
	UFUNCTION(BlueprintCallable, Category = "Camera|Target Locking") virtual void TrySetServerCurrentTarget();

	/** Resets the target lock delay to allow transition between targets */
	UFUNCTION(BlueprintCallable, Category = "Camera|Target Locking") virtual void ResetCurrentTargetDelay();
	
	/** Clears the array of target lock characters */
	UFUNCTION(BlueprintCallable, Category = "Camera|Target Locking") virtual void ClearTargetLockCharacters(UPARAM(ref) TArray<AActor*>& ActorsToIgnore);
	
	
//-------------------------------------------------------------------------------------//
// Utility																			   //
//-------------------------------------------------------------------------------------//
public:
	/** Returns the camera style */
	virtual FName GetCameraStyle_Implementation() const override;
	
	/** Returns the camera orientation */
	virtual ECameraOrientation GetCameraOrientation_Implementation() const override;

	/** Returns the camera offset based on the camera's style and orientation */
	UFUNCTION(BlueprintCallable, Category = "Camera|Utilities") virtual FVector GetCameraOffset(FName Style, ECameraOrientation Orientation) const;
	
	/** Checks if the character's rotation is oriented towards the camera, and returns true if so */
	UFUNCTION(BlueprintCallable, Category = "Camera|Orientation") virtual bool IsRotationOrientedToCamera() const;

	/** Returns player's camera location */
	UFUNCTION(BlueprintCallable, Category = "Camera|Utilities") virtual FVector GetCameraLocation();

	/** Returns the camera arm's length */
	UFUNCTION(BlueprintCallable, Category = "Camera|Utilities") virtual float GetCameraArmLength() const;

	/** Updates the target lock transition speed for the character and the camera arm */
	UFUNCTION(BlueprintCallable, Category = "Camera|Target Locking") virtual void SetTargetLockTransitionSpeed(float Speed);
	
public:
	/** Internal function for returning a reference to the target lock characters array */
	UFUNCTION() virtual TArray<AActor*>& GetTargetLockCharactersReference();

	/** Returns a list of the target lock characters */
	UFUNCTION(BlueprintCallable, Category = "Camera|Target Locking") virtual TArray<AActor*> GetTargetLockCharacters() const;

	/** Returns true if the character is target locking */
	UFUNCTION(BlueprintCallable, Category = "Camera|Target Locking") bool IsTargetLocking() const;

	/** Returns the current target */
	UFUNCTION(BlueprintCallable, Category = "Camera|Target Locking") AActor* GetCurrentTarget() const;


protected:
	/** Set's a new target */
	UFUNCTION(BlueprintCallable, Category = "Camera|Target Locking") virtual void SetCurrentTarget(AActor* Target);

	/** Sets the target lock character's list */
	UFUNCTION(BlueprintCallable, Category = "Camera|Target Locking") virtual void SetTargetLockCharacters(UPARAM(ref) TArray<AActor*>& TargetCharacters);
	
	
};
