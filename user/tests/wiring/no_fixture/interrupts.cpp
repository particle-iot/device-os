
#include "application.h"
#include "unit-test/unit-test.h"

// TODO for HAL_PLATFORM_NRF52840

namespace
{

class TestHandler
{
public:
	TestHandler()
	{
		++count;
	}

	TestHandler(const TestHandler&)
	{
		++count;
	}

	~TestHandler()
	{
		--count;
	}

	void operator()()
	{
	}

	static int count;
};

} // namespace
