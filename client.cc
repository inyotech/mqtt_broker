//
// Created by Scott Brumbaugh on 8/11/16.
//

#include "mqtt.h"

#include <iostream>
#include <string>
#include <cstdlib>
#include <vector>

#include <getopt.h>

static void usage(void);

int main(int argc, char *argv[]) {

    static struct option longopts[] = {
            {"broker-host",   required_argument, NULL, 'b'},
            {"broker-port",   required_argument, NULL, 'p'},
            {"client-id",     required_argument, NULL, 'i'},
            {"topic",         required_argument, NULL, 't'},
            {"qos",           required_argument, NULL, 'q'},
            {"keep-alive",    required_argument, NULL, 'k'},
            {"clean-session", no_argument,       NULL, 'c'},
            {"help",          no_argument,       NULL, 'h'}
    };

    std::string broker_host;
    uint16_t broker_port = 1883;
    std::string client_id;
    std::vector<std::string> topics;
    uint16_t qos = 0;
    uint16_t keep_alive = 60;
    bool clean_session = false;

    int ch;
    while ((ch = getopt_long(argc, argv, "b:p:i:t:q:k:c", longopts, NULL)) != -1) {
        switch (ch) {
            case 'b':
                broker_host = optarg;
                break;
            case 'p':
                broker_port = atoi(optarg);
                break;
            case 'i':
                client_id = optarg;
                break;
            case 't':
                topics.push_back(optarg);
                break;
            case 'q':
                qos = atoi(optarg);
                break;
            case 'k':
                keep_alive = atoi(optarg);
                break;
            case 'c':
                clean_session = true;
                break;
            case 'h':
                usage();
                std::exit(0);
                break;
            default:
                usage();
                std::exit(1);
        }
    }

    std::cout << "broker host " << broker_host << "\n";
    std::cout << "broker port " << broker_port << "\n";
    std::cout << "client id " << client_id << "\n";
    for (auto topic : topics) {
        std::cout << "topic " << topic << "\n";
    }
    std::cout << "qos " << qos << "\n";
    std::cout << "keep alive " << keep_alive << "\n";
    std::cout << "clean session " << clean_session << "\n";

}

void usage() {
    std::cout << "usage: client [options]\n";
}
