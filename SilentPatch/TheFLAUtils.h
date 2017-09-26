#pragma once

#include <cstdint>

class FLAUtils
{
public:
	class int8
	{
	public:
		inline int32_t Get() const
		{
			return GetExtendedID8Func( &value );
		}

	private:
		int8() = delete;
		int8& operator =( const int8& ) = delete;

		uint8_t value;
	};

	class int16
	{
	public:
		inline int32_t Get() const
		{
			return GetExtendedID16Func( &value );
		}

	private:
		int16() = delete;
		int16& operator =( const int16& ) = delete;

		uint16_t value;
	};

	static void Init();
	static bool UsesEnhancedIMGs();

private:
	static constexpr int32_t MAX_UINT8_ID = 0xFF;
	static constexpr int32_t MAX_UINT16_ID = 0xFFFD;

	static int32_t GetExtendedID8_Stock(const uint8_t* ptr)
	{
		const uint8_t uID = *ptr;
		return uID == MAX_UINT8_ID ? -1 : uID;
	}

	static int32_t GetExtendedID16_Stock(const uint16_t* ptr)
	{
		const uint16_t uID = *ptr;
		return uID > MAX_UINT16_ID ? *reinterpret_cast<const int16_t*>(ptr) : uID;
	}

	static int32_t (*GetExtendedID8Func)(const uint8_t* ptr);
	static int32_t (*GetExtendedID16Func)(const uint16_t* ptr);

	static_assert( sizeof(int8) == sizeof(uint8_t) );
	static_assert( sizeof(int16) == sizeof(uint16_t) );
};