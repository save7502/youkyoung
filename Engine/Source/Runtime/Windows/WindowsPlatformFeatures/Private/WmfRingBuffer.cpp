// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "WmfRingBuffer.h"
#include "Misc/ScopeLock.h"

DECLARE_LOG_CATEGORY_EXTERN(WmfRingBuffer, Log, VeryVerbose);
DEFINE_LOG_CATEGORY(WmfRingBuffer);

WINDOWSPLATFORMFEATURES_START

void FWmfRingBuffer::Push(const FGameplayMediaEncoderSample& Sample)
{
	check(MaxDuration != 0);

	FScopeLock Lock(&Mutex);

	if (Samples.Num() == 0 && Sample.GetType() == EMediaType::Audio)
		// start always from video sample for simplicity of `Cleanup` impl
	{
		// drop
	}
	else if (Samples.Num() == 0)
	{
		Samples.Add(Sample);
	}
	else
		// video encoding takes longer than audio thus video encoded samples arrive later and need to be placed
		// inside the container. Video also is not strictly timestamp-ordered (maybe due to B-frames)
	{
		FTimespan TimestampToInsert = Sample.GetTime();

		int i = Samples.Num() - 1;
		while (true)
		{
			if (Samples[i].GetType() == Sample.GetType() // don't change order of same stream cos video is not strictly timestamp-ordered (B-Frames)
				|| TimestampToInsert >= Samples[i].GetTime())
			{
				Samples.Insert(Sample, i + 1);
				break;
			}

			if (i == 0)
			{
				if (Sample.GetType() == EMediaType::Video)
				{
					Samples.Insert(Sample, 0);
				}
				// else: the first sample in the buffer always should be video key-frame, just drop audio sample
				// this can happen during debugging due to big gaps in timestamps

				break;
			}

			--i;
		}

		// cleanup only on receiving key-frame and only when ring-buffer if full
		if (!bCleanupPaused && Sample.IsVideoKeyFrame())
		{
			while (GetDuration() > GetMaxDuration())
			{
				Cleanup();
			}
		}
	}
}

void FWmfRingBuffer::Cleanup()
// removes a/v samples grouped by the oldest key-frame period
{
	FScopeLock Lock(&Mutex);

	check(Samples.Num() != 0);

	check(Samples[0].IsVideoKeyFrame());

	// find the second key-frame
	int i = 1; // skip the first sample which is a keyframe
	bool bFound = false;
	for (; i != Samples.Num(); ++i)
	{
		if (Samples[i].IsVideoKeyFrame())
		{
			bFound = true;
			break;
		}
	}
	check(bFound);

	// remove key-frame period
	Samples.RemoveAt(0, i, false);
	check(Samples[0].IsVideoKeyFrame());

	UE_LOG(WmfRingBuffer, VeryVerbose, TEXT("%d samples, %.3f s, %.3f - %.3f:%.3f"), Samples.Num(), GetDuration().GetTotalSeconds(), Samples[0].GetTime().GetTotalSeconds(), Samples.Last().GetTime().GetTotalSeconds(), Samples.Last().GetDuration().GetTotalSeconds());
}

void FWmfRingBuffer::PauseCleanup(bool bPause)
{
	bCleanupPaused = bPause;
}

FTimespan FWmfRingBuffer::GetDuration() const
{
	if (Samples.Num() == 0)
	{
		return 0;
	}

	return Samples.Last().GetTime() - Samples[0].GetTime();
}

TArray<FGameplayMediaEncoderSample> FWmfRingBuffer::GetCopy()
{
	FScopeLock Lock(&Mutex);

	TArray<FGameplayMediaEncoderSample> Copy;
	Copy.Reset(Samples.Num());

	for (const auto& Sample: Samples)
	{
		// cloning is required because during saving a copy of ring buffer we modify samples timestamps.
		// using samples references shared between ring buffer and its copy being saved would cause ring
		// buffer timestamp logic corruption
		Copy.Add(Sample.Clone());
	}

	return Copy;
}

void FWmfRingBuffer::Reset()
{
	FScopeLock Lock(&Mutex);
	Samples.Empty();
}

WINDOWSPLATFORMFEATURES_END

