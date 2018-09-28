// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Interfaces/IBuildPatchServicesModule.h"

namespace BuildPatchTool
{
	enum class EReturnCode : int32;

	class IToolMode
	{
	public:
		TCHAR const * const EqualsStr = TEXT("=");
		TCHAR const * const QuoteStr = TEXT("\"");

	public:
		virtual EReturnCode Execute() = 0;

		/**
		 * Helper for parsing a switch from an array of switches, usually produced using FCommandLine::Parse(..)
		 *
		 * @param InSwitch      The switch name, ending with =. E.g. option=, foo=. It would usually be a compile time const.
		 * @param Value         Receives the value from the switch.
		 * @param Switches      The array of switches to search through.
		 * @return true if the switch was found.
		 */
		template <typename TValueType>
		bool ParseSwitch(const TCHAR* InSwitch, TValueType& Value, const TArray<FString>& Switches)
		{
			// Debug check requirements for InSwitch
			checkSlow(InSwitch != nullptr);
			checkSlow(InSwitch[FCString::Strlen(InSwitch)-1] == TEXT('='));

			for (const FString& Switch : Switches)
			{
				if (Switch.StartsWith(InSwitch))
				{
					FString StringValue;
					Switch.Split(EqualsStr, nullptr, &StringValue);
					return ParseValue(StringValue, Value);
				}
			}
			return false;
		}

		/**
		 * Helper for parsing an array of multiple same name switches from the full array of switches, usually produced using FCommandLine::Parse(..)
		 *
		 * @param InSwitch      The switch name, ending with =. E.g. option=, foo=. It would usually be a compile time const.
		 * @param Values        Receives the values from the switches.
		 * @param Switches      The array of switches to search through.
		 * @return true if at least one match was found.
		 */
		template <typename TValueType>
		bool ParseSwitches(const TCHAR* InSwitch, TArray<TValueType>& Values, const TArray<FString>& Switches)
		{
			// Debug check requirements for InSwitch
			checkSlow(InSwitch != nullptr);
			checkSlow(InSwitch[FCString::Strlen(InSwitch)-1] == TEXT('='));

			bool bFoundValue = false;
			for (const FString& Switch : Switches)
			{
				if (Switch.StartsWith(InSwitch))
				{
					FString StringValue;
					TValueType Value;
					Switch.Split(EqualsStr, nullptr, &StringValue);
					if (ParseValue(StringValue, Value))
					{
						Values.Emplace(MoveTemp(Value));
						bFoundValue = true;
					}
				}
			}
			return bFoundValue;
		}

		bool ParseOption(const TCHAR* InSwitch, const TArray<FString>& Switches)
		{
			return Switches.Contains(InSwitch);
		}

		bool ParseValue(const FString& ValueIn, FString& ValueOut)
		{
			ValueOut = ValueIn.TrimQuotes();
			return true;
		}

		bool ParseValue(const FString& ValueIn, uint64& ValueOut)
		{
			if (FCString::IsNumeric(*ValueIn) && !ValueIn.Contains(TEXT("-"), ESearchCase::CaseSensitive))
			{
				ValueOut = FCString::Strtoui64(*ValueIn, nullptr, 10);
				return true;
			}
			return false;
		}

		bool ParseValue(const FString& ValueIn, uint32& ValueOut)
		{
			if (FCString::IsNumeric(*ValueIn) && !ValueIn.Contains(TEXT("-"), ESearchCase::CaseSensitive))
			{
				ValueOut = (uint32)FCString::Strtoi(*ValueIn, nullptr, 10);
				return true;
			}
			return false;
		}
	};

	typedef TSharedRef<IToolMode> IToolModeRef;
	typedef TSharedPtr<IToolMode> IToolModePtr;

	class FToolModeFactory
	{
	public:
		static IToolModeRef Create(IBuildPatchServicesModule& BpsInterface);
	};
}
