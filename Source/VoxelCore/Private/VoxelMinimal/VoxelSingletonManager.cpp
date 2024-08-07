﻿// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelSingletonManager.h"
#include "VoxelSingletonSceneViewExtension.h"

struct FVoxelQueuedSingleton
{
	FVoxelSingleton* Singleton = nullptr;
	FVoxelQueuedSingleton* Next = nullptr;
};
FVoxelQueuedSingleton* GVoxelQueuedSingleton = nullptr;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelSingletonManager* GVoxelSingletonManager = nullptr;

VOXEL_RUN_ON_STARTUP_GAME()
{
	// Do this once all singletons have been queued
	GVoxelSingletonManager = new FVoxelSingletonManager();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelSingletonManager::RegisterSingleton(FVoxelSingleton& Singleton)
{
	ensure(!GVoxelSingletonManager);

	FVoxelQueuedSingleton* QueuedSingleton = new FVoxelQueuedSingleton();
	QueuedSingleton->Singleton = &Singleton;
	QueuedSingleton->Next = GVoxelQueuedSingleton;
	GVoxelQueuedSingleton = QueuedSingleton;
}

void FVoxelSingletonManager::Destroy()
{
	ensure(GVoxelSingletonManager);

	delete GVoxelSingletonManager;
	GVoxelSingletonManager = nullptr;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelSingletonManager::FVoxelSingletonManager()
{
	VOXEL_FUNCTION_COUNTER();

	while (GVoxelQueuedSingleton)
	{
		FVoxelSingleton* Singleton = GVoxelQueuedSingleton->Singleton;

		{
			FVoxelQueuedSingleton* Next = GVoxelQueuedSingleton->Next;
			delete GVoxelQueuedSingleton;
			GVoxelQueuedSingleton = Next;
		}

		if (!GIsEditor &&
			Singleton->IsEditorOnly())
		{
			// Editor singletons should be within WITH_EDITOR
			ensure(WITH_EDITOR);
			continue;
		}

		Singletons.Add(Singleton);
	}
	check(!GVoxelQueuedSingleton);

	for (FVoxelSingleton* Singleton : Singletons)
	{
		Singleton->Initialize();
	}

	FVoxelUtilities::DelayedCall([this]
	{
		ViewExtension = FSceneViewExtensions::NewExtension<FVoxelSingletonSceneViewExtension>();

		for (FVoxelSingleton* Singleton : Singletons)
		{
			if (!Singleton->bIsRenderSingleton)
			{
				continue;
			}

			ViewExtension->Singletons.Add(static_cast<FVoxelRenderSingleton*>(Singleton));
		}
	});
}

FVoxelSingletonManager::~FVoxelSingletonManager()
{
	VOXEL_FUNCTION_COUNTER();

	if (GraphEvent.IsValid())
	{
		VOXEL_SCOPE_COUNTER("Wait");
		GraphEvent->Wait();
	}

	for (const FVoxelSingleton* Singleton : Singletons)
	{
		delete Singleton;
	}
	Singletons.Empty();

	if (ViewExtension)
	{
		ViewExtension->Singletons.Empty();
		ViewExtension.Reset();
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelSingletonManager::Tick()
{
	VOXEL_FUNCTION_COUNTER();

	for (FVoxelSingleton* Singleton : Singletons)
	{
		Singleton->Tick();
	}

	if (GraphEvent.IsValid())
	{
		VOXEL_SCOPE_COUNTER("Waiting for Tick_Async");
		GraphEvent->Wait();
	}

	GraphEvent = TGraphTask<TVoxelGraphTask<ENamedThreads::AnyBackgroundThreadNormalTask>>::CreateTask().ConstructAndDispatchWhenReady([this]
	{
		VOXEL_SCOPE_COUNTER("FVoxelSingletonManager::Tick_Async");

		for (FVoxelSingleton* Singleton : Singletons)
		{
			Singleton->Tick_Async();
		}
	});
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FString FVoxelSingletonManager::GetReferencerName() const
{
	return "FVoxelSingletonManager";
}

void FVoxelSingletonManager::AddReferencedObjects(FReferenceCollector& Collector)
{
	VOXEL_FUNCTION_COUNTER();

	for (FVoxelSingleton* Singleton : Singletons)
	{
		Singleton->AddReferencedObjects(Collector);
	}
}