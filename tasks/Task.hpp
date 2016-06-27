/* Generated from orogen/lib/orogen/templates/tasks/Task.hpp */

#ifndef UWMODEM_SIMULATION_TASK_TASK_HPP
#define UWMODEM_SIMULATION_TASK_TASK_HPP

#include "uwmodem_simulation/TaskBase.hpp"
#include "usbl_evologics/Driver.hpp"
#include <iostream>
#include <boost/random.hpp>
#include <queue>

namespace uwmodem_simulation{

    /*! \class Task
     * \brief The task context provides and requires services. It uses an ExecutionEngine to perform its functions.
     * Essential interfaces are operations, data flow ports and properties. These interfaces have been defined using the oroGen specification.
     * In order to modify the interfaces you should (re)use oroGen and rely on the associated workflow.
     *
     * \details
     * The name of a TaskContext is primarily defined via:
     \verbatim
     deployment 'deployment_name'
         task('custom_task_name','uwmodem_simulation::Task')
     end
     \endverbatim
     *  It can be dynamically adapted when the deployment is called with a prefix argument.
     */
    class Task : public TaskBase
    {
	friend class TaskBase;
    protected:

        base::Time travel_time;
        static const int sound_velocity = 1500;

        // Constant bitrate for Instant Message
        static const int im_bitrate = 976;
        // Configurable bitrate for Raw Data transmission
        int raw_bitrate;
        // Size of tiny RawPackets. Raw Data is break in tiny packets
        static const int packet_size = 20;
        // Internal buffer for Raw Data
        static const int transmission_buffer_size = 16384;
        // Account buffer used
        int buffer_size;
        // Max times a transmission of Instant Message can be performed
        int im_retry;
        // Account Instant Message attempts
        int im_attempts;

        // Bernoulli distribution stuff
        boost::random::mt19937 generator;
        boost::random::bernoulli_distribution<> dist;

        usbl_evologics::InterfaceType interface;

        // Queue of RawPacket waintg in internal buffer to be transmitted.
        std::queue<iodrivers_base::RawPacket> queueRawPacket;
        // Queue of RawPacket been transmitted
        std::queue<iodrivers_base::RawPacket> queueRawPacketOnTheWay;
        // Queue of Instant Messages from transmitter to receiver.
        std::queue<usbl_evologics::SendIM> queueSendIM;

        // Instant Message been delivered.
        usbl_evologics::MessageStatus im_status;

        static const size_t MAX_QUEUE_MSG_SIZE = 100;
        static const size_t MAX_MSG_SIZE = 64;

        base::Time last_write_raw_packet;

        base::Time status_period;
        base::Time last_status;

    public:
        /** TaskContext constructor for Task
         * \param name Name of the task. This name needs to be unique to make it identifiable via nameservices.
         * \param initial_state The initial TaskState of the TaskContext. Default is Stopped state.
         */
        Task(std::string const& name = "uwmodem_simulation::Task");

        /** TaskContext constructor for Task
         * \param name Name of the task. This name needs to be unique to make it identifiable for nameservices.
         * \param engine The RTT Execution engine to be used for this task, which serialises the execution of all commands, programs, state machines and incoming events for a task.
         *
         */
        Task(std::string const& name, RTT::ExecutionEngine* engine);

        /** Default deconstructor of Task
         */
	~Task();

        /** This hook is called by Orocos when the state machine transitions
         * from PreOperational to Stopped. If it returns false, then the
         * component will stay in PreOperational. Otherwise, it goes into
         * Stopped.
         *
         * It is meaningful only if the #needs_configuration has been specified
         * in the task context definition with (for example):
         \verbatim
         task_context "TaskName" do
           needs_configuration
           ...
         end
         \endverbatim
         */
        bool configureHook();

        /** This hook is called by Orocos when the state machine transitions
         * from Stopped to Running. If it returns false, then the component will
         * stay in Stopped. Otherwise, it goes into Running and updateHook()
         * will be called.
         */
        bool startHook();

        /** This hook is called by Orocos when the component is in the Running
         * state, at each activity step. Here, the activity gives the "ticks"
         * when the hook should be called.
         *
         * The error(), exception() and fatal() calls, when called in this hook,
         * allow to get into the associated RunTimeError, Exception and
         * FatalError states.
         *
         * In the first case, updateHook() is still called, and recover() allows
         * you to go back into the Running state.  In the second case, the
         * errorHook() will be called instead of updateHook(). In Exception, the
         * component is stopped and recover() needs to be called before starting
         * it again. Finally, FatalError cannot be recovered.
         */
        void updateHook();

