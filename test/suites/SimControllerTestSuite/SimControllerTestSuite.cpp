
// Prerequisites
#include "ForwardDeclarations.h"
#include "RoRPrerequisites.h"
class GlobalEnvironment* gEnv; // global var
#include "../fakes_impl.inl"

#include "OgreLodStrategyManager.h"
#include "OgreFileSystem.h"
#include "OgreDefaultHardwareBufferManager.h"

#include <gtest/gtest.h>

// Fakes for "/source/main/physics/Savegame.cpp"
bool RoR::ActorManager::LoadScene(std::string) {return false;}
bool RoR::ActorManager::SaveScene(std::string) {return false;}
std::string RoR::ActorManager::GetQuicksaveFilename(std::string) { return ""; }

// Fakes for "CacheSystem.cpp"
CacheEntry *  CacheSystem::FindEntryByFilename(std::string) { return &m_entries.at(0); }
bool          CacheSystem::CheckResourceLoaded(std::string&, std::string&) { return false; }
void CacheSystem::UnloadActorFromMemory(std::string) {}
bool CacheSystem::CheckResourceLoaded(std::string &) {return false;}
std::shared_ptr<RoR::SkinDef> CacheSystem::FetchSkinDef(class CacheEntry *) { return nullptr; }
CacheSystem::CacheSystem(void) {}

CacheEntry::CacheEntry() {}

// "Tweak" classes which give us access to protected vars/functions
//    TODO: Maybe define these as *Fake classes in *Fake headers? ~ 07/2019

class CacheSystemTweak: public CacheSystem
{
public:
    void TestTweak_AddEntry(CacheEntry e) { m_entries.push_back(e); }
};

class SimControllerTweak: public SimController
{
public:
    SimControllerTweak(): SimController(nullptr, nullptr) {}
    void TestTweak_UpdateSimulation() { this->UpdateSimulation(0.f); }
};

class OgreSceneManagerFake: public Ogre::SceneManager
{
public:
    OgreSceneManagerFake()
        : m_typename("OgreSceneManagerFake")
        , Ogre::SceneManager("rorTest")
        , m_root_scenenode(nullptr)
    {}

    Ogre::SceneNode* getRootSceneNode(void)
    {
        return &m_root_scenenode;
    }

    // pure virtuals
    const Ogre::String& getTypeName(void) const { return m_typename; }

    Ogre::SceneNode m_root_scenenode;
    Ogre::String m_typename;
};

