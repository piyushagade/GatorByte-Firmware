#ifndef GB_TIMER_h
#define GB_TIMER_h

#include "./lib/timer/timer.h"

class GB_TIMER {
    public:
        GB_TIMER();
        
        String MODULE_NAME = "Timer library";
        String MODULE_DESC = "Timer library";
        String MODULE_BRAND = "N/A";

        void start();
        void stop();
        void pause();
        void reset();
        void interval(unsigned long interval, void (*function)(void));
        void timeout(unsigned long timeout, void (*function)(void));
        unsigned long getElapsedTime();
        void setInterval(unsigned long interval, unsigned int repeat_count=-1);
        void setTimeout(unsigned long timeout);
        void clearInterval();
        void setCallback(void (*function)(void));
        void update();
        bool isPaused();
        bool isStopped();
        bool isRunning();
        
    private:
        GB _gb;
	    void (*function_callback)(void);
        CusTimer _timer;
};

GB_TIMER::GB_TIMER() {
    _timer = CusTimer();
}

void GB_TIMER::start() {_timer.start();}
void GB_TIMER::stop() {_timer.stop();}
void GB_TIMER::pause() {_timer.pause();}
void GB_TIMER::reset() {_timer.reset();}
unsigned long GB_TIMER::getElapsedTime() {_timer.getElapsedTime();}
void GB_TIMER::setInterval(unsigned long interval, unsigned int repeat_count) {_timer.setInterval(interval, repeat_count);}
void GB_TIMER::setTimeout(unsigned long timeout) {_timer.setTimeout(timeout);}
void GB_TIMER::clearInterval() {_timer.clearInterval();}
void GB_TIMER::setCallback(void (*function)(void)) {_timer.function_callback = function;}
void GB_TIMER::update() {_timer.update();}
bool GB_TIMER::isPaused() { return _timer.isPaused();}
bool GB_TIMER::isRunning() { return _timer.isRunning();}
void GB_TIMER::interval(unsigned long interval_time, void (*function)(void)) {
    _timer.setInterval(interval_time);    
    _timer.function_callback = function;   
    _timer.start();
}
void GB_TIMER::timeout(unsigned long interval_time, void (*function)(void)) {
    _timer.setTimeout(interval_time);    
    _timer.function_callback = function;   
    _timer.start();
}

#endif