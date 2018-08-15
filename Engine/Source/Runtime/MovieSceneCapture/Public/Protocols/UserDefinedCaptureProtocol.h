// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "MovieSceneCaptureProtocolBase.h"
#include "FrameGrabber.h"
#include "ImageWriteStream.h"
#include "UserDefinedCaptureProtocol.generated.h"

enum class EDesiredImageFormat : uint8;
struct FImagePixelData;
struct FImageWriteOptions;
struct ICaptureProtocolHost;
class UTexture;

USTRUCT(BlueprintType)
struct FCapturedPixels
{
	GENERATED_BODY()

	TSharedPtr<FImagePixelData, ESPMode::ThreadSafe> ImageData;
};


DECLARE_DYNAMIC_DELEGATE_ThreeParams(FOnReceiveCapturedPixels, const FCapturedPixels&, Pixels, FName, StreamName, FFrameMetrics, FrameMetrics);

/**
 * A blueprintable capture protocol that defines how to capture frames from the engine
 */
UCLASS(Abstract, Blueprintable, meta=(DisplayName="Capture Protocol"))
class MOVIESCENECAPTURE_API UUserDefinedCaptureProtocol
	: public UMovieSceneImageCaptureProtocolBase
{
public:

	GENERATED_BODY()

	UUserDefinedCaptureProtocol(const FObjectInitializer& ObjInit);

protected:

	/**
	 * Called before the capture process itself is ticked, before each frame is set up for capture
	 * Useful for any pre-frame set up or resetting the previous frame's state
	 */
	UFUNCTION(BlueprintImplementableEvent, Category=Capture)
	void OnPreTick();

	/**
	 * Called after the capture process itself is ticked, after the frame is set up for capture, but before most actors have ticked
	 */
	UFUNCTION(BlueprintImplementableEvent, Category=Capture)
	void OnTick();

	/**
	 * Called once at the start of the capture process, but before any warmup frames
	 * @return true if this protocol set up successfully, false otherwise
	 */
	UFUNCTION(BlueprintNativeEvent, Category=Capture)
	bool OnSetup();
	virtual bool OnSetup_Implementation() { return true; }

	/**
	 * Called when the capture process is warming up. Protocols may transition from either an initialized, or capturing state to warm-up
	 */
	UFUNCTION(BlueprintImplementableEvent, Category=Capture)
	void OnWarmUp();

	/**
	 * Called when this protocol should start capturing frames
	 */
	UFUNCTION(BlueprintImplementableEvent, Category=Capture)
	void OnStartCapture();

	/**
	 * Called when this protocol should capture the current frame
	 */
	UFUNCTION(BlueprintImplementableEvent, Category=Capture)
	void OnCaptureFrame();

	/**
	 * Called when this protocol should temporarily stop capturing frames
	 */
	UFUNCTION(BlueprintImplementableEvent, Category=Capture)
	void OnPauseCapture();

	/**
	 * Called when this protocol is going to be shut down - should not capture any more frames
	 */
	UFUNCTION(BlueprintImplementableEvent, Category=Capture)
	void OnBeginFinalize();


	/**
	 * Called to check whether this protocol has finished any pending tasks, and can now be shut down
	 */
	UFUNCTION(BlueprintNativeEvent, Category=Capture)
	bool OnCanFinalize() const;
	virtual bool OnCanFinalize_Implementation() const { return true; }


	/**
	 * Called to complete finalization of this protocol
	 */
	UFUNCTION(BlueprintImplementableEvent, Category=Capture)
	void OnFinalize();

public:

	/**
	 * Add a handler to the specified stream that will be called whenever it receives data.
	 * Push data to a stream with either StartCapturingFinalPixels() or PushBufferToStream()
	 *
	 * @param StreamName      The name of the stream to bind this handler to. The handler will be invoked whenever pixels are received on that stream.
	 * @param Handler         A delegate for handling the pixels received on this stream name
	 */
	UFUNCTION(BlueprintCallable, Category=Capture)
	void BindToStream(FName StreamName, FOnReceiveCapturedPixels Handler);


	/*
	 * Capture the specified texture buffer using this protocol's settings
	 * 
	 * @param Buffer          The desired buffer to save
	 * @param StreamName      The name of the stream to push the buffer onto (e.g. a composition pass name). Bind to the stream with BindStreamHandler.
	 */
	UFUNCTION(BlueprintCallable, Category=Capture)
	void PushBufferToStream(UTexture* Buffer, FName StreamName);


	/*
	 * Resolve the specified buffer and pass it directly to the specified handler when done (does not pass to any bound streams)
	 * 
	 * @param Buffer          The desired buffer to save
	 * @param BufferName      The name of this buffer that is passed to the resulting handler (e.g. a composition pass name).
	 * @param Handler         A delegate for handling the pixels received on this stream name
	 */
	UFUNCTION(BlueprintCallable, Category=Capture)
	void ResolveBuffer(UTexture* Buffer, FName BufferName, FOnReceiveCapturedPixels Handler);


	/**
	 * Instruct this protocol to start capturing LDR final pixels (including slate widgets and burn-ins)
	 *
	 * @param StreamName      The name of the stream to send final pixels to. Bind to streams with BindStreamHandler.
	 */
	UFUNCTION(BlueprintCallable, Category=Capture)
	void StartCapturingFinalPixels(FName StreamName);


	/**
	 * Instruct this protocol to stop capturing LDR final pixels
	 */
	UFUNCTION(BlueprintCallable, Category=Capture)
	void StopCapturingFinalPixels();


	/**
	 * Generate a filename for the current frame
	 */
	UFUNCTION(BlueprintCallable, Category=Capture)
	virtual FString GenerateFilename(const FFrameMetrics& InFrameMetrics) const;


	/**
	 * Access this protocol's current frame metrics
	 */
	UFUNCTION(BlueprintCallable, Category=Capture)
	FFrameMetrics GetCurrentFrameMetrics() const
	{
		return CachedFrameMetrics;
	}

public:

	/**
	 * Called when image pixel data is ready to be processed
	 */
	void OnBufferReady();

protected:

	/**~ Begin UMovieSceneCaptureProtocolBase implementation */
	virtual void PreTickImpl() override;
	virtual void TickImpl() override;
	virtual bool SetupImpl() override;
	virtual void WarmUpImpl() override;
	virtual bool StartCaptureImpl() override;
	virtual void CaptureFrameImpl(const FFrameMetrics& FrameMetrics) override;
	virtual void BeginFinalizeImpl() override;
	virtual bool HasFinishedProcessingImpl() const override;
	virtual void FinalizeImpl() override;
	virtual void AddFormatMappingsImpl(TMap<FString, FStringFormatArg>& FormatMappings) const override;
	/**~ End UMovieSceneCaptureProtocolBase implementation */

	/**~ Begin UObject implementation */
	virtual UWorld* GetWorld() const override { return World; }
	/**~ End UObject implementation */

protected:

	/** World pointer assigned on Setup */
	UPROPERTY(transient, BlueprintReadOnly, Category=Capture)
	UWorld* World;

protected:

	/** A frame grabber responsible for capturing LDR final pixels from the viewport when requested */
	TUniquePtr<FFrameGrabber> FinalPixelsFrameGrabber;

	/** A running count of the number of currently pending async operations */
	TAtomic<int32> NumOutstandingOperations;

	/** Frame metrics cached for the current frame */
	FFrameMetrics CachedFrameMetrics;

	/** A map of stream names to pixel data pipes that handle each stream's pixels */
	TMap<FName, TSharedPtr<FImagePixelPipe, ESPMode::ThreadSafe>> StreamPipes;

	/** The name of final pixel streams cached from StartCapturingFinalPixels */
	FName FinalPixelsStreamName;

	/** Transient stream name used only during filename generation */
	FName CurrentStreamName;
};


