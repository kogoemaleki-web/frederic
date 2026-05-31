/*
  Balance électronique fiable - PIC16F876A (28 broches)
  Outil: mikroC PRO for PIC
*/

// ========================= Liaisons LCD =========================
sbit LCD_RS at RC0_bit;
sbit LCD_EN at RC1_bit;
sbit LCD_D4 at RC2_bit;
sbit LCD_D5 at RC3_bit;
sbit LCD_D6 at RC4_bit;
sbit LCD_D7 at RC5_bit;

sbit LCD_RS_Direction at TRISC0_bit;
sbit LCD_EN_Direction at TRISC1_bit;
sbit LCD_D4_Direction at TRISC2_bit;
sbit LCD_D5_Direction at TRISC3_bit;
sbit LCD_D6_Direction at TRISC4_bit;
sbit LCD_D7_Direction at TRISC5_bit;

// ========================= Liaisons HX711 =========================
sbit HX711_DOUT at RB0_bit;
sbit HX711_SCK  at RB1_bit;
sbit BTN_TARE   at RB2_bit;

sbit HX711_DOUT_Direction at TRISB0_bit;
sbit HX711_SCK_Direction  at TRISB1_bit;
sbit BTN_TARE_Direction   at TRISB2_bit;

// ========================= Paramètres =========================
float calibration_factor = 2100.0;   // Ajuster à la calibration
long  tare_offset = 0;

#define HX711_TIMEOUT_MS  120
#define SAMPLES_NORMAL    10
#define SAMPLES_TARE      20

// ========================= Outils =========================
unsigned short abs_long_diff_ok(long a, long b, long threshold) {
  long d = a - b;
  if(d < 0) d = -d;
  return (d <= threshold);
}

// ========================= HX711 robuste =========================
unsigned short HX711_WaitReady(unsigned int timeout_ms) {
  while(timeout_ms--) {
    if(HX711_DOUT == 0) return 1;
    Delay_ms(1);
  }
  return 0;
}

unsigned short HX711_ReadRawSafe(long *out_value) {
  unsigned long value = 0;
  unsigned short i;

  if(!HX711_WaitReady(HX711_TIMEOUT_MS)) {
    return 0; // timeout
  }

  for(i = 0; i < 24; i++) {
    HX711_SCK = 1;
    Delay_us(1);

    value <<= 1;
    if(HX711_DOUT) value |= 1;

    HX711_SCK = 0;
    Delay_us(1);
  }

  // Gain 128
  HX711_SCK = 1;
  Delay_us(1);
  HX711_SCK = 0;
  Delay_us(1);

  // Sign extension
  if(value & 0x800000) value |= 0xFF000000;

  *out_value = (long)value;
  return 1;
}

unsigned short HX711_ReadAverageSafe(unsigned short times, long *avg) {
  unsigned short i, ok_count = 0;
  long v, sum = 0;

  for(i = 0; i < times; i++) {
    if(HX711_ReadRawSafe(&v)) {
      sum += v;
      ok_count++;
    }
    Delay_ms(2);
  }

  if(ok_count == 0) return 0;
  *avg = sum / ok_count;
  return 1;
}

unsigned short HX711_TareSafe(unsigned short times) {
  long t;
  if(!HX711_ReadAverageSafe(times, &t)) return 0;
  tare_offset = t;
  return 1;
}

unsigned short HX711_GetWeightSafe(unsigned short times, float *w) {
  long raw;
  if(!HX711_ReadAverageSafe(times, &raw)) return 0;
  *w = ((float)(raw - tare_offset)) / calibration_factor;
  return 1;
}

// ========================= Init =========================
void Setup() {
  ADCON1 = 0x06; // digital

  HX711_DOUT_Direction = 1;
  HX711_SCK_Direction  = 0;
  BTN_TARE_Direction   = 1;

  HX711_SCK = 0;
  OPTION_REG.NOT_RBPU = 0; // pull-up PORTB

  Lcd_Init();
  Lcd_Cmd(_LCD_CLEAR);
  Lcd_Cmd(_LCD_CURSOR_OFF);
}

unsigned short ButtonTarePressed() {
  if(BTN_TARE == 0) {
    Delay_ms(30);           // anti-rebond
    if(BTN_TARE == 0) return 1;
  }
  return 0;
}

void main() {
  char txt[17];
  float w = 0.0;
  float last_w = 0.0;

  Setup();

  Lcd_Out(1, 1, "Balance HX711");
  Lcd_Out(2, 1, "Initialisation");
  Delay_ms(1200);

  if(!HX711_TareSafe(SAMPLES_TARE)) {
    Lcd_Cmd(_LCD_CLEAR);
    Lcd_Out(1, 1, "Erreur HX711");
    Lcd_Out(2, 1, "Verif cablage");
    while(1);
  }

  while(1) {
    if(ButtonTarePressed()) {
      Lcd_Cmd(_LCD_CLEAR);
      Lcd_Out(1, 1, "Tare en cours");
      if(HX711_TareSafe(SAMPLES_TARE)) {
        Lcd_Out(2, 1, "Tare OK");
      } else {
        Lcd_Out(2, 1, "Tare Echec");
      }
      while(BTN_TARE == 0);
      Delay_ms(120);
      Lcd_Cmd(_LCD_CLEAR);
    }

    if(HX711_GetWeightSafe(SAMPLES_NORMAL, &w)) {
      if((w < 0.0) && (w > -2.0)) w = 0.0;

      // Petit lissage anti-saut: ignore pics trop brutaux
      if(!abs_long_diff_ok((long)(w * 10.0), (long)(last_w * 10.0), 150)) {
        w = last_w;
      }
      last_w = w;

      Lcd_Out(1, 1, "Poids:");
      Lcd_Out(1, 8, "        ");
      FloatToStr(w, txt);
      Lcd_Out(2, 1, "                ");
      Lcd_Out(2, 1, txt);
      Lcd_Out(2, 12, "g");
    } else {
      Lcd_Out(1, 1, "HX711 absent ? ");
      Lcd_Out(2, 1, "Check cablage   ");
    }

    Delay_ms(150);
  }
}
