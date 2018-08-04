
#include "im_event.hpp"

#include <vector>
#include <list>
#include <cstdio>
using namespace std;

static int g_ep = -1;
static epoll_event g_event_list[MAX_EPOLL_EVENTS];
static unsigned int g_nevents;

static im_event_status im_epoll_process_events(int timer) {
	int events;

	events = epoll_wait(g_ep, g_event_list, g_nevents, timer);
	if(events == -1) {
		perror("epoll wait failed");
		return IM_EVENT_ERROR;
	}

	if(events == 0) {
		if(timer < 0) { 		//timeout
			return IM_EVENT_OK;
		}
		return IM_EVENT_ERROR;
	}

	for(i = 0; i < events; i++) {
		
	}
}

