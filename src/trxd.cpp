#include <uhd/convert.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/utils/cast.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/utils/thread.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/program_options.hpp>
#include <atomic>
#include <chrono>
#include <complex>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <vector>

// following benchmark_rate.cpp, using the same namespace and start_time_type for consistency
namespace po = boost::program_options;
using namespace std::chrono_literals;

namespace {
constexpr auto CLOCK_TIMEOUT = 1000ms; // 1000mS timeout for external clock locking
} // namespace

using start_time_type = std::chrono::time_point<std::chrono::steady_clock>;

inline auto time_delta(const start_time_type& ref_time)
{
    return std::chrono::steady_clock::now() - ref_time;
}

inline std::string time_delta_str(const start_time_type& ref_time)
{
    const auto delta   = time_delta(ref_time);
    const auto hours   = std::chrono::duration_cast<std::chrono::hours>(delta);
    const auto minutes = std::chrono::duration_cast<std::chrono::minutes>(delta - hours);
    const auto seconds =
        std::chrono::duration_cast<std::chrono::seconds>(delta - hours - minutes);
    const auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(
        delta - hours - minutes - seconds);

    return str(boost::format("%02d:%02d:%02d.%06d") % hours.count() % minutes.count()
               % seconds.count() % nanoseconds.count());
}

#define NOW() (time_delta_str(start_time))

