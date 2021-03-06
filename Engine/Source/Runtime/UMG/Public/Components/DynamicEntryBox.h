// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/Widget.h"
#include "Blueprint/UserWidgetPool.h"

#include "DynamicEntryBox.generated.h"

class UUserWidget;

UENUM(BlueprintType)
enum class EDynamicBoxType : uint8
{
	Horizontal,
	Vertical,
	Wrap,
	Overlay
};

/**
 * A special box panel that auto-generates its entries at both design-time and runtime.
 * Useful for cases where you can have a varying number of entries, but it isn't worth the effort or conceptual overhead to set up a list/tile view.
 * Note that entries here are *not* virtualized as they are in the list views, so generally this should be avoided if you intend to scroll through lots of items.
 *
 * No children can be manually added in the designer - all are auto-generated based on the given entry class.
 */
UCLASS()
class UMG_API UDynamicEntryBox : public UWidget
{
	GENERATED_BODY()

public:
	UDynamicEntryBox(const FObjectInitializer& Initializer);
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

	EDynamicBoxType GetBoxType() const { return EntryBoxType; }
	TSubclassOf<UUserWidget> GetEntryWidgetClass() const { return EntryWidgetClass; }
	const FVector2D& GetEntrySpacing() const { return EntrySpacing; }
	
	template <typename WidgetT = UUserWidget>
	WidgetT* CreateEntry(const TSubclassOf<WidgetT>& ExplicitEntryClass = nullptr)
	{
		if (ExplicitEntryClass && ExplicitEntryClass->IsChildOf(WidgetT::StaticClass()))
		{
			return Cast<WidgetT>(CreateEntryInternal(ExplicitEntryClass));
		}
		if (EntryWidgetClass && EntryWidgetClass->IsChildOf(WidgetT::StaticClass()))
		{
			return Cast<WidgetT>(CreateEntryInternal(EntryWidgetClass));
		}
		return nullptr;
	}
	
	template <typename WidgetT = UUserWidget>
	void Reset(TFunctionRef<void(WidgetT&)> ResetEntryFunc, bool bDeleteWidgets = false)
	{
		for (UUserWidget* EntryWidget : EntryWidgetPool.GetActiveWidgets())
		{
			if (WidgetT* TypedEntry = Cast<WidgetT>(EntryWidget))
			{
				ResetEntryFunc(*TypedEntry);
			}
		}

		Reset(bDeleteWidgets);
	}

	/** Clear out the box entries, optionally deleting the underlying Slate widgets entirely as well. */
	UFUNCTION(BlueprintCallable, Category = DynamicEntryBox)
	void Reset(bool bDeleteWidgets = false);

	UFUNCTION(BlueprintCallable, Category = DynamicEntryBox)
	const TArray<UUserWidget*>& GetAllEntries() const;

	template <typename EntryWidgetT = UUserWidget>
	TArray<EntryWidgetT*> GetTypedEntries() const
	{
		TArray<EntryWidgetT*> TypedEntries;
		for (UUserWidget* Entry : GetAllEntries())
		{
			if (EntryWidgetT* TypedEntry = Cast<EntryWidgetT>(Entry))
			{
				TypedEntries.Add(TypedEntry);
			}
		}
		return TypedEntries;
	}

	UFUNCTION(BlueprintCallable, Category = DynamicEntryBox)
	int32 GetNumEntries() const;

	UFUNCTION(BlueprintCallable, Category = DynamicEntryBox)
	void RemoveEntry(UUserWidget* EntryWidget);

	UFUNCTION(BlueprintCallable, Category = DynamicEntryBox)
	void SetEntrySpacing(const FVector2D& InEntrySpacing);
	
	//~ Begin UWidget Interface
#if WITH_EDITOR	
	virtual const FText GetPaletteCategory() override;

	virtual void ValidateCompiledDefaults(class IWidgetCompilerLog& CompileLog) const override;
#endif
	//~ End UWidget Interface

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, Category = DynamicEntryBox, meta = (ClampMin = 0, ClampMax = 20))
	int32 NumDesignerPreviewEntries = 3;

	/** 
	 * Called whenever a preview entry is made for this widget in the designer. 
	 * Intended to allow a containing widget to do any additional modifications needed in the interest of maintaining an accurate designer preview.
	 */
	TFunction<void(UUserWidget*)> OnPreviewEntryCreatedFunc;
#endif

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	virtual void SynchronizeProperties() override;

	virtual void AddEntryChild(UUserWidget& ChildWidget);

protected:
	UUserWidget* CreateEntryInternal(TSubclassOf<UUserWidget> InEntryClass);

	/** The type of box panel into which created entries are added. Some differences in functionality exist between each type. */
	UPROPERTY(EditAnywhere, Category = DynamicEntryBox, meta = (DesignerRebuild))
	EDynamicBoxType EntryBoxType;

	/** 
	 * The padding to apply between entries in the box.
	 * Note horizontal boxes only use the X and vertical boxes only use Y. Value is also ignored for the first entry in the box.
	 * Wrap and Overlay types use both X and Y for spacing.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = EntryLayout)
	FVector2D EntrySpacing;

	//@todo DanH EntryBox: Consider giving a callback option as well/instead. Then this thing could actually create circular or pinwheel layouts...
	/** The looping sequence of entry paddings to apply as entries are created. Overlay boxes only. Ignores EntrySpacing if not empty. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = EntryLayout)
	TArray<FVector2D> SpacingPattern;

	/** Sizing rule to apply to generated entries. Horizontal/Vertical boxes only. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = EntryLayout)
	FSlateChildSize EntrySizeRule;

	/** Horizontal alignment of generated entries. Horizontal/Vertical/Wrap boxes only. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = EntryLayout)
	TEnumAsByte<EHorizontalAlignment> EntryHorizontalAlignment;

	/** Vertical alignment of generated entries. Horizontal/Vertical/Wrap boxes only. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = EntryLayout)
	TEnumAsByte<EVerticalAlignment> EntryVerticalAlignment;

	/** The maximum size of each entry in the dominant axis of the box. Vertical/Horizontal boxes only. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = EntryLayout)
	int32 MaxElementSize = 0;

	// Can be a horizontal, vertical, wrap box, or overlay
	TSharedPtr<SPanel> MyPanelWidget;

private:
	/** Creates and establishes a new dynamic entry in the box */
	UFUNCTION(BlueprintCallable, Category = DynamicEntryBox, meta = (DisplayName = "Create Entry", AllowPrivateAccess = true))
	UUserWidget* BP_CreateEntry();

	/** Creates and establishes a new dynamic entry in the box using the specified class instead of the default. */
	UFUNCTION(BlueprintCallable, Category = DynamicEntryBox, meta = (DisplayName = "Create Entry of Class", AllowPrivateAccess = true))
	UUserWidget* BP_CreateEntryOfClass(TSubclassOf<UUserWidget> EntryClass);

	FMargin BuildEntryPadding(const FVector2D& DesiredSpacing);
	
	/**
	 * The class of widget to create entries of.
	 * If natively binding this widget, use the EntryClass UPROPERTY metadata to specify the desired entry widget base class.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = EntryLayout, meta = (AllowPrivateAccess = true))
	TSubclassOf<UUserWidget> EntryWidgetClass;
	FUserWidgetPool EntryWidgetPool;

	// Let the details customization manipulate us directly
	friend class FDynamicEntryBoxDetails;
};