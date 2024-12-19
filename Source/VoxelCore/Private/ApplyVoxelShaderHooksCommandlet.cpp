﻿// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "ApplyVoxelShaderHooksCommandlet.h"
#include "VoxelShaderHooksManager.h"

#if WITH_EDITOR
int32 UApplyVoxelShaderHooksCommandlet::Main(const FString& Params)
{
	for (FVoxelShaderHookGroup* Hook : GVoxelShaderHooksManager->Hooks)
	{
		Hook->Apply();
	}

	bool bFailed = false;
	for (const FVoxelShaderHookGroup* Hook : GVoxelShaderHooksManager->Hooks)
	{
		if (Hook->GetState() == EVoxelShaderHookState::Active)
		{
			continue;
		}

		bFailed = true;
		LOG_VOXEL(Error, "Failed to apply hook %s", *Hook->DisplayName);
	}

	if (bFailed)
	{
		return 1;
	}

	return 0;
}
#endif