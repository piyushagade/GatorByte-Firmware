#ifndef GB_PIPER_h
#define GB_PIPER_h

#ifndef GB_h
    #include "../GB.h"
#endif

class GB_PIPER {
    public:
        GB_PIPER();

        DEVICE device = {
            "piper",
            "GatorByte Piper"
        };

        bool ishot();
        int secondsuntilhot();

        typedef void (*callback_t_func)(int);
        int pipe(int interval, bool runatinit, callback_t_func function);

    private:
        unsigned long _executed_at = 0;
        unsigned long _execution_counter = 0;
        unsigned long _interval;
        unsigned long _runatinit = 0;
};

GB_PIPER::GB_PIPER() { }

/* 
    Process the incoming command
    Interval is in milliseconds

    Returns seconds until the piper becomes hot
*/
int GB_PIPER::pipe(int interval, bool runatinit, callback_t_func function) {

    if (interval <= 0) {
        interval =  10 * 60 * 1000;
    }

    this->_interval = interval;
    this->_runatinit = runatinit;
    unsigned long secondsuntilhot;

    if (this->ishot()) {
        secondsuntilhot = 0;
        this->_execution_counter++;
        this->_executed_at = millis();
        
        // Execute the function
        function(this->_execution_counter);
    }
    else {
        // Serial.println(this->_interval);
        // Serial.println(millis());
        // Serial.println(this->_executed_at);
        secondsuntilhot = (this->_interval - millis() + this->_executed_at) / 1000;
    }

    return secondsuntilhot > 0 ? secondsuntilhot : this->_interval / 1000;
}

bool GB_PIPER::ishot() {
    bool result = (
        this->_runatinit && 
        this->_executed_at == 0
    ) ||
    (
        millis() - this->_executed_at > this->_interval
    ); 
    return result;
}

int GB_PIPER::secondsuntilhot() {
    // Serial.println(this->_interval / 1000);
    // Serial.println(millis() / 1000);
    // Serial.println(this->_executed_at / 1000);
    return (this->_interval - millis() + this->_executed_at) / 1000;
}
#endif