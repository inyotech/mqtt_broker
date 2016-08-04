#include "session.h"
#include "make_unique.h"

#include <cstring>
#include <memory>
#include <list>

#include <event2/listener.h>

std::list<std::unique_ptr<Session>> sessions;

static void listener_cb(struct evconnlistener *, evutil_socket_t,
			struct sockaddr *, int socklen, void *);

int main(int argc, char *argv[]) {

  struct event_base *evloop;
  struct evconnlistener *listener;
  struct sockaddr_in sin;
  
  unsigned short listen_port = 1883;

  evloop = event_base_new();
  if (!evloop) {
    fprintf(stderr, "Could not initialize libevent\n");
    return 1;
  }

  std::memset(&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_port = htons(listen_port);

  listener = evconnlistener_new_bind(evloop, listener_cb, (void *) evloop,
				     LEV_OPT_REUSEABLE|LEV_OPT_CLOSE_ON_FREE, -1,
				     (struct sockaddr*)&sin, sizeof(sin));
  if (!listener) {
    fprintf(stderr, "Could not create a listener!\n");
    return 1;
  }

  event_base_dispatch(evloop);

  evconnlistener_free(listener);
  event_base_free(evloop);

  printf("done\n");
  return 0;
  
}

static void listener_cb(struct evconnlistener *listener, evutil_socket_t fd,
			struct sockaddr *sa, int socklen, void *user_data)
{
  struct event_base *base = static_cast<event_base *>(user_data);
  struct bufferevent *bev;

  bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
  if (!bev) {
    fprintf(stderr, "Error constructing bufferevent!");
    event_base_loopbreak(base);
    return;
  }

  sessions.push_back(make_unique<Session>(bev, base));
  
}
