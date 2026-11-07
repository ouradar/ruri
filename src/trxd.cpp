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

void rx_worker(uhd::usrp::multi_usrp::sptr usrp)
{
    return;
}

// main
int UHD_SAFE_MAIN(int argc, char* argv[])
{
    const std::string help_text =
    "usage: trxd [OPTIONS]\n"
    "  This program is transceiver relay to the USRP device. More to come ...\n";

    // hard-coded for now, will replace with RadarKit's preference derived configurations
    std::string args = "type:n3xx,addr=192.168.21.109,master_clock_rate=125e6";

    std::string rx_subdev, tx_subdev;
    std::string rx_otw = "sc16";
    std::string tx_otw = "sc16";

    std::string priority;
    bool elevate_priority = false;
    int verbose = 0;

    const int rx_decimation = 16;
    const int tx_decimation = 4;

    // setup the program options
    po::options_description desc("Allowed options");
    // clang-format off
    desc.add_options()
        ("help", "Show this help message and exit."),
        ("priority", po::value<std::string>(&priority)->default_value("normal"), "Thread priority (normal, "
            "high)."),
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

    if (priority == "high") {
        uhd::set_thread_priority_safe();
        elevate_priority = true;
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

    std::atomic<bool> burst_timer_elapsed(false);
    std::vector<std::thread> threads;

    // hard-coded for now, will replace with RadarKit's preference derived configurations
    double rx_rate = 125e6 / (double)rx_decimation;
    double tx_rate = 125e6 / (double)tx_decimation;

    usrp->set_rx_rate(rx_rate);
    usrp->set_tx_rate(tx_rate);

    // create a receive streamer
    uhd::stream_args_t rx_stream_args("", rx_otw);
    rx_stream_args.channels = {0, 1}; // may replace w/ 4-channel setup later
    rx_stream_args.args     = uhd::device_addr_t("");
    uhd::rx_streamer::sptr rx_stream = usrp->get_rx_stream(rx_stream_args);

    // duration in seconds
    double duration = 3.0;

    // spawn a thread to handle the RX stream
    threads.emplace_back([=, &burst_timer_elapsed]() {
        rx_worker(usrp);
    });


    // finished
    std::cout << std::endl << "Done!" << std::endl << std::endl;

    return EXIT_SUCCESS;
}