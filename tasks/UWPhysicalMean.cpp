/* Generated from orogen/lib/orogen/templates/tasks/Task.cpp */

#include "UWPhysicalMean.hpp"

using namespace uwmodem_simulation;

UWPhysicalMean::UWPhysicalMean(std::string const& name)
    : UWPhysicalMeanBase(name)
{
}

UWPhysicalMean::UWPhysicalMean(std::string const& name, RTT::ExecutionEngine* engine)
    : UWPhysicalMeanBase(name, engine)
{
}

UWPhysicalMean::~UWPhysicalMean()
{
}



/// The following lines are template definitions for the various state machine
// hooks defined by Orocos::RTT. See UWPhysicalMean.hpp for more detailed
// documentation about them.

bool UWPhysicalMean::configureHook()
{
    if (! UWPhysicalMeanBase::configureHook())
        return false;

    distance = _distance.get();
    return true;
}
bool UWPhysicalMean::startHook()
{
    if (! UWPhysicalMeanBase::startHook())
        return false;
    return true;
}
void UWPhysicalMean::updateHook()
{
    UWPhysicalMeanBase::updateHook();

    iodrivers_base::RawPacket raw_data_input;
    while(_raw_data_input_modem1.read(raw_data_input) == RTT::NewData)
        queueRawPacket12.push(raw_data_input);

    while(_raw_data_input_modem2.read(raw_data_input) == RTT::NewData)
        queueRawPacket21.push(raw_data_input);

}
void UWPhysicalMean::errorHook()
{
    UWPhysicalMeanBase::errorHook();
}
void UWPhysicalMean::stopHook()
{
    UWPhysicalMeanBase::stopHook();
}
void UWPhysicalMean::cleanupHook()
{
    UWPhysicalMeanBase::cleanupHook();
}
