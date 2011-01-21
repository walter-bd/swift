/*
 * Copyright (c) 2010 Kevin Smith
 * Licensed under the GNU General Public License v3.
 * See Documentation/Licenses/GPLv3.txt for more information.
 */

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include "3rdParty/hippomocks.h"

#include "Swift/Controllers/XMPPEvents/EventController.h"
#include "Swiften/Presence/DirectedPresenceSender.h"
#include "Swiften/Presence/StanzaChannelPresenceSender.h"
#include "Swiften/Avatars/NullAvatarManager.h"
#include "Swift/Controllers/Chat/MUCController.h"
#include "Swift/Controllers/UIInterfaces/ChatWindow.h"
#include "Swift/Controllers/UIInterfaces/ChatWindowFactory.h"
#include "Swiften/Client/NickResolver.h"
#include "Swiften/Roster/XMPPRoster.h"
#include "Swift/Controllers/UIEvents/UIEventStream.h"
#include "Swift/Controllers/UnitTest/MockChatWindow.h"
#include "Swiften/Client/DummyStanzaChannel.h"
#include "Swiften/Queries/DummyIQChannel.h"
#include "Swiften/Presence/PresenceOracle.h"
#include "Swiften/Network/TimerFactory.h"
#include "Swiften/Elements/MUCUserPayload.h"

using namespace Swift;

class MUCControllerTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(MUCControllerTest);
	CPPUNIT_TEST(testJoinPartStringContructionSimple);
	CPPUNIT_TEST(testJoinPartStringContructionMixed);
	CPPUNIT_TEST(testAppendToJoinParts);
	CPPUNIT_TEST(testAddressedToSelf);
	CPPUNIT_TEST(testNotAddressedToSelf);
	CPPUNIT_TEST(testAddressedToSelfBySelf);
	CPPUNIT_TEST_SUITE_END();

