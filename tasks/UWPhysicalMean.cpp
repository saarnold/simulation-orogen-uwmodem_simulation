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

    travel_time = base::Time::fromSeconds(_distance.get()*sound_velocity);
    buffer_size_modem1 = 0;
    buffer_size_modem2 = 0;

    return true;
}
bool UWPhysicalMean::startHook()
{
    if (! UWPhysicalMeanBase::startHook())
        return false;

    // initialize timers
    last_write_raw_packet_12 = base::Time::now();
    last_write_raw_packet_21 = base::Time::now();
    last_write_im_12 = base::Time::now();
    last_write_im_21 = base::Time::now();
    return true;
}
void UWPhysicalMean::updateHook()
{
    UWPhysicalMeanBase::updateHook();

    iodrivers_base::RawPacket raw_data_input;
    while(_raw_data_input_modem1.read(raw_data_input) == RTT::NewData)
        handleRawPacket(queueRawPacket12, raw_data_input, buffer_size_modem1);

    while(_raw_data_input_modem2.read(raw_data_input) == RTT::NewData)
        handleRawPacket(queueRawPacket21, raw_data_input, buffer_size_modem2);

    while(hasRawData(queueRawPacket12, last_write_raw_packet_12))
    {
        _raw_data_output_modem2.write(queueRawPacket12.front());
        last_write_raw_packet_12 = base::Time::now();
        buffer_size_modem1 -= queueRawPacket12.front().data.size();
        queueRawPacket12.pop();
    }

    while(hasRawData(queueRawPacket21, last_write_raw_packet_21))
    {
        _raw_data_output_modem1.write(queueRawPacket12.front());
        last_write_raw_packet_21 = base::Time::now();
        buffer_size_modem2 -= queueRawPacket21.front().data.size();
        queueRawPacket21.pop();
    }

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
    queueRawPacket12 = std::queue<iodrivers_base::RawPacket>();
    queueRawPacket21 = std::queue<iodrivers_base::RawPacket>();
    buffer_size_modem1 = 0;
    buffer_size_modem2 = 0;
    queueSendIM12 = std::queue<usbl_evologics::SendIM>();
    queueSendIM21 = std::queue<usbl_evologics::SendIM>();
}

std::vector<iodrivers_base::RawPacket> UWPhysicalMean::breakInTinyPackets(const iodrivers_base::RawPacket &input_data, int packet_size)
{
    std::vector<iodrivers_base::RawPacket> output_vector;
    iodrivers_base::RawPacket output_raw = input_data;
    for( size_t i=0; i<input_data.data.size(); i+=packet_size)
    {
        output_raw.data.clear();
        if(input_data.data.size() - i < packet_size)
            output_raw.data = std::vector<uint8_t>(input_data.data.begin()+i, input_data.data.end());
        else
            output_raw.data = std::vector<uint8_t>(input_data.data.begin()+i, input_data.data.begin()+i+packet_size);
        output_vector.push_back(output_raw);
    }
    return output_vector;
}

void UWPhysicalMean::handleRawPacket(std::queue<iodrivers_base::RawPacket> &queueRawPacket, const iodrivers_base::RawPacket &input_data, int &buffer_size)
{
    iodrivers_base::RawPacket raw_data_input = input_data;
    // The input time is not used in the driver, neither transmitted as raw_data.
    // It will be used to control the transmission time.
    raw_data_input.time = base::Time::now();
    std::vector<iodrivers_base::RawPacket> tiny_packets;
    tiny_packets = breakInTinyPackets(raw_data_input, packet_size);
    for(size_t i=0; i<tiny_packets.size(); i++)
    {   // Lose packets
        if(rand() % 100 > probability_good_transmission)
             continue;
        if(buffer_size > transmission_buffer_size)
            continue;
        queueRawPacket.push(tiny_packets[i]);
        buffer_size += tiny_packets[i].data.size();
    }
}

bool UWPhysicalMean::checkTravelTime(const iodrivers_base::RawPacket &input_data, base::Time travel_time)
{
    return ((base::Time::now() - input_data.time) >= travel_time);
}

bool UWPhysicalMean::controlRawDataBitRate(const iodrivers_base::RawPacket &input_data, base::Time last_time)
{
    double time_window = (base::Time::now() - last_time).toSeconds();
    return (input_data.data.size()*8/time_window <= raw_bitrate);
}

bool UWPhysicalMean::hasRawData(const std::queue<iodrivers_base::RawPacket> &queuRawData, base::Time last_write)
{
    if(queuRawData.empty())
        return false;
    if(!controlRawDataBitRate(queuRawData.front(), last_write))
        return false;
    if(!checkTravelTime(queuRawData.front(), travel_time))
        return false;
    return true;
}
