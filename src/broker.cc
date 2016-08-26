/**
 * @file broker.cc
 *
 * MQTT Broker (server)
 *
 * Listen for connections from clients.  Accept subscribe, unsubscribe and publish commands and forward according to
 * the [MQTT protocol](http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/mqtt-v3.1.1.html)
 * */

#include "session_manager.h"
#include "broker_session.h"

#include <event2/listener.h>

#include <csignal>
#include <cstring>

/**
 * Manange sessions for each client.
 * Sessions will persist between connections and are identified by the client id of the connecting client.
 */
SessionManager session_manager;

/**
 * Callback run when SIGINT or SIGTERM is attached, will cleanly exit.
 *
 * @param signal Integer value of signal.
 * @param event  Should be EV_SIGNAL.
 * @param arg    Pointer originally passed to evsignal_new.
 */
static void signal_cb(evutil_socket_t signal, short event, void * arg);

/**
 * Callback run when connection is received on the listening socket.
 *
 * @param listener Pointer to this listener's internal control structure.
 * @param fd       File descriptor of the newly accepted socket.
 * @param addr     Address structure for the peer.
 * @param socklen  Length of the address structure.
 * @param arg      Pointer orignally passed to evconlistener_new_bind.
 */
static void listener_cb(struct evconnlistener * listener, evutil_socket_t fd,
                        struct sockaddr * addr, int socklen, void * arg);

int main(int argc, char *argv[]) {

    struct event_base *evloop;
    struct event *signal_event;
    struct evconnlistener *listener;
    struct sockaddr_in sin;

    unsigned short listen_port = 1883;

    evloop = event_base_new();
    if (!evloop) {
        std::cerr << "Could not initialize libevent\n";
        return 1;
    }

    signal_event = evsignal_new(evloop, SIGINT, signal_cb, evloop);
    evsignal_add(signal_event, NULL);
    signal_event = evsignal_new(evloop, SIGTERM, signal_cb, evloop);
    evsignal_add(signal_event, NULL);

    std::memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(listen_port);

    listener = evconnlistener_new_bind(evloop, listener_cb, (void *) evloop,
                                       LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE, -1,
                                       (struct sockaddr *) &sin, sizeof(sin));
    if (!listener) {
        std::cerr << "Could not create a listener!\n";
        return 1;
    }

    event_base_dispatch(evloop);
    event_free(signal_event);
    evconnlistener_free(listener);
    event_base_free(evloop);

    return 0;

}

static void listener_cb(struct evconnlistener *listener, evutil_socket_t fd,
                        struct sockaddr *sa, int socklen, void *user_data) {
    struct event_base *base = static_cast<event_base *>(user_data);
    struct bufferevent *bev;

    bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    if (!bev) {
        std::cerr << "Error constructing bufferevent!\n";
        event_base_loopbreak(base);
        return;
    }

    session_manager.accept_connection(bev);
}

static void signal_cb(evutil_socket_t fd, short event, void *arg) {

    event_base *base = static_cast<event_base *>(arg);

    if (event_base_loopexit(base, NULL)) {
        std::cerr << "failed to exit event loop\n";
    }

}
