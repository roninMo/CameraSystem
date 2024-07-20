# Overview

The `Camera System` helps with logic for handling smooth transitions between different camera behaviors, and has built in logic for first, third, and target lock logic with smooth movement and easy to implement customization. 

![CameraSystemTutorial](/images/CameraSystemImage.png)

`Hopefully this saves you time and effort while you're developing things`




<br><br/>
# Setup
The primary components for creating a good camera system are the `CharacterCamera` and `BaseCameraManager`. Just have your character component use the CharacterCamera class, and the controller use the proper CameraManager class, and customize any other logic you'd like to adjust

![CameraSystemTutorial](/images/CameraSystemTutorial.png)

There's also blueprint examples for reference!



<br><br/>
# Tutorial 

<br><br/>
## Create the Camera Character
Begin by creating the `CharacterCameraLogic` component blueprint, and once you've created the class open the CameraCharacter details panel to adjust the settings

![CameraSystemTutorial](/images/CameraSystemTutorial_2.png)




<br><br/>
## `CharacterCameraComponent` Settings
There's a lot of settings for adjusting the transitions, and the locations for every camera style. Some camera arm settings are also here, just to make things easier, so edit these values and let the character camera logic handle everything else. 

![CameraSystemTutorial](/images/CameraSystemTutorial_3.png)

<br><br/>
Here's a list of what everything does:

| CameraStyle 							| Description						|
| ---									                         | -----------						|
| CameraStyle							                   | The current style of the camera that determines the behavior. The default styles are `Fixed`, `Spectator`, `FirstPerson`, `ThirdPerson`, `TargetLocking`, and `Aiming`. You can also add your own in the BasePlayerCameraManager class |
| CameraOrientation 					              | These are based on the client, but need to be replicated for late joining clients, so we're using both RPC's and replication to achieve this |
| CameraOrientation Transition Speed 	 | The camera orientation transition speed |
| Target Lock Transition Speed 			     | Controls how quickly the camera transitions between targets. `ACharacterCameraLogic`'s **TargetLockTransitionSpeed** value adjusts this |
| Camera Offset First Person 			       | The first person camera's location |
| Camera Offset Center 					           | The third person camera's default location |
| Camera Offset Left 					             | The third person camera's left side location |
| Camera Offset Right 					            | The third person camera's right side location |
| Camera Lag 							                   | The camera lag of the arm. @remarks This overrides the value of the camera arm's lag |
| Target Arm Length 					              | The target arm length of the camera arm. @remarks This overrides the value of the camera arm's target arm length |
| Input Pressed Replication Interval 	 | The interval for when the player is allowed to transition between camera styles. This is used for network purposes |
	


 
<br><br/>
## `CharacterCameraComponent` Functionality
Once you open the character camera component, just search through the list of functions for the list of `Camera` functions. There's some that you can customize and override your own behavior and logic, and there's also default logic that uses the values stated previously for smooth camera logic without any of the complications. Here's a list of the functions you should know about:

<br><br/>
#### `SetCameraStyle`
This calls `TryActivateCameraTransition` to prevent network problems, and then updates the camera style on the server if it succeeds. If everything is good, `OnCameraStyleSet` is then invoked and you can override this in your blueprint for adding your own logic when the camera style is adjusted. 
	
![CameraSystemTutorial](/images/CameraSystemTutorial_3.png)

<br><br/>
Here's example code of what's handled during `OnCameraStyleSet`:

``` c++
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
	
		if (bDebugCameraStyle)
		{
			UE_LOGFMT(CameraLog, Log, "{0}: {1}'s camera style was updated to {2}",
				*UEnum::GetValueAsString(GetLocalRole()), *GetName(), CameraStyle
			);
		}
		
		// blueprint logic
		BP_OnCameraStyleSet();
	}
```

![CameraSystemTutorial](/images/CameraSystemTutorial_3_.png)




<br><br/>
#### `SetCameraOrientation`
This adjusts the camera orientation, and then calls `OnCameraOrientationSet`. By default, `OnCameraOrientationSet` updates the camera arm location, and if you want to customize this you'll have to invoke `UpdateCameraArmSettings` in the blueprint. It won't cause any problems because the transition logic is handled during runtime to create smooth transitions with `UpdateCameraSocketLocation`, the values it uses are from the update camera arm settings function. Here's example code on how to use `SetCameraOrientation`:

``` c++
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
		
		if (bDebugCameraOrientation)
		{
			UE_LOGFMT(CameraLog, Log, "{0}: {1}'s camera orientation was updated to {2}",
				*UEnum::GetValueAsString(GetLocalRole()), *GetName(), *UEnum::GetValueAsString(CameraOrientation)
			);
		}
		
		// blueprint logic
		BP_OnCameraOrientationSet();
	 }
```

![CameraSystemTutorial](/images/CameraSystemTutorial_4.png)
![CameraSystemTutorial](/images/CameraSystemTutorial_5.png)




<br><br/>
#### `Target Locking`
Once you add targets to the character list, use the `AdjustCurrentTarget` function for handling transitioning between each of the targets. By default the target it first selects is the one closest to where the player is aiming, and after that if you pass in the orientation left/right to navigate betwwen the target list. It handles determining where the characters are with reference to the character, and you can override this to add custom logic here for how you want to transition between targets. Here's example blueprint code for target locking: 

![CameraSystemTutorial](/images/CameraSystemTutorial_7.png)
![CameraSystemTutorial](/images/CameraSystemTutorial_8.png)




<br><br/>
#### Set function and Delegate function overrides
There's overrides for the logic that handles the camera logic and events for when something happens, and both are blueprints to keep that in mind whule you're developing

![CameraSystemTutorial](/images/CameraSystemTutorial_9.png)




<br><br/>
## Camera Behavior Customization and Adding Behaviors
If you'd like to adjust one of the behaviors or create your own custom camera logic, there's already functions in place for this. Create a custom `BasePlayerCameraManager` class, and override these functions for this. `BlueprintUpdateViewTarget` allows you to add additional behaviors

![CameraSystemTutorial](/images/CameraSystemTutorial_10.png)
![CameraSystemTutorial](/images/CameraSystemTutorial_12.png)



## Happy Coding (:
