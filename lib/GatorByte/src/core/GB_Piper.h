#ifndef GB_PIPER_h
#define GB_PIPER_h

#ifndef GB_h
    #include "./GB_Primary.h"
#endif

class GB_PIPER : public GB_DEVICE {
    public:
        GB_PIPER(GB &gb);

        DEVICE device = {
            "piper",
            "GatorByte Piper"
        };

        typedef void (*callback_t_func)(int);
        GB_PIPER& pipe(int interval, bool runatinit, callback_t_func function);

    private:
        GB *_gb; 
        int _executed_at = 0;
        int _execution_counter = 0;
};

GB_PIPER::GB_PIPER(GB &gb) {
    _gb = &gb;
    _gb->includelibrary(this->device.id, this->device.name);
}

/* 
    Process the incoming command
    
    1. Interval is in milliseconds
*/
GB_PIPER& GB_PIPER::pipe(int interval, bool runatinit, callback_t_func function) {
    if (
        (
            runatinit && 
            this->_executed_at == 0
        ) ||
        (
            millis() - this->_executed_at > interval
        )) 
    {
        this->_execution_counter++;
        this->_executed_at = millis();
        
        // Execute the function
        function(this->_execution_counter);
    }

    return *this;
}
#endif