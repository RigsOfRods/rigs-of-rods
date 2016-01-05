
/** 
    @file
    @date   01/2016
    @author Petr Ohlidal
*/

#include "Powertrain.h"

#include "BeamEngine.h"

using namespace RoR;

Powertrain::Powertrain():
    m_engine(nullptr),
    m_main_thread_index(0),
    m_sim_thread_index(1)
{
    m_state[0].Reset();
    m_state[1].Reset();

    m_command_queue[0].queue.reserve(20);
    m_command_queue[1].queue.reserve(20);
}

Powertrain::~Powertrain()
{
    delete m_engine;
}

void Powertrain::PowertrainExportState()
{
    PowertrainState state;
    state.Reset();
    
    state.conf_engine_has_turbo              = m_engine->hasTurbo();
    state.conf_engine_max_rpm                = m_engine->getMaxRPM();
    state.conf_transmission_clutch_force     = m_engine->getClutchForce();
    state.current_acceleration               = m_engine->getAcc();
    state.engine_crank_factor                = m_engine->getCrankFactor();
    state.engine_current_rpm                 = m_engine->getRPM();
    state.engine_current_torque              = m_engine->getTorque();
    state.engine_is_running                  = m_engine->isRunning();
    state.engine_starter_has_contact         = m_engine->hasContact();
    state.gfx_engine_exhaust_smoke           = m_engine->getSmoke();
    state.transmission_auto_shift_mode       = m_engine->getAutoShift();
    state.transmission_current_clutch        = m_engine->getClutch();
    state.transmission_current_gear          = m_engine->getGear();
    state.transmission_num_forward_gears     = m_engine->getNumGears();
    state.turbo_current_psi                  = m_engine->getTurboPSI();
    state.conf_engine_min_rpm                = m_engine->getMinRPM();
    state.transmission_current_gear_range    = m_engine->getGearRange();
    state.conf_transmission_num_gears_ranges = m_engine->getNumGearsRanges();

    this->GetStateOnSimThread() = state;
}

void Powertrain::PowertrainUpdate(float dt_seconds, bool just_synced)
{
    m_engine->UpdateBeamEngine(dt_seconds, just_synced);
}

void Powertrain::PowertrainProcessInput()
{
    // Process command queue
    auto& cmd_queue = this->GetQueueOnSimThread();
    auto itor = cmd_queue.queue.begin();
    auto endi = cmd_queue.queue.end();
    for (; itor != endi; ++itor)
    {
        auto cmd = *itor;
        switch (cmd.type)
        {
        case PowertrainCommand::COMMAND_AUTO_SET_ACC:
            m_engine->autoSetAcc(cmd.value);
            break;
        case PowertrainCommand::COMMAND_OFFSTART:
            m_engine->offstart();
            break;
        case PowertrainCommand::COMMAND_SET_ACC:
            m_engine->setAcc(cmd.value);
            break;
        case PowertrainCommand::COMMAND_SET_CLUTCH:
            m_engine->setClutch(cmd.value);
            break;
        case PowertrainCommand::COMMAND_SHIFT:
            m_engine->BeamEngineShift(static_cast<int>(cmd.value));
            break;
        case PowertrainCommand::COMMAND_START:
            m_engine->BeamEngineStart();
            break;
        case PowertrainCommand::COMMAND_AUTO_SHIFT_SET:
            m_engine->autoShiftSet(static_cast<int>(cmd.value));
            break;
        case PowertrainCommand::COMMAND_SET_GEAR:
            m_engine->setGear(static_cast<int>(cmd.value));
            break;
        case PowertrainCommand::COMMAND_AUTO_SHIFT_UP:
            m_engine->autoShiftUp();
            break;
        case PowertrainCommand::COMMAND_AUTO_SHIFT_DOWN:
            m_engine->autoShiftDown();
            break;
        case PowertrainCommand::COMMAND_TOGGLE_CONTACT:
            m_engine->toggleContact();
            break;
        case PowertrainCommand::COMMAND_SET_STARTER:
            m_engine->setstarter(static_cast<int>(cmd.value));
            break;
        case PowertrainCommand::COMMAND_TOGGLE_AUTO_MODE:
            m_engine->toggleAutoMode();
            break;
        case PowertrainCommand::COMMAND_SET_MANUAL_CLUTCH:
            m_engine->setManualClutch(cmd.value);
            break;
        case PowertrainCommand::COMMAND_SHIFT_TO:
            m_engine->BeamEngineShiftTo(static_cast<int>(cmd.value));
            break;
        case PowertrainCommand::COMMAND_SET_GEAR_RANGE:
            m_engine->setGearRange(static_cast<int>(cmd.value));
            break;
        }
    }

    // Process networked update
    if (cmd_queue.was_network_updated)
    {
        m_engine->netForceSettings(
            cmd_queue.net_rpm,
            cmd_queue.net_force,
            cmd_queue.net_clutch,
            cmd_queue.net_gear,
            cmd_queue.net_is_running,
            cmd_queue.net_has_contact,
            cmd_queue.net_auto_mode
            );
    }
    cmd_queue.Reset();
}

void PowertrainCommandQueue::AddCommand(PowertrainCommand::CommandType type, float value /* = 0.f */)
{
    PowertrainCommand cmd;
    cmd.type = type;
    cmd.value = value;
    queue.push_back(cmd);
}

void PowertrainCommandQueue::NetworkedUpdate(float rpm, float force, float clutch, int gear, bool _running, bool _contact, char _automode)
{
    net_rpm = rpm;
    net_force = force;
    net_clutch = clutch;
    net_gear = gear;
    net_is_running = _running;
    net_has_contact = _contact;
    net_auto_mode = _automode;

    was_network_updated = true;
}

void PowertrainCommandQueue::Reset()
{
    queue.clear(); // Doesn't affect pre-allocated capacity
    was_network_updated = false;
    net_rpm = 0.f;
    net_force = 0.f;
    net_clutch = 0.f;
    net_gear = 0;
    net_is_running = false;
    net_has_contact = false;
    net_auto_mode = 0;
}
