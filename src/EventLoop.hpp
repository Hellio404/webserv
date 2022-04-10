#pragma once
#include <set>
#include <sys/time.h>
#include <ctime>

union EventData
{
    int                 integer;
    void*               pointer;
    long long           l_integer;
    unsigned long long  ul_integer;
};

struct Event {
    static unsigned long long id_generator;
    void (*handler) (EventData);
    const EventData             data;
    const unsigned long long    ms_time;
    struct timeval              activation_time;
    const unsigned long long    periodic:1; // default to false
    const unsigned long long    id:63;
    Event(void (*) (EventData), EventData, struct timeval, unsigned long long,bool = false);
    bool operator<(const Event& other) const;
};

class EventLoop
{
private:
    std::set<Event> events;
public:
    EventLoop(/* args */);
    ~EventLoop();
    void add_event(Event event);
    void add_event(void (*handler) (EventData), EventData data, struct timeval activation_time, bool periodic = false);
    void add_event(void (*handler) (EventData), EventData data, long long ms, bool periodic = false);
    int run();
    struct timeval  get_current_time();
    long long       get_next_activation_time();
    void            reschedule_event(std::set<Event>::iterator it);
};