/**
 * A blueprintable capture protocol tailored to capturing and exporting frames as images
 */
UCLASS(Abstract, config=EditorPerProjectUserSettings, Blueprintable, meta=(DisplayName="Capture Protocol"))
class MOVIESCENECAPTURE_API UUserDefinedImageCaptureProtocol
	: public UUserDefinedCaptureProtocol
{
public:

	GENERATED_BODY()

	UUserDefinedImageCaptureProtocol(const FObjectInitializer& ObjInit);

	/** The image format to save as */
	UPROPERTY(config, BlueprintReadWrite, EditAnywhere, Category=Capture)
	EDesiredImageFormat Format;

	/** Whether to save images with compression or not. Not supported for bitmaps. */
	UPROPERTY(config, BlueprintReadWrite, EditAnywhere, Category=Capture, meta=(InlineEditConditionToggle))
	bool bEnableCompression;

	/** The compression quality for the image type. For EXRs, 0 = Default ZIP compression, 1 = No compression, PNGs and JPEGs expect a value between 0 and 100*/
	UPROPERTY(config, BlueprintReadWrite, EditAnywhere, Category=Capture, meta=(EditCondition=bEnableCompression))
	int32 CompressionQuality;

public:


	/*
	 * Generate a filename for the specified buffer using this protocol's file name formatter
	 * 
	 * @param Buffer          The desired buffer to generate a filename for
	 * @param StreamName      The name of the stream for this buffer (e.g. a composition pass name)
	 * @return A fully qualified file name
	 */
	UFUNCTION(BlueprintCallable, Category=Capture)
	FString GenerateFilenameForBuffer(UTexture* Buffer, FName StreamName);


	/*
	 * Generate a filename for the current frame using this protocol's file name formatter
	 * 
	 * @return A fully qualified file name for the current frame number
	 */
	UFUNCTION(BlueprintCallable, Category=Capture)
	FString GenerateFilenameForCurrentFrame();


	/*
	 * Generate a filename for the current frame using this protocol's file name formatter
	 * 
	 * @return A fully qualified file name for the current frame number
	 */
	UFUNCTION(BlueprintCallable, Category=Capture)
	void WriteImageToDisk(const FCapturedPixels& PixelData, FName StreamName, const FFrameMetrics& FrameMetrics, bool bCopyImageData = false);

protected:

	/**~ Begin UMovieSceneCaptureProtocolBase implementation */
	virtual void OnReleaseConfigImpl(FMovieSceneCaptureSettings& InSettings) override;
	virtual void OnLoadConfigImpl(FMovieSceneCaptureSettings& InSettings) override;
	/**~ End UMovieSceneCaptureProtocolBase implementation */

public:

	/**
	 * Called on the main thread when an async operation dispatched from this class has completed (either successfully or otherwise)
	 */
	void OnFileWritten();

private:

	/**~ Begin UUserDefinedCaptureProtocol implementation */
	virtual FString GenerateFilename(const FFrameMetrics& InFrameMetrics) const override;
	/**~ Begin UUserDefinedCaptureProtocol implementation */
};