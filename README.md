# MQTT broker and clients

The MQTT publish/subscribe protocol is a simple lightweight messaging
protocol for distributed network connected devices.

## About

Open source, asynchronous, C++ [MQ Telemetry
Transport](http://mqtt.org) client/server implementing the 3.1.1
OASIS standard available
[here](http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html).

## Installation

### Requirements

This MQTT implementation is completely asynchronous and requires the
[Libevent](http://libevent.org) networking library.  Other than that
there are no other external run-time dependencies.

* A C++11 conformant compiler.
* [Libevent](http://libevent.org)
* [CMake](Cmhttps://cmake.org/)

### Building

1. Clone this repository.
````
   $ git clone https://github.com/inyotech/mqtt_broker.git
   $ cd mqtt_broker
````

2. Create a build directory.
````
   $ mkdir build
   $ cd build
````

3. Generate build files.
````
   $ cmake ..
````

4. Build/
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
