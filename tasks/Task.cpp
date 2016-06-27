/* Generated from orogen/lib/orogen/templates/tasks/Task.cpp */

#include "Task.hpp"

using namespace uwmodem_simulation;
using namespace usbl_evologics;

Task::Task(std::string const& name)
    : TaskBase(name)
{
}

Task::Task(std::string const& name, RTT::ExecutionEngine* engine)
    : TaskBase(name, engine)
{
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

    interface = _receiver_interface.get();
    status_period = base::Time::fromSeconds(1);
    travel_time = base::Time::fromSeconds(_distance.get()/sound_velocity);
    boost::random::bernoulli_distribution<> bernoulli((double)_probability.get());
    dist = bernoulli;
    im_retry = _im_retry.get();
    im_status.status = EMPTY;
    raw_bitrate = _bitrate.get();
    last_write_raw_packet = base::Time::fromSeconds(0);

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

    /**
     * Output status
     */
    if((base::Time::now() - last_status) > status_period)
    {
        last_status = base::Time::now();
        // Keep delivery time in im_status
        MessageStatus status = im_status;
        status.time = base::Time::now();
        _message_status.write(status);
    }

    /**
     * Transmission of Raw Data
     */
    iodrivers_base::RawPacket raw_data_input;
    while(_raw_data_input.read(raw_data_input) == RTT::NewData)
        buffer_size += handleRawPacket(raw_data_input, buffer_size);
    while(checkRawDataTransmission())
        buffer_size -= sendOnePacket();
    while(hasRawData(queueRawPacketOnTheWay))
    {
        _raw_data_output.write(queueRawPacketOnTheWay.front());
        queueRawPacketOnTheWay.pop();
    }

    /**
     * Transmission of Instant Message
     */
    SendIM send_im;
    while(_message_input.read(send_im) == RTT::NewData)
        enqueueSendIM(send_im);
    while(checkIMStatus(im_status))
    {
        im_attempts = defineAttempts();
        im_status = processIM(im_status);
    }
    DeliveryStatus delivery_status;
    while( (delivery_status = checkDelivery(im_status, im_attempts)) != PENDING
            && im_status.status == PENDING)
            im_status = updateDelivery(im_status, delivery_status);

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
    queueRawPacket = std::queue<iodrivers_base::RawPacket>();
    queueSendIM = std::queue<usbl_evologics::SendIM>();
    // queueRawPacketOnTheWay is not empty.
    // Well, it's on the way, the receiver must deal with this data. ¯\_(ツ)_/¯
    buffer_size = 0;
    im_status.status = EMPTY;
    last_write_raw_packet = base::Time::fromSeconds(0);
}


std::vector<iodrivers_base::RawPacket> Task::breakInTinyPackets(const iodrivers_base::RawPacket &input_data, int packet_size)
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

int Task::handleRawPacket(const iodrivers_base::RawPacket &input_data, int buffer_size)
{
    iodrivers_base::RawPacket raw_data_input = input_data;
    int increment = 0;
    std::vector<iodrivers_base::RawPacket> tiny_packets;
    tiny_packets = breakInTinyPackets(raw_data_input, (interface == SERIAL)? 2 : packet_size);
    for(size_t i=0; i<tiny_packets.size(); i++)
    {   // Include in buffer as much bytes as possible
        if(buffer_size+increment >= transmission_buffer_size)
            continue;
        increment += tiny_packets[i].data.size();
        queueRawPacket.push(tiny_packets[i]);
    }
    return increment;
}

bool Task::checkRawDataTransmission(void)
{
    if(queueRawPacket.empty())
        return false;
    if(last_write_raw_packet.toSeconds() == 0)
        return true;
    if(!controlBitRate(queueRawPacket.front().data, last_write_raw_packet, raw_bitrate))
        return false;
    return true;
}

int Task::sendOnePacket(void)
{
    if(queueRawPacket.empty())
        return 0;
    iodrivers_base::RawPacket on_the_way = queueRawPacket.front();
    on_the_way.time = base::Time::now();
    last_write_raw_packet = on_the_way.time;
    // Don't lose packet
   if(dist(generator))
        queueRawPacketOnTheWay.push(on_the_way);
    queueRawPacket.pop();
    return on_the_way.data.size();
}

