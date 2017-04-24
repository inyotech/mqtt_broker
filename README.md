# MQTT broker and clients

## About

The [MQTT (MQ Telemetry Transport) publish/subscribe
protocol](htts://mqtt.org) is a simple lightweight messaging protocol
for distributed network connected devices.  It provides low overhead,
reliable connectivity for resource constrained devices.

This is an open source, asynchronous, C++ implementation of the broker
(server) and connecting clients.  The implementation follows the 3.1.1
OASIS standard available
[here](http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html).

## Installation

### Requirements

Asynchronous networking support requires the
[Libevent](http://libevent.org) networking library.  Other than that
there are no other external run-time dependencies.

* A C++11 conformant compiler.
* [Libevent](http://libevent.org)
* [CMake](Cmhttps://cmake.org/)

Verified platforms.

* Ubuntu Linux 16.04 (gcc 5.4.0)
* Mac OSX 10.11 (llvm 7.3.0)

### Building

1. Clone this repository.
````
   $ git clone https://github.com/inyotech/mqtt_broker.git
   $ cd mqtt_broker
````

2. Install the google tests framework
```
   $ pushd test/lib
   $ git clone https://github.com/google/googletest.git
   $ popd
```

3. Create a build directory.
````
   $ mkdir build
   $ cd build
````

4. Generate build files.
````
   $ cmake ..
````

5. Build
````
   $ make
````

## Example

* Open a terminal and execute the broker.
````
   $ mqtt_broker
````

* In a second terminal execute a subscriber.
````
   $ mqtt_client_sub --topic 'a/b/c'
````

* Execute a publisher in a third terminal.
````
   $ mqtt_client_pub --topic 'a/b/c' --message 'published message'
````

## Documentation

[Doxygen](http://www.stack.nl/~dimitri/doxygen/) documentation is
available [here](https://inyotech.github.io/mqtt_broker).

## License

This software is licensed under the MIT License.  See the LICENSE.TXT file for details.

## TODO

* Client Will not implemented.
* Retained message publication not implemented.
* SSL support not implemented.
