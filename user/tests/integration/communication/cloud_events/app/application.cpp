#include "application.h"
#include "unit-test/unit-test.h"
#include "check.h"

SYSTEM_MODE(MANUAL);

namespace {

const Serial1LogHandler logHandler(115200, LOG_LEVEL_WARN, {
	{ "app", LOG_LEVEL_ALL }
});

class RequestHandler {
public:
	void process(ctrl_request* req) {
		const int r = request(req);
		system_ctrl_set_result(req, r, nullptr, nullptr, nullptr);
	}

	static RequestHandler* instance() {
		static RequestHandler h;
		return &h;
	}

private:
	int request(ctrl_request* ctrlReq) {
		const auto jsonReq = JSONValue::parse(ctrlReq->request_data, ctrlReq->request_size);
		CHECK_TRUE(jsonReq.isObject(), SYSTEM_ERROR_BAD_DATA);
		return jsonRequest(jsonReq, ctrlReq);
	}

	int jsonRequest(const JSONValue& jsonReq, ctrl_request* ctrlReq) {
		const auto cmd = get(jsonReq, "cmd").toString();
		if (cmd == "include") {
			return include(jsonReq, ctrlReq);
		} else if (cmd == "start") {
			return start(jsonReq, ctrlReq);
		} else if (cmd == "status") {
			return status(jsonReq, ctrlReq);
		} else if (cmd.isEmpty()) {
			return 0;
		}
		return SYSTEM_ERROR_BAD_DATA;
	}

	int include(const JSONValue& jsonReq, ctrl_request* ctrlReq) {
		const auto name = get(jsonReq, "name").toString();
		CHECK_TRUE(!name.isEmpty(), SYSTEM_ERROR_BAD_DATA);
		Test::exclude("*");
		Test::include(name.data());
		return 0;
	}

	int start(const JSONValue& jsonReq, ctrl_request* ctrlReq) {
		const auto r = testCmd("start");
		if (r != 0) {
			return SYSTEM_ERROR_UNKNOWN;
		}
		return 0;
	}

	int status(const JSONValue& jsonReq, ctrl_request* ctrlReq) {
		CHECK(allocReply(ctrlReq, 128));
		auto writer = jsonWriter(ctrlReq);
		writer.beginObject()
				.name("passed").value(Test::getCurrentPassed())
				.name("failed").value(Test::getCurrentFailed())
				.name("skipped").value(Test::getCurrentSkipped())
				.name("count").value(Test::getCurrentCount())
				.endObject();
		CHECK(setReplySize(ctrlReq, writer));
		return 0;
	}

	JSONValue get(const JSONValue& obj, const char* name) {
		JSONObjectIterator it(obj);
		while (it.next()) {
			if (it.name() == name) {
				return it.value();
			}
		}
		return JSONValue();
	}

	JSONBufferWriter jsonWriter(ctrl_request* ctrlReq) {
		return JSONBufferWriter(ctrlReq->reply_data, ctrlReq->reply_size);
	}

	int setReplySize(ctrl_request* ctrlReq, const JSONBufferWriter& writer) {
		const auto size = writer.dataSize();
		if (size > ctrlReq->reply_size) {
			return SYSTEM_ERROR_TOO_LARGE;
		}
		ctrlReq->reply_size = size;
		return 0;
	}

	int allocReply(ctrl_request* ctrlReq, size_t size) {
		return system_ctrl_alloc_reply_data(ctrlReq, size, nullptr);
	}
};

} // unnamed

void ctrl_request_custom_handler(ctrl_request* req) {
	RequestHandler::instance()->process(req);
}

UNIT_TEST_APP();
