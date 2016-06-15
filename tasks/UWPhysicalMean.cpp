/* Generated from orogen/lib/orogen/templates/tasks/Task.cpp */

#include "UWPhysicalMean.hpp"

using namespace uwmodem_simulation;
using namespace usbl_evologics;

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
    im_attempts1 = 1;
    im_attempts2 = 1;

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
        buffer_size_modem1 = handleRawPacket(queueRawPacket12, raw_data_input, buffer_size_modem1);

    while(_raw_data_input_modem2.read(raw_data_input) == RTT::NewData)
        buffer_size_modem2 = handleRawPacket(queueRawPacket21, raw_data_input, buffer_size_modem2);

    SendIM send_im;
    while(_message_input_modem1.read(send_im) == RTT::NewData
            && checkIMStatus(im_status_modem1))
        im_attempts1 = handleIM(send_im, im_status_modem1);

    while(_message_input_modem2.read(send_im) == RTT::NewData
            && checkIMStatus(im_status_modem2))
        im_attempts2 = handleIM(send_im, im_status_modem2);

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

    DeliveryStatus delivery_status;
    while( (delivery_status = checkDelivery(im_status_modem1, im_attempts1)) != PENDING)
    {
        if(delivery_status == DELIVERED && im_status_modem1.status == PENDING)
            _message_output_modem2.write(toReceivedIM(im_status_modem1.sendIm));
        im_status_modem1.status == delivery_status;
        _message_status_modem1.write(im_status_modem1);
        if(delivery_status == DELIVERED)
            im_status_modem1.status == EMPTY;
    }

    while( (delivery_status = checkDelivery(im_status_modem2, im_attempts2)) != PENDING)
    {
        if(delivery_status == DELIVERED && im_status_modem2.status == PENDING)
            _message_output_modem1.write(toReceivedIM(im_status_modem2.sendIm));
        im_status_modem2.status == delivery_status;
        _message_status_modem2.write(im_status_modem1);
        if(delivery_status == DELIVERED)
            im_status_modem2.status == EMPTY;
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
    im_status_modem1.status = EMPTY;
    im_status_modem2.status = EMPTY;
    im_attempts1 = 1;
    im_attempts2 = 1;
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

int UWPhysicalMean::handleRawPacket(std::queue<iodrivers_base::RawPacket> &queueRawPacket, const iodrivers_base::RawPacket &input_data, int buffer_size)
{
    iodrivers_base::RawPacket raw_data_input = input_data;
    // The input time is not used in the driver, neither transmitted as raw_data.
    // It will be used to control the transmission time.
    raw_data_input.time = base::Time::now();

    std::vector<iodrivers_base::RawPacket> tiny_packets;
    tiny_packets = breakInTinyPackets(raw_data_input, packet_size);
    for(size_t i=0; i<tiny_packets.size(); i++)
    {   // Include in buffer as much bytes as possible
        if(buffer_size > transmission_buffer_size)
            continue;
        buffer_size += tiny_packets[i].data.size();
         // Lose packets
        if(rand() % 100 > probability_good_transmission)
             continue;
        queueRawPacket.push(tiny_packets[i]);
    }
    return buffer_size;
}

bool UWPhysicalMean::checkTravelTime(base::Time init_time, base::Time travel_time)
{
    return ((base::Time::now() - init_time) >= travel_time);
}

bool UWPhysicalMean::controlBitRate(const std::vector<uint8_t> &data, base::Time last_time, int bitrate)
{
    double time_window = (base::Time::now() - last_time).toSeconds();
    return (data.size()*8/time_window <= bitrate);
}

bool UWPhysicalMean::hasRawData(const std::queue<iodrivers_base::RawPacket> &queuRawData, base::Time last_write)
{
    if(queuRawData.empty())
        return false;
    if(!controlBitRate(queuRawData.front().data, last_write, raw_bitrate))
        return false;
    if(!checkTravelTime(queuRawData.front().time, travel_time))
        return false;
    return true;
}

bool UWPhysicalMean::checkIMStatus(const MessageStatus &im_status)
{
    if(im_status.status == PENDING)
        return false;
    return true;
}

int UWPhysicalMean::handleIM(const SendIM &send_im, MessageStatus &im_status)
{
    im_status.sendIm = send_im;
    im_status.status = PENDING;
    // Used to control the transmission time.
    im_status.time = base::Time::now();
    int attempts = 1;
    while ((rand() % 100 > probability_good_transmission) && (attempts <= im_retry))
        attempts++;
    return attempts;
}

DeliveryStatus UWPhysicalMean::checkDelivery(const MessageStatus &im_status, int attempts)
{
    if(im_status.status != PENDING)
        return im_status.status;
    if(!controlBitRate(im_status.sendIm.buffer, im_status.time, im_bitrate))
        return PENDING;
    int path = im_status.sendIm.deliveryReport? (attempts*2) : 1;
    if(!checkTravelTime(im_status.time, travel_time*path))
        return PENDING;
    // Lose Instant Message in last try
    if(rand() % 100 > probability_good_transmission && attempts == im_retry+1)
        return FAILED;
    return DELIVERED;
}

ReceiveIM UWPhysicalMean::toReceivedIM(const SendIM &send_im)
{
    ReceiveIM msg;
    msg.buffer = send_im.buffer;
    msg.time = base::Time::now();
    msg.destination = send_im.destination;
    msg.source = (send_im.destination == 1)? 2 : 1;
    msg.deliveryReport = send_im.deliveryReport;
    msg.duration = msg.time - send_im.time;
    msg.rssi = -(rand() % 65 + 20);
    msg.integrity = rand() % 60 + 90;
    msg.velocity = (rand() % 1000)/1000;
    return msg;
}