namespace {

void AddNode(std::shared_ptr<RigDef::File> def, float x, float y, float z)
{
    RigDef::Node n;
    n.position = Ogre::Vector3(x,y,z);
    static int num = 0;
    n.id.SetNum(num++);
    static std::shared_ptr<RigDef::NodeDefaults> node_defaults = std::make_shared<RigDef::NodeDefaults>();
    n.node_defaults = node_defaults;
    static std::shared_ptr<RigDef::MinimassPreset> minimass = std::make_shared<RigDef::MinimassPreset>();
    n.node_minimass = minimass;
    def->root_module->nodes.push_back(n);
}

TEST(SimControllerTestSuite, Feat_QueueActorSpawn_Terrn2ActorRotated)
{
    // Based on "Barges (20KLpont) on BDM3-20Kleagues", see https://github.com/RigsOfRods/rigs-of-rods/issues/2334

    // How it works:
    // 1. Actor is spawned with all nodes relative to `asr_position` - see `ActorSpawner::SetupActor()`
    // 2. Actor is rotated by `asr_rotation` around `asr_position` - see `ActorSpawner::SetupActor()`
    // 3. Actor is translated to match node#0's position with `asr_position` - That's the tricky bit, see `Actor::ResetPosition()`

    GlobalEnvironment gEnvInstance;
    gEnv = &gEnvInstance; // Declared `extern` in RoRPrerequisites.h

    Ogre::LogManager* logman = OGRE_NEW Ogre::LogManager();
    logman->createLog("Feat_QueueActorSpawn_Terrn2ActorRotated.log", true, true);

    OgreSceneManagerFake sm;
    gEnv->sceneManager = &sm;

    // OGRE setup - inspired by https://github.com/OGRECave/ogre/blob/master/Tests/OgreMain/src/MeshWithoutIndexDataTests.cpp#L46
    OGRE_NEW Ogre::ResourceGroupManager();
    OGRE_NEW Ogre::LodStrategyManager();
    OGRE_NEW Ogre::ShadowTextureManager();
    auto ogreBufMgr = OGRE_NEW Ogre::DefaultHardwareBufferManager();
    auto ogreMeshMgr = OGRE_NEW Ogre::MeshManager();
    Ogre::ArchiveManager* ogreArchiveMgr = OGRE_NEW Ogre::ArchiveManager();
    ogreArchiveMgr->addArchiveFactory(OGRE_NEW Ogre::FileSystemArchiveFactory());
    Ogre::MaterialManager* matMgr = OGRE_NEW Ogre::MaterialManager();
    matMgr->initialise();

    Collisions* col = new Collisions();
    gEnv->collisions = col;

    ground_model_t gm;
    col->defaultgm = &gm;

    SimControllerTweak sc;
    RoR::App::SetSimController(&sc);

    TerrainManager tm;
    RoR::App::SetSimTerrain(&tm);

    RoR::App::CreateGuiManagerIfNotExists();

    RoR::App::gfx_shadow_type.SetActive(RoR::GfxShadowType::NONE);

    // Based on 20KLpont; extents: X [-10 10], Y [0 3], Z [-110 110]
    std::shared_ptr<RigDef::File> def = std::make_shared<RigDef::File>();
    def->root_module = std::make_shared<RigDef::File::Module>(RigDef::ROOT_MODULE_NAME);
    AddNode(def, 10, 0, -90); // Node 0 - exactly as in 20KLpont
    AddNode(def, -10, 0, -110); // Artificial low extent node
    AddNode(def, 10, 3, 110); // Artificial high extent node

    CacheEntry e;
    e.actor_def = def;
    e.fname = "dummy.truck";

    CacheSystemTweak cs;
    cs.TestTweak_AddEntry(e);
    RoR::App::SetCacheSystem(&cs);

    // TOBJ: 4946.37, 300.3, 4131.12, 0, -186.563, 0, truck 20KLpont.truck
    RoR::ActorSpawnRequest rq;
    rq.asr_origin = RoR::ActorSpawnRequest::Origin::TERRN_DEF;
    rq.asr_cache_entry = &e;
    rq.asr_position = Ogre::Vector3(4946.37, 300.3, 4131.12);
    rq.asr_rotation = Ogre::Quaternion(Ogre::Degree(0), Ogre::Vector3::UNIT_X) *
                      Ogre::Quaternion(Ogre::Degree(-186.563), Ogre::Vector3::UNIT_Y) *
                      Ogre::Quaternion(Ogre::Degree(0), Ogre::Vector3::UNIT_Z);

    RoR::App::sim_state.SetActive(RoR::SimState::PAUSED);
    sc.QueueActorSpawn(rq);
    sc.TestTweak_UpdateSimulation(); // Spawn the actor

    // Check that the actor spawned at all
    ASSERT_EQ(sc.GetActors().size(), 1);
    Actor* a = sc.GetActors().at(0);

    // Replicate the node positioning for verification
    std::vector<node_t> verify_nodes;
    for (RigDef::Node& def_node: def->root_module->nodes)
    {
        node_t n;
        // From `ActorSpawner::ProcessNode()`
        n.AbsPosition = def_node.position + rq.asr_position;
        n.RelPosition = n.AbsPosition; // ar_origin is 0,0,0
        // From `ActorManager::SetupActor()`
        n.AbsPosition = rq.asr_position + rq.asr_rotation * (n.AbsPosition - rq.asr_position); 
        n.RelPosition = n.AbsPosition; // ar_origin is 0,0,0
        verify_nodes.push_back(n);
    }

    // from `Actor::UpdateBoundingBoxes()`
    const Ogre::Vector3 AABB_PAD(0.05f, 0.05f, 0.05f); // Copy of BOUNDING_BOX_PADDING from "Beam.cpp" - FIXME: Add to BeamConstants.h and use from there!
    Ogre::AxisAlignedBox aabb = Ogre::AxisAlignedBox::BOX_NULL;
    for (node_t& n: verify_nodes)
    {
        aabb.merge(n.AbsPosition);
    }
    aabb.setMinimum(aabb.getMinimum() - AABB_PAD);
    aabb.setMaximum(aabb.getMaximum() + AABB_PAD);

    // From `ActorManager::SetupActor()`
    float px = rq.asr_position.x + (rq.asr_position.x - aabb.getCenter().x);
    float pz = rq.asr_position.z + (rq.asr_position.z - aabb.getCenter().z);
    // From `Actor::ResetPosition(float, float, bool, float)`
    // ~ Horizontal displacement
    Ogre::Vector3 horizontal_offset = Ogre::Vector3(px, verify_nodes[0].AbsPosition.y, pz) - verify_nodes[0].AbsPosition;
    // ~ Vertical displacement
    float min_height = std::numeric_limits<float>::max();
    for (int i = 0; i < a->ar_num_nodes; i++)
    {
        min_height = std::min(verify_nodes[i].AbsPosition.y, min_height);
    }
    float vertical_offset = (-min_height);
    for (int i = 0; i < a->ar_num_nodes; i++)
    {
        vertical_offset += std::max(0.0f, -(verify_nodes[i].AbsPosition.y + vertical_offset));
    }

    for (node_t& n: verify_nodes)
    {
        n.AbsPosition += horizontal_offset;
        n.AbsPosition.y += vertical_offset;
        n.RelPosition = n.AbsPosition; // ar_origin is 0,0,0
    }

    // Verify output nodes against the verification nodes
    for (size_t i = 0; i < verify_nodes.size(); ++i)
    {
        ASSERT_FLOAT_EQ(a->ar_nodes[i].AbsPosition.x, verify_nodes[i].AbsPosition.x);
        ASSERT_FLOAT_EQ(a->ar_nodes[i].AbsPosition.y, verify_nodes[i].AbsPosition.y);
        ASSERT_FLOAT_EQ(a->ar_nodes[i].AbsPosition.z, verify_nodes[i].AbsPosition.z);

        ASSERT_FLOAT_EQ(a->ar_nodes[i].RelPosition.x, verify_nodes[i].RelPosition.x);
        ASSERT_FLOAT_EQ(a->ar_nodes[i].RelPosition.y, verify_nodes[i].RelPosition.y);
        ASSERT_FLOAT_EQ(a->ar_nodes[i].RelPosition.z, verify_nodes[i].RelPosition.z);
    }

    delete col;

    OGRE_DELETE Ogre::MaterialManager::getSingletonPtr();
    OGRE_DELETE ogreArchiveMgr;
    OGRE_DELETE ogreMeshMgr;
    OGRE_DELETE ogreBufMgr;
    OGRE_DELETE Ogre::LodStrategyManager::getSingletonPtr();
    OGRE_DELETE Ogre::ResourceGroupManager::getSingletonPtr();

   // FIXME: Assertion failed: msSingleton, file ogremain\src\ogreshadowtexturemanager.cpp, line 61
   // OGRE_DELETE Ogre::ShadowTextureManager::getSingletonPtr();
}

}  // namespace
