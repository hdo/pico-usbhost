/**
 * Simple USB HOST implementation for generic hid device (sublass 0)
 * 
 * Example inspired from Raspberry Pi Pico Examples.
 * see https://github.com/raspberrypi/pico-examples
 * 
 * Requires: https://github.com/hdo/tinyusb (Branch generic_hid_host)
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "bsp/board.h"
#include "tusb.h"


typedef struct TU_ATTR_PACKED
{
  uint8_t buffer[128];  
} hid_generic_hid_report_t;


void print_greeting(void);
void led_blinking_task(void);
extern void hid_task(void);

int main(void) {
    board_init();
    print_greeting();

    tusb_init();

    while (1) {
        // tinyusb host task
        tuh_task();
        led_blinking_task();

#if CFG_TUH_HID_KEYBOARD || CFG_TUH_HID_MOUSE
        hid_task();
#endif
    }

    return 0;
}

//--------------------------------------------------------------------+
// USB HID
//--------------------------------------------------------------------+
#if CFG_TUH_HID_KEYBOARD

CFG_TUSB_MEM_SECTION static hid_keyboard_report_t usb_keyboard_report;
uint8_t const keycode2ascii[128][2] = {HID_KEYCODE_TO_ASCII};

// look up new key in previous keys
static inline bool find_key_in_report(hid_keyboard_report_t const *p_report, uint8_t keycode) {
    for (uint8_t i = 0; i < 6; i++) {
        if (p_report->keycode[i] == keycode) return true;
    }

    return false;
}

static inline void process_kbd_report(hid_keyboard_report_t const *p_new_report) {
    static hid_keyboard_report_t prev_report = {0, 0, {0}}; // previous report to check key released

    //------------- example code ignore control (non-printable) key affects -------------//
    for (uint8_t i = 0; i < 6; i++) {
        if (p_new_report->keycode[i]) {
            if (find_key_in_report(&prev_report, p_new_report->keycode[i])) {
                // exist in previous report means the current key is holding
            } else {
                // not existed in previous report means the current key is pressed
                bool const is_shift =
                        p_new_report->modifier & (KEYBOARD_MODIFIER_LEFTSHIFT | KEYBOARD_MODIFIER_RIGHTSHIFT);
                uint8_t ch = keycode2ascii[p_new_report->keycode[i]][is_shift ? 1 : 0];
                putchar(ch);
                if (ch == '\r') putchar('\n'); // added new line for enter key

                fflush(stdout); // flush right away, else nanolib will wait for newline
            }
        }
        // TODO example skips key released
    }

    prev_report = *p_new_report;
}

void tuh_hid_keyboard_mounted_cb(uint8_t dev_addr) {
    // application set-up
    printf("A Keyboard device (address %d) is mounted\r\n", dev_addr);

    tuh_hid_keyboard_get_report(dev_addr, &usb_keyboard_report);
}

void tuh_hid_keyboard_unmounted_cb(uint8_t dev_addr) {
    // application tear-down
    printf("A Keyboard device (address %d) is unmounted\r\n", dev_addr);
}

// invoked ISR context
void tuh_hid_keyboard_isr(uint8_t dev_addr, xfer_result_t event) {
    (void) dev_addr;
    (void) event;
}

#endif

#if CFG_TUH_HID_MOUSE

CFG_TUSB_MEM_SECTION static hid_mouse_report_t usb_mouse_report;

void cursor_movement(int8_t x, int8_t y, int8_t wheel) {
    //------------- X -------------//
    if (x < 0) {
        printf(ANSI_CURSOR_BACKWARD(%d), (-x)); // move left
    } else if (x > 0) {
        printf(ANSI_CURSOR_FORWARD(%d), x); // move right
    } else {}

    //------------- Y -------------//
    if (y < 0) {
        printf(ANSI_CURSOR_UP(%d), (-y)); // move up
    } else if (y > 0) {
        printf(ANSI_CURSOR_DOWN(%d), y); // move down
    } else {}

    //------------- wheel -------------//
    if (wheel < 0) {
        printf(ANSI_SCROLL_UP(%d), (-wheel)); // scroll up
    } else if (wheel > 0) {
        printf(ANSI_SCROLL_DOWN(%d), wheel); // scroll down
    } else {}
}

static inline void process_mouse_report(hid_mouse_report_t const *p_report) {
    static hid_mouse_report_t prev_report = {0};

    //------------- button state  -------------//
    uint8_t button_changed_mask = p_report->buttons ^prev_report.buttons;
    if (button_changed_mask & p_report->buttons) {
        printf(" %c%c%c ",
               p_report->buttons & MOUSE_BUTTON_LEFT ? 'L' : '-',
               p_report->buttons & MOUSE_BUTTON_MIDDLE ? 'M' : '-',
               p_report->buttons & MOUSE_BUTTON_RIGHT ? 'R' : '-');
    }

    //------------- cursor movement -------------//
    cursor_movement(p_report->x, p_report->y, p_report->wheel);
}


void tuh_hid_mouse_mounted_cb(uint8_t dev_addr) {
    // application set-up
    printf("A Mouse device (address %d) is mounted\r\n", dev_addr);
}

void tuh_hid_mouse_unmounted_cb(uint8_t dev_addr) {
    // application tear-down
    printf("A Mouse device (address %d) is unmounted\r\n", dev_addr);
}

// invoked ISR context
void tuh_hid_mouse_isr(uint8_t dev_addr, xfer_result_t event) {
    (void) dev_addr;
    (void) event;
}

#endif

#if CFG_TUSB_HOST_HID_GENERIC

CFG_TUSB_MEM_SECTION static hid_generic_hid_report_t usb_generic_hid_report;

static inline void process_generic_hid_report(hid_generic_hid_report_t const *p_new_report) {
    board_led_write(1);
    puts("process_generic_hid_report");
    printf("%02X %02X %02X %02X %02X %02X \r\n", 
        p_new_report->buffer[0], 
        p_new_report->buffer[1],
        p_new_report->buffer[2],
        p_new_report->buffer[3],
        p_new_report->buffer[4],
        p_new_report->buffer[5]);

    board_led_write(0);
}

void tuh_hid_generic_hid_mounted_cb(uint8_t dev_addr) {
    // application set-up
    printf("A generic device (address %d) is mounted\r\n", dev_addr);
}

void tuh_hid_generic_hid_unmounted_cb(uint8_t dev_addr) {
    // application tear-down
    printf("A generic device (address %d) is unmounted\r\n", dev_addr);
}

// invoked ISR context
void tuh_hid_generic_hid_isr(uint8_t dev_addr, xfer_result_t event) {
    (void) dev_addr;
    (void) event;
}

#endif





void hid_task(void) {
    uint8_t const addr = 1;

#if CFG_TUH_HID_KEYBOARD
    if (tuh_hid_keyboard_is_mounted(addr)) {
        if (!tuh_hid_keyboard_is_busy(addr)) {
            process_kbd_report(&usb_keyboard_report);
            tuh_hid_keyboard_get_report(addr, &usb_keyboard_report);
        }
    }
#endif

#if CFG_TUH_HID_MOUSE
    if (tuh_hid_mouse_is_mounted(addr)) {
        if (!tuh_hid_mouse_is_busy(addr)) {
            process_mouse_report(&usb_mouse_report);
            tuh_hid_mouse_get_report(addr, &usb_mouse_report);
        }
    }
#endif

#if CFG_TUSB_HOST_HID_GENERIC
    if (tuh_hid_generic_hid_is_mounted(addr)) {
        if (!tuh_hid_generic_hid_is_busy(addr)) {
            process_generic_hid_report(&usb_generic_hid_report);
            tuh_hid_generic_hid_get_report(addr, &usb_generic_hid_report);
        }
    }
#endif


}


//--------------------------------------------------------------------+
// BLINKING TASK
//--------------------------------------------------------------------+
void led_blinking_task(void) {
    const uint32_t interval_ms = 250;
    static uint32_t start_ms = 0;

    static bool led_state = false;

    // Blink every interval ms
    if (board_millis() - start_ms < interval_ms) return; // not enough time
    start_ms += interval_ms;

    board_led_write(led_state);
    led_state = 1 - led_state; // toggle
}

//--------------------------------------------------------------------+
// HELPER FUNCTION
//--------------------------------------------------------------------+
void print_greeting(void) {
    printf("This Host demo is configured to support:\n");
    if (CFG_TUH_HID_KEYBOARD) puts("  - HID Keyboard");
    if (CFG_TUH_HID_MOUSE) puts("  - HID Mouse");
}