bool Task::checkTravelTime(base::Time init_time, base::Time travel_time)
{
    return ((base::Time::now() - init_time) >= travel_time);
}

bool Task::controlBitRate(const std::vector<uint8_t> &data, base::Time last_time, int bitrate)
{
    double time_window = (base::Time::now() - last_time).toSeconds();
    return (data.size()*8/time_window <= bitrate);
}

bool Task::hasRawData(const std::queue<iodrivers_base::RawPacket> &queuRawData)
{
    if(queuRawData.empty())
        return false;
    if(!checkTravelTime(queuRawData.front().time, travel_time))
        return false;
    return true;
}

bool Task::checkIMStatus(const MessageStatus &im_status)
{
    if(im_status.status == PENDING)
        return false;
    if(queueSendIM.empty())
        return false;
    return true;
}

void Task::enqueueSendIM(const SendIM &send_im)
{
    // Check size of Message. It can't be bigger than MAX_MSG_SIZE, according device's manual.
    if(send_im.buffer.size() > MAX_MSG_SIZE)
    {
        if(state() != HUGE_INSTANT_MESSAGE)
            state(HUGE_INSTANT_MESSAGE);
        RTT::log(RTT::Error) << "Usbl_evologics Task.cpp. HUGE_INSTANT_MESSAGE. Message discharged: \""
                             << UsblParser::printBuffer(send_im.buffer)
                             << "\"" << std::endl;
        return;
    }
    // Check queue size.
    if(queueSendIM.size() > MAX_QUEUE_MSG_SIZE)
    {
        if(state() != FULL_IM_QUEUE)
            state(FULL_IM_QUEUE);
        RTT::log(RTT::Error) << "Usbl_evologics Task.cpp. FULL_IM_QUEUE. Message discharged: \""
                             << UsblParser::printBuffer(send_im.buffer)
                             << "\"" << std::endl;
        return;
    }
    if(state() != RUNNING)
        state(RUNNING);
    queueSendIM.push(send_im);
}

MessageStatus Task::processIM(const MessageStatus &im_status)
{
    if(queueSendIM.empty())
        throw std::runtime_error("uwmodem_simulation::Task.cpp: processIM: queueSendIM is empty");
    MessageStatus status = im_status;
    status.sendIm = queueSendIM.front();
    status.status = PENDING;
    // Used to control the transmission time.
    status.time = base::Time::now();
    status.messageSent++;
    queueSendIM.pop();
    return status;
}

int Task::defineAttempts(void)
{
    int attempts = 1;
    while (dist(generator) && (attempts <= im_retry))
        attempts++;
    return attempts;
}

DeliveryStatus Task::checkDelivery(const MessageStatus &im_status, int attempts)
{
    if(im_status.status != PENDING)
        return im_status.status;
    if(!controlBitRate(im_status.sendIm.buffer, im_status.time, im_bitrate))
        return PENDING;
    int path = im_status.sendIm.deliveryReport? (attempts*2) : 1;
    if(!checkTravelTime(im_status.time, travel_time*path))
        return PENDING;
    // Lose Instant Message in last try
    if(!dist(generator) && attempts == im_retry+1)
        return FAILED;
    return DELIVERED;
}

MessageStatus Task::updateDelivery(const MessageStatus &im_status, const DeliveryStatus &delivery)
{
    MessageStatus status = im_status;
    if(delivery == DELIVERED)
    {
        _message_output.write(toReceivedIM(im_status.sendIm));
        status.messageDelivered++;
    }
    if(delivery == FAILED)
        status.messageFailed++;

    status.status = delivery;
    status.time = base::Time::now();
    _message_status.write(status);

    if(delivery == DELIVERED)
        status.status = EMPTY;
    status.sendIm = SendIM();
    return status;
}

ReceiveIM Task::toReceivedIM(const SendIM &send_im)
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
