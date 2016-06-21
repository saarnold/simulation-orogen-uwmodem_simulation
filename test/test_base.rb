require 'minitest/spec'
require 'orocos/test/component'
require 'minitest/autorun'

describe  'uwmodem_simulation::Task' do

    include Orocos::Test::Component
    start 'simulator', 'uwmodem_simulation::Task' => 'simulator'#, :gdb => true
    writer 'simulator', 'message_input', attr_name: 'message_input', type: :buffer, size: 50
    writer 'simulator', 'raw_data_input', attr_name: 'raw_data_input', type: :buffer, size: 50
    reader 'simulator', 'message_output', attr_name: 'message_output', type: :buffer, size: 50
    reader 'simulator', 'raw_data_output', attr_name: 'raw_data_output', type: :buffer, size: 50
    reader 'simulator', 'message_status', attr_name: 'message_status', type: :buffer, size: 50

    def data(text)
        data = simulator.raw_data_input.new_sample
        data.time = Time.now
        data_array = text.bytes
        data_array.each {|byte|  data.data << byte.to_i }
        data
    end

    def msg(text)
        msg = Types::UsblEvologics::SendIM.new
        msg.time = Time.now
        msg.deliveryReport = true
        msg.destination = 2
        data_array = text.bytes
        data_array.each {|byte|  msg.buffer << byte.to_i }
        msg
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

    it 'count rate of good IM transmission' do
        prob = 70
        simulator.probability = prob
        simulator.distance = 0.01
        simulator.configure
        simulator.start

        succes = 0
        total = 10

        for i in 0...total
            message_input.write msg("message number #{i}")
            if sample = get_one_new_sample(message_output, 0.5)
                succes += 1
            end
        end
        assert_in_delta succes*100/total, prob, 30
    end

    it 'check bitrate of IM transmission' do
        simulator.distance = 0.01
        simulator.configure
        simulator.start

        message_input.write msg("message number 1")
        message_input.write msg("message number 2")
        sample1 = assert_has_one_new_sample(message_output)
        sample2 = assert_has_one_new_sample(message_output)
        bitrate = sample2.buffer.size*8 / (sample2.time - sample1.time)
        assert_operator bitrate, :<=, 976
    end

    it 'check travel time of IM transmission' do
        simulator.distance = 750
        simulator.im_retry = 0
        simulator.configure
        simulator.start

        message_input.write msg("message")
        sample = assert_has_one_new_sample(message_output)
        # Delivery and ack
        travel_time = 750*2/1500

        assert sample.duration.to_i == travel_time
    end

    it 'check travel time of raw data transmission' do
        simulator.distance = 750
        simulator.configure
        simulator.start

        data = data("message")
        raw_data_input.write data
        sample = assert_has_one_new_sample(raw_data_output, 10)
        # one way delivry
        travel_time = 750/1500

        assert (sample.time - data.time).to_i == travel_time
    end

    it 'check birate of raw data transmission in short distance' do
        simulator.distance = 0.1
        simulator.configure
        simulator.start

        string = "message"
        for i in 0..100
            string += " add more data"
        end
        data = data(string)
        raw_data_input.write data

        sample1 = assert_has_one_new_sample(raw_data_output, 10)
        sample2 = assert_has_one_new_sample(raw_data_output, 10)

        bitrate = sample2.data.size*8 / (sample2.time - sample1.time)
        assert_operator bitrate, :<=, 1600
    end

    it 'check birate of raw data transmission for long distance' do
        simulator.distance = 500
        simulator.configure
        simulator.start

        string = "message"
        for i in 0..100
            string += " add more data"
        end
        data = data(string)
        raw_data_input.write data

        sample1 = assert_has_one_new_sample(raw_data_output, 10)
        sample2 = assert_has_one_new_sample(raw_data_output, 10)

        bitrate = sample2.data.size*8 / (sample2.time - sample1.time)
        assert_operator bitrate, :<=, 1600
    end

    it 'count rate of good raw data transmission' do
        prob = 70
        simulator.probability = prob
        simulator.distance = 0.01
        simulator.configure
        simulator.start

        succes = 0
        total = 10

        for i in 0...total
            raw_data_input.write data("message number #{i}")
            if sample = get_one_new_sample(raw_data_output, 0.5)
                succes += 1
            end
        end
        assert_in_delta succes*100/total, prob, 30
    end

end
