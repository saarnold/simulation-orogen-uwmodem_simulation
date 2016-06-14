/* Generated from orogen/lib/orogen/templates/tasks/Task.cpp */

#include "Task.hpp"

using namespace uwmodem_simulation;
using namespace usbl_evologics;

Task::Task(std::string const& name)
    : TaskBase(name)
{
    _interface.set(ETHERNET);
}

Task::Task(std::string const& name, RTT::ExecutionEngine* engine)
    : TaskBase(name, engine)
{
    _interface.set(ETHERNET);
}

Task::~Task()
{
}



/// The following lines are template definitions for the various state machine
// hooks defined by Orocos::RTT. See Task.hpp for more detailed
// documentation about them.

bool Task::configureHook()
{
    if (! TaskBase::configureHook())
        return false;

    interface = _interface.get();

    return true;
}
bool Task::startHook()
{
    if (! TaskBase::startHook())
        return false;
    return true;
}
void Task::updateHook()
{
    TaskBase::updateHook();
    base::Time status_period = base::Time::fromSeconds(1);

    // Output status
   if((base::Time::now() - last_status) > status_period)
   {
       last_status = base::Time::now();
    //    _message_status.write( addStatisticCounters( checkMessageStatus()));
   }
}
void Task::errorHook()
{
    TaskBase::errorHook();
}
void Task::stopHook()
{
    TaskBase::stopHook();
}
void Task::cleanupHook()
{
    TaskBase::cleanupHook();
}
