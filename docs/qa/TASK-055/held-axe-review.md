# TASK-055 equipped stone axe display

The third test run used the normal public InitializeActorsForPlay/NotifyBeginPlay lifecycle. All positive controls passed, and only the broken current GUID with a usable spare and the ranged-bow selection failed. The prior two fixture runs did not establish a business RED: AActor::ProcessEvent skipped normal actor delegate callbacks before actors were initialized. Visibility diagnostics excluded hidden-in-game and level collection masking.

RefreshHeldTool now reads the current weapon instance GUID, requires the actual axe instance to have durability above zero, and hides it when ranged mode is selected. Existing inventory/gameplay/restore events and imported mesh/hand_r attachment remain the update path. No new header, polling or setter.

Root build passed; held-axe055-green-restart071-write passed this test with no errors or warnings. The original repair-page test was unchanged. The evidence covers the real Hero component display state and normal equipment transaction callbacks; camera framing, grip, continuous motion and formal map combat still require separate verification.
