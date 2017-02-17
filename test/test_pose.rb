require 'minitest/spec'
require 'orocos/test/component'
require 'minitest/autorun'

describe  'uwmodem_simulation::Usbl' do

    include Orocos::Test::Component
    start 'simulator', 'uwmodem_simulation::Usbl' => 'simulator'#, :gdb => true
    writer 'simulator', 'message_input', attr_name: 'message_input', type: :buffer, size: 50
    writer 'simulator', 'raw_data_input', attr_name: 'raw_data_input', type: :buffer, size: 50
    writer 'simulator', 'local_position', attr_name: 'local_position', type: :buffer, size: 50
    writer 'simulator', 'remote_position', attr_name: 'remote_position', type: :buffer, size: 50
    reader 'simulator', 'message_output', attr_name: 'message_output', type: :buffer, size: 50
    reader 'simulator', 'raw_data_output', attr_name: 'raw_data_output', type: :buffer, size: 50
    reader 'simulator', 'message_status', attr_name: 'message_status'
    reader 'simulator', 'usbl_position_samples', attr_name: 'usbl_position_samples', type: :buffer, size: 50
    reader 'simulator', 'position_samples', attr_name: 'position_samples', type: :buffer, size: 50
    reader 'simulator', 'direction_samples', attr_name: 'direction_samples', type: :buffer, size: 50

    def data(text)
        data = simulator.raw_data_input.new_sample
        data.time = Time.now
        data_array = text.bytes
        data_array.each {|byte|  data.data << byte.to_i }
        data
    end

    def msg(text)
        msg = Types.usbl_evologics.SendIM.new
        msg.time = Time.now
        msg.deliveryReport = true
        msg.destination = 2
        data_array = text.bytes
        data_array.each {|byte|  msg.buffer << byte.to_i }
        msg
    end

    def pose(p = Types.base.Vector3d.Zero)
        position = Types.base.samples.RigidBodyState.new
        position.time = Time.now
        position.position = p
        position
    end

    def get_one_new_sample(reader, timeout = 3, poll_period = 0.01)
        Integer(timeout / poll_period).times do
            if sample = reader.read_new
                return sample
            end
            sleep poll_period
        end
        return false
    end

    it 'check position from im' do
        prob = 1
        simulator.probability = prob
        simulator.im_retry = 0
        simulator.configure
        simulator.start

        local_position.write pose
        remote_position.write pose(Types.base.Vector3d.new(750,0,0))

        message_input.write msg("message")
        sample = get_one_new_sample(message_output, 3)

        position = assert_has_one_new_sample position_samples, 3
        usbl_position = assert_has_one_new_sample usbl_position_samples

        assert_equal position.position[0], 750
        assert_equal position.position[1], 0
        assert_equal position.position[2], 0
        assert_equal position.targetFrame, simulator.target_frame
        assert_equal position.sourceFrame, simulator.source_frame
        assert_equal usbl_position.x, 750
        assert_equal usbl_position.y, 0
        assert_equal usbl_position.z, 0

    end

    it 'check position from raw data in eth' do
        prob = 1
        simulator.probability = prob
        simulator.im_retry = 0
        simulator.configure
        simulator.start

        local_position.write pose
        remote_position.write pose(Types.base.Vector3d.new(750,0,0))

        raw_data_input.write data("message_of_size_18")
        sample = get_one_new_sample(message_output, 3)
       
        total = 0
        for i in 0..5
            if  position = get_one_new_sample(position_samples)
                total += 1
            end
        end
        assert_equal total, 1
    end

    it 'check position from raw data in serial' do
        prob = 1
        simulator.probability = prob
        simulator.im_retry = 0
        simulator.receiver_interface = :SERIAL
        simulator.configure
        simulator.start

        local_position.write pose
        remote_position.write pose(Types.base.Vector3d.new(750,0,0))

        raw_data_input.write data("message_of_size_18")
        sample = get_one_new_sample(message_output, 3)
       
        total = 0
        for i in 0..5
            if  position = get_one_new_sample(position_samples)
                total += 1
            end
        end
        assert_equal total, 1
    end

end
