// Copyright (c) 2026. All Rights Reserved.

#include "DIVEConvention.h"

FString DIVE::NormalizeComponentToken(FString Token)
{
	Token.RemoveFromEnd(TEXT("_GEN_VARIABLE"), ESearchCase::CaseSensitive);

	while (Token.Len() > 0)
	{
		int32 UnderscoreIndex = INDEX_NONE;
		if (!Token.FindLastChar(TEXT('_'), UnderscoreIndex) || UnderscoreIndex <= 0 || UnderscoreIndex >= Token.Len() - 1)
		{
			break;
		}

		bool bAllDigits = true;
		for (int32 Index = UnderscoreIndex + 1; Index < Token.Len(); ++Index)
		{
			if (!FChar::IsDigit(Token[Index]))
			{
				bAllDigits = false;
				break;
			}
		}

		if (!bAllDigits)
		{
			break;
		}

		Token.LeftInline(UnderscoreIndex, EAllowShrinking::No);
	}

	return Token;
}