public:
	void setUp() {
		self_ = JID("girl@wonderland.lit/rabbithole");
		nick_ = "aLiCe";
		mocks_ = new MockRepository();
		stanzaChannel_ = new DummyStanzaChannel();
		iqChannel_ = new DummyIQChannel();
		iqRouter_ = new IQRouter(iqChannel_);
		eventController_ = new EventController();
		chatWindowFactory_ = mocks_->InterfaceMock<ChatWindowFactory>();
		presenceOracle_ = new PresenceOracle(stanzaChannel_);
		presenceSender_ = new StanzaChannelPresenceSender(stanzaChannel_);
		directedPresenceSender_ = new DirectedPresenceSender(presenceSender_);
		uiEventStream_ = new UIEventStream();
		avatarManager_ = new NullAvatarManager();
		TimerFactory* timerFactory = NULL;
		window_ = new MockChatWindow();//mocks_->InterfaceMock<ChatWindow>();
		mucRegistry_ = new MUCRegistry();
		muc_ = MUC::ref(new MUC(stanzaChannel_, iqRouter_, directedPresenceSender_, JID("teaparty@rooms.wonderland.lit"), mucRegistry_));
		mocks_->ExpectCall(chatWindowFactory_, ChatWindowFactory::createChatWindow).With(muc_->getJID(), uiEventStream_).Return(window_);
		controller_ = new MUCController (self_, muc_, nick_, stanzaChannel_, iqRouter_, chatWindowFactory_, presenceOracle_, avatarManager_, uiEventStream_, false, timerFactory, eventController_);
	};

	void tearDown() {
		delete controller_;
		delete eventController_;
		delete presenceOracle_;
		delete mocks_;
		delete uiEventStream_;
		delete stanzaChannel_;
		delete presenceSender_;
		delete directedPresenceSender_;
		delete iqRouter_;
		delete iqChannel_;
		delete mucRegistry_;
		delete avatarManager_;
	}

	void finishJoin() {
		Presence::ref presence(new Presence());
		presence->setFrom(JID(muc_->getJID().toString() + "/" + nick_));
		MUCUserPayload::ref status(new MUCUserPayload());
		MUCUserPayload::StatusCode code;
		code.code = 110;
		status->addStatusCode(code);
		presence->addPayload(status);
		stanzaChannel_->onPresenceReceived(presence);
	}

	void testAddressedToSelf() {
		finishJoin();
		Message::ref message(new Message());

		message = Message::ref(new Message());
		message->setFrom(JID(muc_->getJID().toString() + "/otherperson"));
		message->setBody("basic " + nick_ + " test.");
		message->setType(Message::Groupchat);
		controller_->handleIncomingMessage(MessageEvent::ref(new MessageEvent(message)));
		CPPUNIT_ASSERT_EQUAL((size_t)1, eventController_->getEvents().size());

		message = Message::ref(new Message());
		message->setFrom(JID(muc_->getJID().toString() + "/otherperson"));
		message->setBody(nick_ + ": hi there");
		message->setType(Message::Groupchat);
		controller_->handleIncomingMessage(MessageEvent::ref(new MessageEvent(message)));
		CPPUNIT_ASSERT_EQUAL((size_t)2, eventController_->getEvents().size());

		message->setFrom(JID(muc_->getJID().toString() + "/other"));
		message->setBody("Hi there " + nick_);
		message->setType(Message::Groupchat);
		controller_->handleIncomingMessage(MessageEvent::ref(new MessageEvent(message)));
		CPPUNIT_ASSERT_EQUAL((size_t)3, eventController_->getEvents().size());

		message = Message::ref(new Message());
		message->setFrom(JID(muc_->getJID().toString() + "/other2"));
		message->setBody("Hi " + nick_.getLowerCase() + ".");
		message->setType(Message::Groupchat);
		controller_->handleIncomingMessage(MessageEvent::ref(new MessageEvent(message)));
		CPPUNIT_ASSERT_EQUAL((size_t)4, eventController_->getEvents().size());

		message = Message::ref(new Message());
		message->setFrom(JID(muc_->getJID().toString() + "/other3"));
		message->setBody("Hi bert.");
		message->setType(Message::Groupchat);
		controller_->handleIncomingMessage(MessageEvent::ref(new MessageEvent(message)));
		CPPUNIT_ASSERT_EQUAL((size_t)4, eventController_->getEvents().size());

		message = Message::ref(new Message());
		message->setFrom(JID(muc_->getJID().toString() + "/other2"));
		message->setBody("Hi " + nick_.getLowerCase() + "ie.");
		message->setType(Message::Groupchat);
		controller_->handleIncomingMessage(MessageEvent::ref(new MessageEvent(message)));
		CPPUNIT_ASSERT_EQUAL((size_t)4, eventController_->getEvents().size());
	}

	void testNotAddressedToSelf() {
		finishJoin();
		Message::ref message(new Message());
		message->setFrom(JID(muc_->getJID().toString() + "/other3"));
		message->setBody("Hi there Hatter");
		message->setType(Message::Groupchat);
		controller_->handleIncomingMessage(MessageEvent::ref(new MessageEvent(message)));
		CPPUNIT_ASSERT_EQUAL((size_t)0, eventController_->getEvents().size());
	}

	void testAddressedToSelfBySelf() {
		finishJoin();
		Message::ref message(new Message());
		message->setFrom(JID(muc_->getJID().toString() + "/" + nick_));
		message->setBody("Hi there " + nick_);
		message->setType(Message::Groupchat);
		controller_->handleIncomingMessage(MessageEvent::ref(new MessageEvent(message)));
		CPPUNIT_ASSERT_EQUAL((size_t)0, eventController_->getEvents().size());
	}

	void checkEqual(const std::vector<NickJoinPart>& expected, const std::vector<NickJoinPart>& actual) {
		CPPUNIT_ASSERT_EQUAL(expected.size(), actual.size());
		for (size_t i = 0; i < expected.size(); i++) {
			CPPUNIT_ASSERT_EQUAL(expected[i].nick, actual[i].nick);
			CPPUNIT_ASSERT_EQUAL(expected[i].type, actual[i].type);
		}
	}

	void testAppendToJoinParts() {
		std::vector<NickJoinPart> list;
		std::vector<NickJoinPart> gold;
		MUCController::appendToJoinParts(list, NickJoinPart("Kev", Join));
		gold.push_back(NickJoinPart("Kev", Join));
		checkEqual(gold, list);
		MUCController::appendToJoinParts(list, NickJoinPart("Remko", Join));
		gold.push_back(NickJoinPart("Remko", Join));
		checkEqual(gold, list);
		MUCController::appendToJoinParts(list, NickJoinPart("Bert", Join));
		gold.push_back(NickJoinPart("Bert", Join));
		checkEqual(gold, list);
		MUCController::appendToJoinParts(list, NickJoinPart("Bert", Part));
		gold[2].type = JoinThenPart;
		checkEqual(gold, list);
		MUCController::appendToJoinParts(list, NickJoinPart("Kev", Part));
		gold[0].type = JoinThenPart;
		checkEqual(gold, list);
		MUCController::appendToJoinParts(list, NickJoinPart("Remko", Part));
		gold[1].type = JoinThenPart;
		checkEqual(gold, list);
		MUCController::appendToJoinParts(list, NickJoinPart("Ernie", Part));
		gold.push_back(NickJoinPart("Ernie", Part));
		checkEqual(gold, list);
		MUCController::appendToJoinParts(list, NickJoinPart("Ernie", Join));
		gold[3].type = PartThenJoin;
		checkEqual(gold, list);
		MUCController::appendToJoinParts(list, NickJoinPart("Kev", Join));
		gold[0].type = Join;
		checkEqual(gold, list);
		MUCController::appendToJoinParts(list, NickJoinPart("Ernie", Part));
		gold[3].type = Part;
		checkEqual(gold, list);

	}

	void testJoinPartStringContructionSimple() {
		std::vector<NickJoinPart> list;
		list.push_back(NickJoinPart("Kev", Join));
		CPPUNIT_ASSERT_EQUAL(String("Kev has joined the room."), MUCController::generateJoinPartString(list));
		list.push_back(NickJoinPart("Remko", Part));
		CPPUNIT_ASSERT_EQUAL(String("Kev has joined and Remko has left the room."), MUCController::generateJoinPartString(list));
		list.push_back(NickJoinPart("Bert", Join));
		CPPUNIT_ASSERT_EQUAL(String("Kev and Bert have joined and Remko has left the room."), MUCController::generateJoinPartString(list));
		list.push_back(NickJoinPart("Ernie", Join));
		CPPUNIT_ASSERT_EQUAL(String("Kev, Bert and Ernie have joined and Remko has left the room."), MUCController::generateJoinPartString(list));
	}

	void testJoinPartStringContructionMixed() {
		std::vector<NickJoinPart> list;
		list.push_back(NickJoinPart("Kev", JoinThenPart));
		CPPUNIT_ASSERT_EQUAL(String("Kev joined then left the room."), MUCController::generateJoinPartString(list));
		list.push_back(NickJoinPart("Remko", Part));
		CPPUNIT_ASSERT_EQUAL(String("Remko has left and Kev joined then left the room."), MUCController::generateJoinPartString(list));
		list.push_back(NickJoinPart("Bert", PartThenJoin));
		CPPUNIT_ASSERT_EQUAL(String("Remko has left, Kev joined then left and Bert left then rejoined the room."), MUCController::generateJoinPartString(list));
		list.push_back(NickJoinPart("Ernie", JoinThenPart));
		CPPUNIT_ASSERT_EQUAL(String("Remko has left, Kev and Ernie joined then left and Bert left then rejoined the room."), MUCController::generateJoinPartString(list));
	}

private:
	JID self_;
	MUC::ref muc_;
	String nick_;
	StanzaChannel* stanzaChannel_;
	IQChannel* iqChannel_;
	IQRouter* iqRouter_;
	EventController* eventController_;
	ChatWindowFactory* chatWindowFactory_;
	MUCController* controller_;
//	NickResolver* nickResolver_;
	PresenceOracle* presenceOracle_;
	AvatarManager* avatarManager_;
	StanzaChannelPresenceSender* presenceSender_;
	DirectedPresenceSender* directedPresenceSender_;
	MockRepository* mocks_;
	UIEventStream* uiEventStream_;
	MockChatWindow* window_;
	MUCRegistry* mucRegistry_;
};

CPPUNIT_TEST_SUITE_REGISTRATION(MUCControllerTest);

