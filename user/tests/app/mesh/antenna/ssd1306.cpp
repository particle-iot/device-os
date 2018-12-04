#include "application.h"
#include "ssd1306.h"
#include "ssd1306_font.h"


#define OLED_SLAVE_ADDR         0x3C

#define MAX_X_PIXEL             128
#define MAX_Y_PAGE              4


ssd1306 oled(OLED_SLAVE_ADDR);


ssd1306::ssd1306(uint8_t address) {
    _address = address;
}

void ssd1306::init() {
    Wire.begin();

    send_command(0xAE); // display off
    send_command(0x00); // set low column address
    send_command(0x10); // set high column address
    send_command(0x40); // set start line address
    send_command(0xB0); // set page address
    send_command(0x81); // contract control
    send_command(0xFF); // 128
    send_command(0xA1); // set segment remap
    send_command(0xA6); // normal / reverse
    send_command(0xA8); // set multiplex ratio(1 to 64)
    send_command(0x1F); // 1/16 duty
    send_command(0xC8); // Com scan direction
    send_command(0xD3); // set display offset
    send_command(0x00);
    send_command(0xD5); // set osc division
    send_command(0x80);
    send_command(0xD9); // set pre-charge period
    send_command(0x22);
    send_command(0xDA); // set COM pins
    send_command(0x02);
    send_command(0xDB); // set vcomh
    send_command(0x40);
    send_command(0x8D); // set charge pump enable
    send_command(0x14);
    send_command(0xAF); // turn on oled panel
}

size_t ssd1306::write(uint8_t c) {
    if (c == '\r') {
        _curr_x_pos = 0;
    }
    else if (c == '\n') {
        _curr_x_pos = 0;
        _curr_y_pos++;
        if (_curr_y_pos >= MAX_Y_PAGE) {
            _curr_y_pos = 0;
        }
    }
    else {
        if ((_curr_x_pos + OLED12832_CHAR_WIDTH) > MAX_X_PIXEL) {
            _curr_x_pos = 0;
            _curr_y_pos++;
            if (_curr_y_pos >= MAX_Y_PAGE) {
                _curr_y_pos = 0;
            }
        }

        if (_curr_x_pos == 0) {
            clear(_curr_y_pos, _curr_y_pos);
        }

        show(_curr_x_pos, _curr_y_pos, c);
        _curr_x_pos += OLED12832_CHAR_WIDTH;
    }

    return 0;
}

void ssd1306::clear(void) {
    for (uint8_t y = 0; y < MAX_Y_PAGE; y++) {
        send_command(0xB0 + y);
        send_command(0x00);
        send_command(0x10);

        for (uint8_t x = 0; x < MAX_X_PIXEL; x++) {
            send_data(0);
        }
    }

    _curr_x_pos = 0;
    _curr_y_pos = 0;
}

void ssd1306::clear(uint8_t page) {
    if (page >= MAX_Y_PAGE) {
        return;
    }

    send_command(0xB0 + page);
    send_command(0x00);
    send_command(0x10);

    for (uint8_t x = 0; x < MAX_X_PIXEL; x++) {
        send_data(0);
    }
}

void ssd1306::clear(uint8_t start_page, uint8_t end_page) {
    if (start_page >= MAX_Y_PAGE || end_page < start_page) {
        return;
    }

    for (uint8_t y = start_page; y <= end_page; y++) {
        send_command(0xB0 + y);
        send_command(0x00);
        send_command(0x10);

        for (uint8_t x = 0; x < MAX_X_PIXEL; x++) {
            send_data(0);
        }
    }
}

void ssd1306::show(uint8_t x, uint8_t y, uint8_t ch) {
    if (set_pos(x, y) == 0) {
        uint8_t c = ch - ' ';
        for (uint8_t i = 0; i < OLED12832_CHAR_WIDTH; i++) {
            send_data(oled12832_font[c][i]);
        }
    }
}

void ssd1306::show(uint8_t x, uint8_t y, const char *str) {
    uint8_t i = 0;
    while (str[i] != '\0') {
        show(x, y, str[i]);
        x += OLED12832_CHAR_WIDTH;
        i++;
    }
}

void ssd1306::send_command(uint8_t cmd) {
    uint8_t tx_buf[2] = { 0X00, cmd };

    Wire.beginTransmission(_address);
    Wire.write(tx_buf, sizeof(tx_buf));
    Wire.endTransmission(true);
}

void ssd1306::send_data(uint8_t data) {
    uint8_t tx_buf[2] = { 0X40, data };

    Wire.beginTransmission(_address);
    Wire.write(tx_buf, sizeof(tx_buf));
    Wire.endTransmission(true);
}

int ssd1306::set_pos(uint8_t x, uint8_t y) {
    if ((x + OLED12832_CHAR_WIDTH) > MAX_X_PIXEL ||
          y > (MAX_Y_PAGE - 1)) {
         return -1;
    }
    send_command(0xB0 + y);
    send_command(x & 0x0F);
    send_command(((x & 0xF0) >> 4) | 0x10);
    return 0;
}