void rx_worker(uhd::usrp::multi_usrp::sptr usrp,
    const std::string args,
    const start_time_type& start_time,
    std::atomic<bool>& want_active)
{
    std::cout << boost::format("[%s] Creating the RX streamer...") % NOW() << std::endl;

    // Set to maximum real-time priority
    uhd::set_thread_priority_safe(1.0F, true);

    // create a receive streamer
    uhd::stream_args_t rx_stream_args("fc32", "sc16");
    rx_stream_args.channels = {0, 1}; // may replace w/ 4-channel setup later
    rx_stream_args.args = uhd::device_addr_t(args);
    uhd::rx_streamer::sptr rx_stream = usrp->get_rx_stream(rx_stream_args);

    // print pre-test summary

    auto time_stamp   = NOW();
    auto rx_rate      = usrp->get_rx_rate() / 1e6;
    auto num_channels = rx_stream->get_num_channels();
    std::cout << boost::format("[%s] Testing receive rate %f Msps on %u channels\n") % time_stamp % rx_rate % num_channels;

    // usrp->set_rx_spp(1024);
    size_t spb = rx_stream->get_max_num_samps();
    std::cout << boost::format("[%s] Using %u samples per buffer\n") % NOW() % spb << std::endl;

    // Receive buffer
    std::vector<char> buff(spb * uhd::convert::get_bytes_per_item("fc32"));
    std::vector<void*> buffs;
    for (size_t ch = 0; ch < rx_stream->get_num_channels(); ch++)
        buffs.push_back(&buff.front()); // same buffer for each channel

    // Create a 2D vector layout: One standalone vector array per channel
    // std::vector<std::vector<std::complex<float>>> channel_buffs(
    //     rx_stream->get_num_channels(),
    //     std::vector<std::complex<float>>(spb)
    // );
    // std::vector<void*> buffs;
    // for (size_t ch = 0; ch < rx_stream->get_num_channels(); ch++) {
    //     buffs.push_back(&channel_buffs[ch].front());
    // }

    uhd::stream_cmd_t cmd(uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
    cmd.num_samps = spb;
    cmd.time_spec = usrp->get_time_now() + uhd::time_spec_t(0.1); // start in the future
    cmd.stream_now = false;
    rx_stream->issue_stream_cmd(cmd);

    uhd::rx_metadata_t md;
    std::atomic_ullong num_rx_samps{0};

    double timeout = 0.1;

    size_t count = 0;
    size_t timeout_count = 0;
    size_t overflow_count = 0;

    while (want_active) {
        try {
            size_t rx_samps_per_channel = rx_stream->recv(buffs, cmd.num_samps, md, timeout);
            num_rx_samps += rx_samps_per_channel;
        } catch (const uhd::io_error& e) {
            std::cerr << "[" << NOW() << "] Caught an IO exception. " << std::endl;
            std::cerr << e.what() << std::endl;
            return;
        }

        // handle error codes
        switch (md.error_code) {
            case uhd::rx_metadata_t::ERROR_CODE_NONE:
                count++;
                break;
            case uhd::rx_metadata_t::ERROR_CODE_TIMEOUT:
                timeout_count++;
                break;
            case uhd::rx_metadata_t::ERROR_CODE_OVERFLOW:
                overflow_count++;
                break;
            default:
                std::cerr << "[" << NOW() << "] Receiver error: " << md.strerror() << std::endl;
                std::cerr << "[" << NOW() << "] Unexpected error on recv, continuing..." << std::endl;
                break;
        }
    }
    rx_stream->issue_stream_cmd(uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS);

    std::cout << boost::format("[%s] Processed %u blocks") % NOW() % count << std::endl;
    std::cout << boost::format("[%s] timeout_count = %u") % NOW() % timeout_count << std::endl;
    std::cout << boost::format("[%s] overflow_count = %u") % NOW() % overflow_count << std::endl;
    std::cout << boost::format("[%s] Received %llu samples") % NOW() % num_rx_samps.load() << std::endl;
    std::cout << "rx_worker: want_active = false, exiting..." << std::endl;
    return;
}

// main
int UHD_SAFE_MAIN(int argc, char* argv[])
{
    const std::string help_text =
    "usage: trxd [OPTIONS]\n"
    "  This program is transceiver relay to the USRP device. More to come ...\n";

    // hard-coded for now, will replace with RadarKit's preference derived configurations
    std::string args = "type:n3xx,addr=192.168.10.2,master_clock_rate=125e6";

    std::string rx_subdev, tx_subdev;
    std::string rx_otw = "sc16";
    std::string tx_otw = "sc16";

    int verbose = 0;

    const int rx_decimation = 16;
    const int tx_decimation = 16;

    // setup the program options
    po::options_description desc("Allowed options");
    // clang-format off
    desc.add_options()
        ("help", "Show this help message and exit."),
        ("verbose", po::value<int>(&verbose)->default_value(0), "Verbose output (0-2).")
    ;
    // clang-format on
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    // print the help message
    if (vm.count("help")) {
        std::cout << help_text << std::endl;
        std::cout << desc << std::endl;
        return EXIT_SUCCESS;
    }

    std::cout << std::endl;
    uhd::device_addrs_t device_addrs = uhd::device::find(args, uhd::device::USRP);
    start_time_type start_time(std::chrono::steady_clock::now());
    std::cout << boost::format("[%s] Creating the usrp device with: %s...") % NOW() % args << std::endl;
    uhd::usrp::multi_usrp::sptr usrp = uhd::usrp::multi_usrp::make(args);

    if (verbose) {
        std::cout << boost::format("Using Device: %s") % usrp->get_pp_string() << std::endl;
        std::cout << boost::format("[%s] Setting device timestamp to 0...") % NOW() << std::endl;
    }

    // int num_mboards = usrp->get_num_mboards();

    std::atomic<bool> want_active(true);
    std::vector<std::thread> threads;

    // hard-coded for now, will replace with RadarKit's preference derived configurations
    double rx_rate = 125e6 / (double)rx_decimation;
    double tx_rate = 125e6 / (double)tx_decimation;

    usrp->set_rx_rate(rx_rate);
    usrp->set_tx_rate(tx_rate);

    // duration in seconds
    double duration = 3.14;

    // spawn a thread to handle the RX stream
    threads.emplace_back([=, &want_active]() {
        rx_worker(usrp, args, start_time, want_active);
    });
    uhd::set_thread_name(&threads.back(), "rx_worker");

    const int64_t secs  = int64_t(duration);
    const int64_t usecs = int64_t((duration - double(secs)) * 1e6);

    std::chrono::microseconds sleep_duration = std::chrono::seconds(secs) + std::chrono::microseconds(usecs);
    std::cout << "sleep_duration.count() = " << sleep_duration.count() << std::endl;

    // sleep for duration seconds, then set the want_active flag to false
    std::this_thread::sleep_for(sleep_duration);

    want_active = false;
    for (auto& t : threads) {
        t.join();
    }

    // finished
    std::cout << std::endl << "Done!" << std::endl << std::endl;

    return EXIT_SUCCESS;
}
