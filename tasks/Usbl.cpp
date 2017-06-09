/* Generated from orogen/lib/orogen/templates/tasks/Task.cpp */

#include "Usbl.hpp"

using namespace uwmodem_simulation;

Usbl::Usbl(std::string const& name)
    : UsblBase(name)
{
    _min_period.set(base::Time::fromSeconds(2));
}

Usbl::Usbl(std::string const& name, RTT::ExecutionEngine* engine)
    : UsblBase(name, engine)
{
    _min_period.set(base::Time::fromSeconds(2));
}

Usbl::~Usbl()
{
}



/// The following lines are template definitions for the various state machine
// hooks defined by Orocos::RTT. See Usbl.hpp for more detailed
// documentation about them.

bool Usbl::configureHook()
{
    if (! UsblBase::configureHook())
        return false;
    return true;
}
bool Usbl::startHook()
{
    if (! UsblBase::startHook())
        return false;
    return true;
}
void Usbl::updateHook()
{
    UsblBase::updateHook();
}
void Usbl::errorHook()
{
    UsblBase::errorHook();
}
void Usbl::stopHook()
{
    UsblBase::stopHook();
}
void Usbl::cleanupHook()
{
    UsblBase::cleanupHook();
}

void Usbl::usblPosition(const base::samples::RigidBodyState &local, const base::samples::RigidBodyState &remote)
{
    if( (base::Time::now() - last_output_pose) < _min_period.get())
        return;
    last_output_pose = base::Time::now();

    if(dist(generator))
    {
        base::samples::RigidBodyState position;
        usbl_evologics::Position usbl_position;
        position.time = remote.time;
        position.position = remote.position - local.position;
        position.orientation = base::Orientation::Identity();
        position.targetFrame = _target_frame.get();
        position.sourceFrame = _source_frame.get();

        usbl_position.time = base::Time::now();
        usbl_position.measurementTime = remote.time;
        usbl_position.remoteAddress = 1;
        usbl_position.x = position.position[0];
        usbl_position.y = position.position[1];
        usbl_position.z = position.position[2];
        usbl_position.E = position.position[0];
        usbl_position.N = position.position[1];
        usbl_position.U = position.position[2];
        usbl_position.roll = 0;
        usbl_position.pitch = 0;
        usbl_position.yaw = 0;
        usbl_position.propagationTime = travel_time;
        usbl_position.rssi = -50;
        usbl_position.integrity = 80;
        usbl_position.accuracy = 0.01;
        
        _position_samples.write(position);
        _usbl_position_samples.write(usbl_position);
    }
    else
    {
        usbl_evologics::Direction direction;
        direction.time = base::Time::now();
        direction.measurementTime = remote.time;
        direction.remoteAddress = 1;
        
        base::Vector3d dir = remote.position - local.position;
        base::Vector3d projection(dir[0], dir[1], 0);
        // TODO check it
        direction.lElevation = base::Angle::vectorToVector(dir, projection).rad;
        direction.lBearing = base::Angle::vectorToVector(projection, Eigen::Vector3d::UnitX()).rad;;
        direction.bearing = direction.lBearing;
        direction.elevation = direction.lElevation;
        direction.roll = 0;
        direction.pitch = 0;
        direction.yaw = 0;
        direction.rssi = -50;
        direction.integrity = 100;
        direction.accuracy = 0.15;

        _direction_samples.write(direction);
    }
}