        /** This hook is called by Orocos when the component is in the
         * RunTimeError state, at each activity step. See the discussion in
         * updateHook() about triggering options.
         *
         * Call recover() to go back in the Runtime state.
         */
        void errorHook();

        /** This hook is called by Orocos when the state machine transitions
         * from Running to Stopped after stop() has been called.
         */
        void stopHook();

        /** This hook is called by Orocos when the state machine transitions
         * from Stopped to PreOperational, requiring the call to configureHook()
         * before calling start() again.
         */
        void cleanupHook();

        /** Break the input RawPacket in tiny RawPackets
         *
         * @param input_data. RawPackt to be transmitted
         * @param packet_size. Max size of RawPacket
         * @return vector of RawPacket
         */
        std::vector<iodrivers_base::RawPacket> breakInTinyPackets(const iodrivers_base::RawPacket &input_data, int packet_size);

        /** Handle received RawPacket
         *
         * @param queueRawPacket. Whre the RawPacket is stored.
         * @param input_data. Data to be handled
         * @param buffer_size. Actual filled buffer
         * @return increment in buffer_size (internal buffer)
         */
        int handleRawPacket(const iodrivers_base::RawPacket &input_data, int buffer_size);

        /** Check if is possible to transmit a new RawPacket
         *
         * @return true if is possible to transmit a RawPacket
         */
        bool checkRawDataTransmission(void);

        /** Send one RawPacket
         *
         * Put it from internal buffer and on the way to receiver
         * @return decrement of buffer_size (internal buffer)
         */
        int sendOnePacket(void);

        /** Check if RawPacket arrived at destination
         *
         * @param init_time. When the transmission started
         * @param travel_time. Duration of packet transmission
         * @return bool. True if packet arrived
         */
        bool checkTravelTime(base::Time init_time, base::Time travel_time);

        /** Control bitrate of Rawpacket
         *
         * @param input_data, RawPacket to be transmitted
         * @param last_time, lat time a data was delivered
         * @param bool. If this rate respect the limit.
         */
        bool controlBitRate(const std::vector<uint8_t> &data, base::Time last_time, int bitrate);

        /** Check if RawPacket can be output at receiver
         *
         * @param queueRawData
         * @param last_write. A packet was transmitted.
         * return bool. Output RawPacket if true
         */
        bool hasRawData(const std::queue<iodrivers_base::RawPacket> &queuRawData);

        /** Check if status allows transmission of Instant MessageStatus
         *
         * @param im_status. Check status
         * @return bool. True if IM can be transmitted
         */
        bool checkIMStatus(const usbl_evologics::MessageStatus &im_status);

        /** Handle new Instat Message.
         *
         * @param send_im. New Instant Message to be enqueued
         */
         void enqueueSendIM(const usbl_evologics::SendIM &send_im);

         /** Sent an IM to the remote side and update im_status
          *
          * @im_status. To be update
          * @return update im_status
          */
         usbl_evologics::MessageStatus processIM(const usbl_evologics::MessageStatus &im_status);

         /** Define amount of attempts it will be made for delivery an IM
          *
          * @return attempts
          */
         int defineAttempts(void);

         /** Check delivery of Instant Message
          *
          * @param im_status. Track Instant Message, sending time.
          * @param attempts. How many times the message will be sent till receives an ack.
          * @return Delivery status.
          */
         usbl_evologics::DeliveryStatus checkDelivery(const usbl_evologics::MessageStatus &im_status, int attempts);

         /** Update output ports according delivery os instant message
          *
          * @im_status. To be update
          * @param delivery. FAILED or DELIVERED
          * @return update im_status
          */
         usbl_evologics::MessageStatus updateDelivery(const usbl_evologics::MessageStatus &im_status, const usbl_evologics::DeliveryStatus &delivery);

         /** Turn a SendIM in a ReceiveIM
          *
          * @param send_im. Instant Message to be converted
          * @return ReceiveIM
          */
         usbl_evologics::ReceiveIM toReceivedIM(const usbl_evologics::SendIM &send_im);
    };
}

#endif
