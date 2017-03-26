#include "lib/framework/trig.h"
#include "lib/framework/debug.h"
#include "lib/framework/crc.h"

void main()
{
	// Check tables are correct.
	uint32_t crc = ~crcSumU16(0, getSinTable().data(), getSinTable().size());
	ASSERT(crc == 0x6D3C8DB5, "Bad trigSinTable CRC = 0x%08X, sin function is broken.", crc);
	crc = ~crcSumU16(0, getATanTable().data(), getATanTable().size());
	ASSERT(crc == 0xD2797F85, "Bad trigAtanTable CRC = 0x%08X, atan function is broken.", crc);

	// Test large and small square roots.
	for (uint64_t test = 0x0000; test != 0x10000; ++test)
	{
	uint64_t lower = test * test;
	uint64_t upper = (test + 1) * (test + 1) - 1;
	ASSERT((uint32_t)iSqrt(lower) == test, "Sanity check failed, sqrt(%" PRIu64") gave %" PRIu32" instead of %" PRIu64"!", lower, i64Sqrt(lower), test);
	ASSERT((uint32_t)iSqrt(upper) == test, "Sanity check failed, sqrt(%" PRIu64") gave %" PRIu32" instead of %" PRIu64"!", upper, i64Sqrt(upper), test);
	}
	for (uint64_t test = 0xFFFE0000; test != 0x00020000; test = (test + 1) & 0xFFFFFFFF)
	{
	uint64_t lower = test * test;
	uint64_t upper = (test + 1) * (test + 1) - 1;
	ASSERT((uint32_t)i64Sqrt(lower) == test, "Sanity check failed, sqrt(%" PRIu64") gave %" PRIu32" instead of %" PRIu64"!", lower, i64Sqrt(lower), test);
	ASSERT((uint32_t)i64Sqrt(upper) == test, "Sanity check failed, sqrt(%" PRIu64") gave %" PRIu32" instead of %" PRIu64"!", upper, i64Sqrt(upper), test);
	}
}
