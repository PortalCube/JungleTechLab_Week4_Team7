#include "FName.h"
#include "Runtime/Core/FNamePool.h"

FName::FName(const char* CharPtr)
	: FName{ FString{ CharPtr } }
{
}

FName::FName(const FString& Str)
	: Entry{ FNamePool::AddEntry(Str) }
{
}

int32 FName::Compare(const FName& Other) const
{
	if (*this == Other) { return 0; }

	const FString& ThisStr = FNamePool::GetComparisonString(Entry);
	const FString& OtherStr = FNamePool::GetComparisonString(Other.Entry);

	return ThisStr.compare(OtherStr);
}

int32 FName::CompareSensitive(const FName& Other) const
{
	if (
		Entry.DisplayBucketIndex == Other.Entry.DisplayBucketIndex &&
		Entry.DisplayIndex == Other.Entry.DisplayIndex
		)
	{

		return 0;
	}

	const FString& ThisStr = FNamePool::GetDisplayString(Entry);
	const FString& OtherStr = FNamePool::GetDisplayString(Other.Entry);

	return ThisStr.compare(OtherStr);
}

bool FName::operator==(const FName& Other) const
{
	if (
		Entry.ComparisonBucketIndex == Other.Entry.ComparisonBucketIndex &&
		Entry.ComparisonIndex == Other.Entry.ComparisonIndex
		)
	{
		
		return true;
	}
	else
	{
		return false;
	}
}

FString FName::ToString() const
{
	return FNamePool::GetDisplayString(Entry);
}