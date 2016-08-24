#include "session_manager.h"
#include "session.h"

#include <event2/listener.h>

#include <csignal>
#include <cstring>

SessionManager session_manager;

static void signal_cb(evutil_socket_t, short event, void *);

static void listener_cb(struct evconnlistener *, evutil_socket_t,
                        struct sockaddr *, int socklen, void *);

int main(int argc, char *argv[]) {

    struct event_base *evloop;
    struct event * signal_event;
    struct evconnlistener *listener;
    struct sockaddr_in sin;

    unsigned short listen_port = 1883;

    evloop = event_base_new();
    if (!evloop) {
        fprintf(stderr, "Could not initialize libevent\n");
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
        fprintf(stderr, "Could not create a listener!\n");
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
        fprintf(stderr, "Error constructing bufferevent!");
        event_base_loopbreak(base);
        return;
    }

    session_manager.accept_connection(bev);
}

static void signal_cb(evutil_socket_t fd, short event, void * arg) {

    std::cout << "signal_event\n";

    event_base * base = static_cast<event_base *>(arg);

    if (event_base_loopexit(base, NULL)) {
        std::cerr << "failed to exit event loop\n";
    }

}
