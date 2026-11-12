# RURI
_RadarKit UHD Relay Interface_

A placeholder for digital transceiver software to interact with RadarKit, UHD USRP-N310

For now, only example codes from the UHD software to accomplish compilation and basic testing.

Try:

```
uhd_find_devices --args "${UHD_CUSTOM_ENV_ARGS}"


benchmark_rate --args "${UHD_CUSTOM_ENV_ARGS}" --duration 5 --rx_rate 0.9765625e6 --rx_otw sc16 --channels 0,1

benchmark_rate --args "${UHD_CUSTOM_ENV_ARGS}" --duration 5 --rx_rate 1.953125e6 --rx_otw sc16 --channels 0,1

benchmark_rate --args "${UHD_CUSTOM_ENV_ARGS}" --duration 5 --rx_rate 3.90625e6 --rx_otw sc16 --channels 0,1

benchmark_rate --args "${UHD_CUSTOM_ENV_ARGS}" --duration 5 --rx_rate 7.8125e6 --rx_otw sc16 --channels 0,1

benchmark_rate --args "${UHD_CUSTOM_ENV_ARGS}" --duration 5 --rx_rate 15.625e6 --rx_otw sc16 --channels 0,1
