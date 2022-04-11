#include "EventLoop.hpp"

namespace we 
{

unsigned long long Event::id_generator = 0;

static bool time_cmp_lteq(const struct timeval& a, const struct timeval& b) 
{
    return a.tv_sec < b.tv_sec || (a.tv_sec == b.tv_sec && a.tv_usec <= b.tv_usec);
}

EventLoop::EventLoop()
{
}

EventLoop::~EventLoop()
{
}

const Event& EventLoop::add_event(Event event)
{
    std::pair<std::set<Event>::iterator, bool> ret = events.insert(event);
    assert(ret.second);
    return *ret.first;
}

const Event& EventLoop::add_event(void (*handler) (EventData), EventData data, struct timeval activation_time, bool periodic)
{
    long long ms_time = 0;
    if (periodic)
    {
        const struct timeval now = get_current_time();
        long long ms_time = (activation_time.tv_sec - now.tv_sec) * 1000 + (activation_time.tv_usec - now.tv_usec) / 1000;
    }
    assert(ms_time >= 0);
    Event event(handler, data, activation_time, ms_time, periodic);
    return add_event(event);
}

const Event& EventLoop::add_event(void (*handler) (EventData), EventData data, long long ms, bool periodic)
{
    const struct timeval now = get_current_time();
    struct timeval activation_time;
    activation_time.tv_sec = now.tv_sec + ms / 1000;
    activation_time.tv_usec = now.tv_usec + (ms % 1000) * 1000;
    if (activation_time.tv_usec > 1000000)
    {
        activation_time.tv_sec++;
        activation_time.tv_usec -= 1000000;
    }
    Event event(handler, data, activation_time, ms, periodic);
    return add_event(event);
}

int EventLoop::run()
{
    int event_count = 0;
    std::set<Event>::iterator it = events.begin();
    while (it != events.end())
    {
        const struct timeval now = get_current_time();
        if (time_cmp_lteq(it->activation_time, now))
        {
            it->handler(it->data);
            if (it->periodic)
            {
                reschedule_event(it++);
            }
            else
                events.erase(it++);
            event_count++;
        }
        else
            break;
       
    }
    return event_count;
}

struct timeval EventLoop::get_current_time()
{
    struct timeval now;
    gettimeofday(&now, NULL);
    return now;
}

long long EventLoop::get_next_activation_time()
{
    if (events.empty())
        return -1;
    const struct timeval now = get_current_time();
    std::set<Event>::iterator it = events.begin();
    return (it->activation_time.tv_sec - now.tv_sec) * 1000 + (it->activation_time.tv_usec - now.tv_usec) / 1000;
}

void EventLoop::reschedule_event(std::set<Event>::iterator it)
{
    struct timeval next = get_current_time();
    long long ms_time = it->ms_time;
    next.tv_sec += ms_time / 1000;
    next.tv_usec += (ms_time % 1000) * 1000;
    if (next.tv_usec > 1000000)
    {
        next.tv_sec++;
        next.tv_usec -= 1000000;
    }
    Event event(it->handler, it->data, next, ms_time, it->periodic);
    events.erase(it);
    events.insert(event);
}

void            EventLoop::remove_event(Event event)
{
    events.erase(event);
}

Event::Event(void (*handler) (EventData), EventData data, struct timeval activation_time, unsigned long long ms_time, bool periodic):
    handler(handler), data(data), activation_time(activation_time), ms_time(ms_time), periodic(periodic), id(id_generator++)
{
}

bool Event::operator < (const Event& other) const
{
    bool smaller_or_equal = time_cmp_lteq(activation_time, other.activation_time);
    if (smaller_or_equal && (activation_time.tv_sec != other.activation_time.tv_sec 
        || activation_time.tv_usec != other.activation_time.tv_usec))
        return true;
    else if (smaller_or_equal)
        return id < other.id;
    else
        return false;
}

} // namespace we