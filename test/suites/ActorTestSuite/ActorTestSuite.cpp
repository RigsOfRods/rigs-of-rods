// Prerequisites
#include "ForwardDeclarations.h"
#include "GlobalEnvironment.h"
class GlobalEnvironment* gEnv; // global var
#include "../fakes_impl.inl"

#include <gtest/gtest.h>

// Fakes for 'BeamForcesEuler.cpp'
void Actor::ToggleSlideNodeLock(void){}
bool Actor::CalcForcesEulerPrepare(bool){return false;}
void Actor::CalcForcesEulerCompute(bool,int){}
void Actor::resetSlideNodes(void){}
void Actor::updateSlideNodePositions(void){}

SimController sc(nullptr, nullptr);

namespace {

static const Ogre::Vector3 AR_ORIGIN(9, 8, 7);

static const Ogre::Vector3 AR_NODE0_RELPOS(1, 2, 3);
static const Ogre::Vector3 AR_NODE1_RELPOS(-1, -2, -3);

static const Ogre::Vector3 AR_NODE0_ABSPOS = AR_NODE0_RELPOS + AR_ORIGIN;
static const Ogre::Vector3 AR_NODE1_ABSPOS = AR_NODE1_RELPOS + AR_ORIGIN;

void SetupTestActor(Actor& a)
{
    RoR::App::SetSimController(&sc); // TODO: do this as global environment setup

    // Basics
    a.ar_num_nodes = 2;
    a.ar_origin = AR_ORIGIN;

    // Nodes
    a.ar_nodes = new node_t[a.ar_num_nodes];

    a.ar_nodes[0].RelPosition = AR_NODE0_RELPOS;
    a.ar_nodes[1].RelPosition = AR_NODE1_RELPOS;

    a.ar_nodes[0].AbsPosition = AR_NODE0_ABSPOS;
    a.ar_nodes[1].AbsPosition = AR_NODE1_ABSPOS;
}

TEST(ActorTest, Method_ResetPosition_TranslateOnly)
{
    Actor a;
    SetupTestActor(a);
    const Ogre::Vector3 TARGET(4, 5, 6);
    a.ResetPosition(TARGET, false);

    EXPECT_EQ(a.ar_origin, AR_ORIGIN);

    const Ogre::Vector3 TRANSLATE = TARGET - AR_NODE0_ABSPOS;
    EXPECT_EQ(a.ar_nodes[0].AbsPosition, AR_NODE0_ABSPOS + TRANSLATE);
    EXPECT_EQ(a.ar_nodes[0].RelPosition, AR_NODE0_RELPOS + TRANSLATE);

    EXPECT_EQ(a.ar_nodes[1].AbsPosition, AR_NODE1_ABSPOS + TRANSLATE);
    EXPECT_EQ(a.ar_nodes[1].RelPosition, AR_NODE1_RELPOS + TRANSLATE);
}

}  // namespace
