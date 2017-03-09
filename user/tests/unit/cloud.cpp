
#include "hippomocks.h"

#include "spark_wiring_cloud.h"

#include "tools/catch.h"


bool CloudClass::publish(const char *eventName, const char *eventData, int ttl, uint32_t flags) {
	return false;
}

class TestCloudClass : public CloudClass {
public:
	void setDefaultAndAssert(MockRepository& mocks, const PublishFlag visibility, const char* name, const char* data, int ttl, uint32_t flag) {
		mocks.ExpectCallFunc(defualtPublishVisibility).Return(visibility);
		// cannot match a null pointer?
		if (data)
			mocks.ExpectCallFunc(CloudClass::publish).With(name, data, ttl, flag).Return(true);
		else
			mocks.ExpectCallFunc(CloudClass::publish).With(name, _, ttl, flag).Return(true);

	}
};

SCENARIO("defaultPublishVisibility is public") {
	REQUIRE(defualtPublishVisibility().flag()==PUBLISH_EVENT_FLAG_PRIVATE);
}

SCENARIO("Particle.publish(eventName) uses default publish when not specified") {
	GIVEN("public is default") {
		MockRepository mocks;
		TestCloudClass sut;
		sut.setDefaultAndAssert(mocks, PUBLIC, "name", nullptr, 60, PUBLISH_EVENT_FLAG_PUBLIC);
		WHEN("publishing") {
			sut.publish("name");
		}
	}

	GIVEN("private is default") {
		MockRepository mocks;
		TestCloudClass sut;
		sut.setDefaultAndAssert(mocks, PRIVATE, "name", nullptr, 60, PUBLISH_EVENT_FLAG_PRIVATE);
		WHEN("publishing") {
			sut.publish("name");
		}
	}
}


SCENARIO("Particle.publish(eventName, data) uses default publish when not specified") {
	GIVEN("public is default") {
		MockRepository mocks;
		TestCloudClass sut;
		sut.setDefaultAndAssert(mocks, PUBLIC, "name", "data", 60, PUBLISH_EVENT_FLAG_PUBLIC);
		WHEN("publishing") {
			sut.publish("name", "data");
		}
	}

	GIVEN("private is default") {
		MockRepository mocks;
		TestCloudClass sut;
		sut.setDefaultAndAssert(mocks, PRIVATE, "name", "data", 60, PUBLISH_EVENT_FLAG_PRIVATE);
		WHEN("publishing") {
			sut.publish("name", "data");
		}
	}
}

SCENARIO("Particle.publish(eventName, data, ttl) uses default publish when not specified") {
	GIVEN("public is default") {
		MockRepository mocks;
		TestCloudClass sut;
		sut.setDefaultAndAssert(mocks, PUBLIC, "name", "data", 30, PUBLISH_EVENT_FLAG_PUBLIC);
		WHEN("publishing") {
			sut.publish("name", "data", 30);
		}
	}

	GIVEN("private is default") {
		MockRepository mocks;
		TestCloudClass sut;
		sut.setDefaultAndAssert(mocks, PRIVATE, "name", "data", 30, PUBLISH_EVENT_FLAG_PRIVATE);
		WHEN("publishing") {
			sut.publish("name", "data", 30);
		}
	}
}
