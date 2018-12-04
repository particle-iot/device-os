#ifndef SSD1306_H
#define SSD1306_H

#include "spark_wiring_print.h"

class ssd1306: public Print {
    private:
        uint8_t _address;

        uint8_t _curr_x_pos = 0;
        uint8_t _curr_y_pos = 0;

        void send_command(uint8_t cmd);
        void send_data(uint8_t data);
        int  set_pos(uint8_t x, uint8_t y);

    public:
        ssd1306(uint8_t address);
        ~ssd1306() {};

        void init(void);

        void clear(void);
        void clear(uint8_t page);
        void clear(uint8_t start_page, uint8_t end_page);

        void show(uint8_t x, uint8_t y, uint8_t ch);
        void show(uint8_t x, uint8_t y, const char *str);

        virtual size_t write(uint8_t c);
};

extern ssd1306 oled;

#endif