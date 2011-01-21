/*
 * Copyright (c) 2010 Remko Tronçon
 * Licensed under the GNU General Public License v3.
 * See Documentation/Licenses/GPLv3.txt for more information.
 */

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>

#include "Swiften/Queries/GenericRequest.h"
#include "Swiften/Queries/IQRouter.h"
#include "Swiften/Queries/DummyIQChannel.h"
#include "Swiften/Elements/Payload.h"

using namespace Swift;

class RequestTest : public CppUnit::TestFixture {
		CPPUNIT_TEST_SUITE(RequestTest);
		CPPUNIT_TEST(testSendGet);
		CPPUNIT_TEST(testSendSet);
		CPPUNIT_TEST(testHandleIQ);
		CPPUNIT_TEST(testHandleIQ_InvalidID);
		CPPUNIT_TEST(testHandleIQ_Error);
		CPPUNIT_TEST(testHandleIQ_ErrorWithoutPayload);
		CPPUNIT_TEST(testHandleIQ_BeforeSend);
		CPPUNIT_TEST_SUITE_END();

	public:
		class MyPayload : public Payload {
			public:
				MyPayload(const String& s = "") : text_(s) {}
				String text_;
		};

		typedef GenericRequest<MyPayload> MyRequest;

	public:
		void setUp() {
			channel_ = new DummyIQChannel();
			router_ = new IQRouter(channel_);
			payload_ = boost::shared_ptr<Payload>(new MyPayload("foo"));
			responsePayload_ = boost::shared_ptr<Payload>(new MyPayload("bar"));
			responsesReceived_ = 0;
		}

		void tearDown() {
			delete router_;
			delete channel_;
		}

		void testSendSet() {
			MyRequest testling(IQ::Set, JID("foo@bar.com/baz"), payload_, router_);
			testling.send();

			CPPUNIT_ASSERT_EQUAL(1, static_cast<int>(channel_->iqs_.size()));
			CPPUNIT_ASSERT_EQUAL(JID("foo@bar.com/baz"), channel_->iqs_[0]->getTo());
			CPPUNIT_ASSERT_EQUAL(IQ::Set, channel_->iqs_[0]->getType());
			CPPUNIT_ASSERT_EQUAL(String("test-id"), channel_->iqs_[0]->getID());
		}

		void testSendGet() {
			MyRequest testling(IQ::Get, JID("foo@bar.com/baz"), payload_, router_);
			testling.send();

			CPPUNIT_ASSERT_EQUAL(1, static_cast<int>(channel_->iqs_.size()));
			CPPUNIT_ASSERT_EQUAL(IQ::Get, channel_->iqs_[0]->getType());
		}

		void testHandleIQ() {
			MyRequest testling(IQ::Get, JID("foo@bar.com/baz"), payload_, router_);
			testling.onResponse.connect(boost::bind(&RequestTest::handleResponse, this, _1, _2));
			testling.send();
			
			channel_->onIQReceived(createResponse("test-id"));

			CPPUNIT_ASSERT_EQUAL(1, responsesReceived_);
			CPPUNIT_ASSERT_EQUAL(0, static_cast<int>(receivedErrors.size()));
			CPPUNIT_ASSERT_EQUAL(1, static_cast<int>(channel_->iqs_.size()));
		}

		// FIXME: Doesn't test that it didn't handle the payload
		void testHandleIQ_InvalidID() {
			MyRequest testling(IQ::Get, JID("foo@bar.com/baz"), payload_, router_);
			testling.onResponse.connect(boost::bind(&RequestTest::handleResponse, this, _1, _2));
			testling.send();

			channel_->onIQReceived(createResponse("different-id"));

			CPPUNIT_ASSERT_EQUAL(0, responsesReceived_);
			CPPUNIT_ASSERT_EQUAL(0, static_cast<int>(receivedErrors.size()));
			CPPUNIT_ASSERT_EQUAL(1, static_cast<int>(channel_->iqs_.size()));
		}

		void testHandleIQ_Error() {
			MyRequest testling(IQ::Get, JID("foo@bar.com/baz"), payload_, router_);
			testling.onResponse.connect(boost::bind(&RequestTest::handleResponse, this, _1, _2));
			testling.send();

			boost::shared_ptr<IQ> error = createError("test-id");
			boost::shared_ptr<Payload> errorPayload = boost::shared_ptr<ErrorPayload>(new ErrorPayload(ErrorPayload::FeatureNotImplemented));
			error->addPayload(errorPayload);
			channel_->onIQReceived(error);

			CPPUNIT_ASSERT_EQUAL(0, responsesReceived_);
			CPPUNIT_ASSERT_EQUAL(1, static_cast<int>(receivedErrors.size()));
			CPPUNIT_ASSERT_EQUAL(1, static_cast<int>(channel_->iqs_.size()));
			CPPUNIT_ASSERT_EQUAL(ErrorPayload::FeatureNotImplemented, receivedErrors[0].getCondition());
		}

		void testHandleIQ_ErrorWithoutPayload() {
			MyRequest testling(IQ::Get, JID("foo@bar.com/baz"), payload_, router_);
			testling.onResponse.connect(boost::bind(&RequestTest::handleResponse, this, _1, _2));
			testling.send();

			channel_->onIQReceived(createError("test-id"));

			CPPUNIT_ASSERT_EQUAL(0, responsesReceived_);
			CPPUNIT_ASSERT_EQUAL(1, static_cast<int>(receivedErrors.size()));
			CPPUNIT_ASSERT_EQUAL(1, static_cast<int>(channel_->iqs_.size()));
			CPPUNIT_ASSERT_EQUAL(ErrorPayload::UndefinedCondition, receivedErrors[0].getCondition());
		}

		void testHandleIQ_BeforeSend() {
			MyRequest testling(IQ::Get, JID("foo@bar.com/baz"), payload_, router_);
			testling.onResponse.connect(boost::bind(&RequestTest::handleResponse, this, _1, _2));
			channel_->onIQReceived(createResponse("test-id"));

			CPPUNIT_ASSERT_EQUAL(0, responsesReceived_);
			CPPUNIT_ASSERT_EQUAL(0, static_cast<int>(receivedErrors.size()));
			CPPUNIT_ASSERT_EQUAL(0, static_cast<int>(channel_->iqs_.size()));
		}
	
	private:
		void handleResponse(boost::shared_ptr<Payload> p, ErrorPayload::ref e) {
			if (e) {
				receivedErrors.push_back(*e);
			}
			else {
				boost::shared_ptr<MyPayload> payload(boost::dynamic_pointer_cast<MyPayload>(p));
				CPPUNIT_ASSERT(payload);
				CPPUNIT_ASSERT_EQUAL(String("bar"), payload->text_);
				++responsesReceived_;
			}
		}

		boost::shared_ptr<IQ> createResponse(const String& id) {
			boost::shared_ptr<IQ> iq(new IQ(IQ::Result));
			iq->addPayload(responsePayload_);
			iq->setID(id);
			return iq;
		}

		boost::shared_ptr<IQ> createError(const String& id) {
			boost::shared_ptr<IQ> iq(new IQ(IQ::Error));
			iq->setID(id);
			return iq;
		}
	
	private:
		IQRouter* router_;
		DummyIQChannel* channel_;
		boost::shared_ptr<Payload> payload_;
		boost::shared_ptr<Payload> responsePayload_;
		int responsesReceived_;
		std::vector<ErrorPayload> receivedErrors;
};

CPPUNIT_TEST_SUITE_REGISTRATION(RequestTest);
