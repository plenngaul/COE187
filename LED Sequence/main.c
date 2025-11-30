#include "mxc_device.h"
#include "mxc_delay.h"
#include "gpio.h"
#include <stdio.h>

// ==============================================================================
// Configuration
// ==============================================================================
#define NUM_LEDS 8
#define BUTTON_PORT MXC_GPIO2
#define BUTTON_PIN  MXC_GPIO_PIN_3
#define BUTTON_MASK (MXC_GPIO_PIN_3)

// ==============================================================================
// GLOBAL VARIABLES
// ==============================================================================
// 0 = Sequence 1 (Stack Left), 1 = Sequence 2 (Stack Right)
int mode = 0; 

// ==============================================================================
// LED Mapping
// ==============================================================================
typedef struct {
    mxc_gpio_regs_t *port;
    uint32_t mask;
} gpio_map_t;

const gpio_map_t led_pins[NUM_LEDS] = {
    { MXC_GPIO2, MXC_GPIO_PIN_4  }, // LED 0 (Leftmost)
    { MXC_GPIO0, MXC_GPIO_PIN_9  },
    { MXC_GPIO0, MXC_GPIO_PIN_8  },
    { MXC_GPIO0, MXC_GPIO_PIN_11 },
    { MXC_GPIO0, MXC_GPIO_PIN_19 },
    { MXC_GPIO3, MXC_GPIO_PIN_1  },
    { MXC_GPIO2, MXC_GPIO_PIN_7  },
    { MXC_GPIO0, MXC_GPIO_PIN_17 }  // LED 7 (Rightmost)
};

// ==============================================================================
// Function Prototypes
// ==============================================================================
void init_leds(void);
void init_button(void);
int  button_pressed(void);
void update_leds(uint8_t pattern);

void seq_stack_left(void);
void seq_stack_right(void);

// ==============================================================================
// MAIN LOOP
// ==============================================================================
int main(void)
{
    printf("MAX78000 LED Sequencer: Button Toggle\n");

    init_leds();
    init_button();

    // Loop forever
    while (1) {
        
        // Mode 0: Run Sequence 1
        if (mode == 0) {
            seq_stack_left();
        } 
        // Mode 1: Run Sequence 2
        else {
            seq_stack_right();
        }
    }

    return 0;
}

// ==============================================================================
// SEQUENCE 1: Stack Left
// (Runner moves Left->Right. Pile builds on the RIGHT)
// ==============================================================================
void seq_stack_left(void)
{
    uint8_t locked_pattern = 0x00;  
    int num_locked = 0;            
    
    // Repeat until all LEDs are ON (0xFF) or button is pressed
    while (locked_pattern != 0xFF) {
        
        // Calculate where the runner should stop (hit the pile)
        int end_pos = (NUM_LEDS - 1) - num_locked;
        
        // Move runner from 0 to end_pos
        for (int pos = 0; pos <= end_pos; pos++) {
            
            // --- INSTANT BUTTON CHECK ---
            if (button_pressed()) {
                mode = 1; // Switch to Sequence 2
                return;   // Stop this function immediately
            }
            // ----------------------------

            uint8_t runner = (1 << pos);
            update_leds(locked_pattern | runner);
            MXC_Delay(MXC_DELAY_MSEC(100)); 
        }
        
        // Lock the LED into the pile
        locked_pattern |= (1 << end_pos);
        num_locked++;
    }
    
    // Hold the full bar for a moment
    update_leds(0xFF);
    MXC_Delay(MXC_DELAY_MSEC(500));
    update_leds(0x00);
    MXC_Delay(MXC_DELAY_MSEC(200));
}

// ==============================================================================
// SEQUENCE 2: Stack Right
// (Runner moves Right->Left. Pile builds on the LEFT)
// ==============================================================================
void seq_stack_right(void)
{
    uint8_t locked_pattern = 0x00;  
    int num_locked = 0;            
    
    while (locked_pattern != 0xFF) {
        
        // Calculate where the runner should stop
        int start_pos = num_locked;
        
        // Move runner from 7 down to start_pos
        for (int pos = NUM_LEDS - 1; pos >= start_pos; pos--) {

            // --- INSTANT BUTTON CHECK ---
            if (button_pressed()) {
                mode = 0; // Switch to Sequence 1
                return;   // Stop this function immediately
            }
            // ----------------------------

            uint8_t runner = (1 << pos);
            update_leds(locked_pattern | runner);
            MXC_Delay(MXC_DELAY_MSEC(100));
        }
        
        // Lock the LED into the pile
        locked_pattern |= (1 << start_pos);
        num_locked++;
    }
    
    // Hold the full bar for a moment
    update_leds(0xFF);
    MXC_Delay(MXC_DELAY_MSEC(500));
    update_leds(0x00);
    MXC_Delay(MXC_DELAY_MSEC(200));
}

// ==============================================================================
// HARDWARE SETUP & HELPERS
// ==============================================================================
void init_leds(void)
{
    mxc_gpio_cfg_t cfg;
    for (int i = 0; i < NUM_LEDS; i++) {
        cfg.port  = led_pins[i].port;
        cfg.mask  = led_pins[i].mask;
        cfg.func  = MXC_GPIO_FUNC_OUT;
        cfg.pad   = MXC_GPIO_PAD_NONE;
        cfg.vssel = MXC_GPIO_VSSEL_VDDIOH;
        MXC_GPIO_Config(&cfg);
        MXC_GPIO_OutClr(led_pins[i].port, led_pins[i].mask);
    }
}

void init_button(void)
{
    mxc_gpio_cfg_t btn;
    btn.port  = BUTTON_PORT;
    btn.mask  = BUTTON_MASK;
    btn.func  = MXC_GPIO_FUNC_IN;
    btn.pad   = MXC_GPIO_PAD_PULL_UP;
    btn.vssel = MXC_GPIO_VSSEL_VDDIO;
    MXC_GPIO_Config(&btn);
}

int button_pressed(void)
{
    // Check if Button is LOW (Pressed)
    if (!(BUTTON_PORT->in & BUTTON_MASK)) { 
        MXC_Delay(MXC_DELAY_MSEC(20)); // Short delay to debounce
        if (!(BUTTON_PORT->in & BUTTON_MASK)) {
            // Wait until user releases the button so we don't toggle rapidly
            while (!(BUTTON_PORT->in & BUTTON_MASK)); 
            MXC_Delay(MXC_DELAY_MSEC(20)); 
            return 1; // True, button was clicked
        }
    }
    return 0; // False
}

void update_leds(uint8_t pattern)
{
    for (int i = 0; i < NUM_LEDS; i++) {
        if (pattern & (1 << i))
            MXC_GPIO_OutSet(led_pins[i].port, led_pins[i].mask);
        else
            MXC_GPIO_OutClr(led_pins[i].port, led_pins[i].mask);
    }
}
