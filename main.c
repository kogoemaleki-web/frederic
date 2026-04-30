/*
 * Balance Electronique - PIC16F876A + HX711 + Load Cell + LCD 16x2
 * Compilateur: XC8
 * Horloge: 4 MHz (cristal)
 */

#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

// ======================== Configuration bits ========================
#pragma config FOSC = HS        // Oscillateur HS
#pragma config WDTE = OFF       // Watchdog OFF
#pragma config PWRTE = ON       // Power-up Timer ON
#pragma config BOREN = ON       // Brown-out Reset ON
#pragma config LVP = OFF        // Low Voltage Programming OFF
#pragma config CPD = OFF
#pragma config WRT = OFF
#pragma config CP = OFF

#define _XTAL_FREQ 4000000UL

// ======================== Connexions ========================
// HX711
#define HX711_DOUT   RB0
#define HX711_SCK    RB1

// LCD 16x2 en mode 4 bits
#define LCD_RS       RC0
#define LCD_EN       RC1
#define LCD_D4       RC2
#define LCD_D5       RC3
#define LCD_D6       RC4
#define LCD_D7       RC5

// Bouton tare (actif bas)
#define BTN_TARE     RB2

// ======================== Paramètres ========================
// A ajuster pendant la calibration
static float g_calibration_factor = 2100.0f;
static long g_offset = 0;

// ======================== Prototypes LCD ========================
void lcd_pulse_enable(void);
void lcd_send_nibble(uint8_t nib);
void lcd_send_cmd(uint8_t cmd);
void lcd_send_data(uint8_t data);
void lcd_init(void);
void lcd_set_cursor(uint8_t row, uint8_t col);
void lcd_print(const char *s);
void lcd_clear(void);

// ======================== Prototypes HX711 ========================
bool hx711_is_ready(void);
long hx711_read_raw(void);
long hx711_read_average(uint8_t times);
void hx711_tare(uint8_t times);
float hx711_get_units(uint8_t times);

// ======================== LCD ========================
void lcd_pulse_enable(void) {
    LCD_EN = 1;
    __delay_us(2);
    LCD_EN = 0;
    __delay_us(100);
}

void lcd_send_nibble(uint8_t nib) {
    LCD_D4 = (nib >> 0) & 0x01;
    LCD_D5 = (nib >> 1) & 0x01;
    LCD_D6 = (nib >> 2) & 0x01;
    LCD_D7 = (nib >> 3) & 0x01;
    lcd_pulse_enable();
}

void lcd_send_cmd(uint8_t cmd) {
    LCD_RS = 0;
    lcd_send_nibble(cmd >> 4);
    lcd_send_nibble(cmd & 0x0F);
    __delay_ms(2);
}

void lcd_send_data(uint8_t data) {
    LCD_RS = 1;
    lcd_send_nibble(data >> 4);
    lcd_send_nibble(data & 0x0F);
    __delay_ms(2);
}

void lcd_init(void) {
    __delay_ms(20);
    LCD_RS = 0;
    LCD_EN = 0;

    // Séquence init 4-bit
    lcd_send_nibble(0x03);
    __delay_ms(5);
    lcd_send_nibble(0x03);
    __delay_us(150);
    lcd_send_nibble(0x03);
    lcd_send_nibble(0x02);

    lcd_send_cmd(0x28); // 4 bits, 2 lignes
    lcd_send_cmd(0x0C); // Display ON, cursor OFF
    lcd_send_cmd(0x06); // Entry mode
    lcd_clear();
}

void lcd_set_cursor(uint8_t row, uint8_t col) {
    uint8_t addr = (row == 0) ? (0x80 + col) : (0xC0 + col);
    lcd_send_cmd(addr);
}

void lcd_print(const char *s) {
    while (*s) {
        lcd_send_data(*s++);
    }
}

void lcd_clear(void) {
    lcd_send_cmd(0x01);
    __delay_ms(2);
}

// ======================== HX711 ========================
bool hx711_is_ready(void) {
    return (HX711_DOUT == 0);
}

long hx711_read_raw(void) {
    uint32_t data = 0;
    uint8_t i;

    while (!hx711_is_ready());

    for (i = 0; i < 24; i++) {
        HX711_SCK = 1;
        __delay_us(1);
        data = (data << 1) | HX711_DOUT;
        HX711_SCK = 0;
        __delay_us(1);
    }

    // Gain 128 => 1 pulse supplémentaire
    HX711_SCK = 1;
    __delay_us(1);
    HX711_SCK = 0;
    __delay_us(1);

    // Conversion 24-bit signé en long signé
    if (data & 0x800000UL) {
        data |= 0xFF000000UL;
    }

    return (long)data;
}

long hx711_read_average(uint8_t times) {
    int32_t sum = 0;
    uint8_t i;
    for (i = 0; i < times; i++) {
        sum += hx711_read_raw();
    }
    return (sum / times);
}

void hx711_tare(uint8_t times) {
    g_offset = hx711_read_average(times);
}

float hx711_get_units(uint8_t times) {
    long raw = hx711_read_average(times);
    return (raw - g_offset) / g_calibration_factor;
}

// ======================== Main ========================
void setup(void) {
    // Désactive les entrées analogiques
    ADCON1 = 0x06;

    // Ports
    TRISB0 = 1; // DOUT input
    TRISB1 = 0; // SCK output
    TRISB2 = 1; // Button input

    TRISC0 = 0;
    TRISC1 = 0;
    TRISC2 = 0;
    TRISC3 = 0;
    TRISC4 = 0;
    TRISC5 = 0;

    HX711_SCK = 0;

    // Pull-up port B (si disponible sur votre config)
    OPTION_REGbits.nRBPU = 0;
}

int main(void) {
    char line[17];
    float weight;

    setup();
    lcd_init();

    lcd_set_cursor(0, 0);
    lcd_print("Balance HX711");
    lcd_set_cursor(1, 0);
    lcd_print("Initialisation");

    __delay_ms(1500);

    hx711_tare(15);

    while (1) {
        if (BTN_TARE == 0) {
            lcd_clear();
            lcd_set_cursor(0, 0);
            lcd_print("Tare en cours...");
            hx711_tare(20);
            __delay_ms(500);
        }

        weight = hx711_get_units(8);
        if (weight < 0.0f && weight > -2.0f) {
            weight = 0.0f;
        }

        lcd_set_cursor(0, 0);
        lcd_print("Poids:          ");

        lcd_set_cursor(1, 0);
        snprintf(line, sizeof(line), "%7.2f g       ", weight);
        lcd_print(line);

        __delay_ms(200);
    }

    return 0;
}
