#ifndef GB_PIPER_h
#define GB_PIPER_h

class GB_PIPER {
    public:
        GB_PIPER();

        DEVICE device = {
            "piper",
            "GatorByte Piper"
        };

        bool ishot();

        typedef void (*callback_t_func)(int);
        GB_PIPER& pipe(int interval, bool runatinit, callback_t_func function);

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
*/
GB_PIPER& GB_PIPER::pipe(int interval, bool runatinit, callback_t_func function) {
    this->_interval = interval;
    this->_runatinit = runatinit;

    if (
        this->ishot()
    ) 
    {
        this->_execution_counter++;
        this->_executed_at = millis();
        
        // Execute the function
        function(this->_execution_counter);
    }
    else {
        Serial.println ("Piper is cold for the next " + String((this-> _interval - millis() + this->_executed_at) / 1000) + " seconds.");
    }

    return *this;
}

bool GB_PIPER::ishot() {
    bool result = (
        this->_runatinit && 
        this->_executed_at == 0
    ) ||
    (
        millis() - this->_executed_at > this-> _interval
    ); 
    return result;
}
#endif