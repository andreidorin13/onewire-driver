#include <u.h>
#include <libc.h>

// SPI Pins
#define RESET 23
#define DC 24
#define HIGH 1
#define LOW 0

// Screen Variables
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 160
#define OFFSET_L 0
#define OFFSET_R 63

// Segment Function definitions
void A(int, u16int);
void B(int, u16int);
void C(int, u16int);
void D(int, u16int);
void E(int, u16int);
void F(int, u16int);
void G(int, u16int);

/*
  Each segment of the display
  is a represented as a bit
  of a u8int mapped as drawn
  in this amazing ascii art:
     _____
  / \__A__/ \
  |F|_____|B|
  \ /__G__\ /
  / \__G__/ \
  |E|_____|C|
  \ /__D__\ /

  The BIT_MAP holds the u8int
  values of all the numbers.
  The NUMBERS array holds function
  pointers to drawing a certain segment
  and it is indexed in order
  A->G, in order to facilitate drawing
*/

static const u8int BIT_MAP[] = { 0x7E, 0x30, 0x6D, 0x79, 0x33, 0x5B, 0x5F, 0x70, 0x7F, 0x7B };
static void (*NUMBERS[])(int, u16int) = { A, B, C, D, E, F, G };

static u16int SCREEN_BG_COLOR = 0x00;
static int SCREEN_LEFT_NUM = 0x00;
static int SCREEN_RIGHT_NUM = 0x00;

static int GPIO;
static int SPI;
static int WIRE;

// ----- GPIO -----
void gpio_init(void){
  int gpio = open("/dev/gpio", ORDWR);
  if (gpio < 0){
    bind("#G", "/dev", MAFTER);
    gpio = open("/dev/gpio", ORDWR);
  }
  GPIO = gpio;
  fprint(gpio, "function %d out\n", RESET);
  fprint(gpio, "function %d out\n", DC);
}

void gpio_set(int pin, int status){
  fprint(GPIO, "set %d %d\n", pin, status);
}


// ----- SPI -----
void spi_init(void){
  int spi = open("/dev/spi0", ORDWR);
  if (spi < 0){
    bind("#π", "/dev", MAFTER);
    spi = open("/dev/spi0", ORDWR);
  }
  SPI = spi;
}

void spi_cmd(u8int cmd){
  gpio_set(DC, LOW);
  pwrite(SPI, &cmd, 1, 0);
}

void spi_data(u8int data){
  gpio_set(DC, HIGH);
  pwrite(SPI, &data, 1, 0);
}

void spi_write(u8int cmd, int argc, ...){
  u8int arg;
  va_list args;
  va_start(args, argc);

  spi_cmd(cmd);
  for (int i = 0; i < argc; i++){
    arg = va_arg(args, u8int);
    spi_data(arg);
  }
  va_end(args);
}


// ----- ONEWIRE -----
void onewire_init(void){
  int one = open("/dev/onewire", ORDWR);
  WIRE = one;
}

int onewire_read(void){
  char buf[7];
  buf[6] = 0;
  read(WIRE, buf, 16);
  return strtoll(buf, nil, 16);
}


// ----- Display -----
void reset_device(void){
  gpio_set(RESET,  HIGH);
  sleep(500);
  gpio_set(RESET, LOW);
  sleep(500);
  gpio_set(RESET, HIGH);
  sleep(500);
}

void screen_init(void){
  spi_write(0x01, 0);
  sleep(150);
  spi_write(0x11, 0);
  sleep(500);
  spi_write(0xB1, 3, 0x01, 0x2C, 0x2D);
  spi_write(0xB2, 3, 0x01, 0x2C, 0x2D);
  spi_write(0xB3, 6, 0x01, 0x2C, 0x2D, 0x01, 0x2C, 0x2D);
  spi_write(0xB4, 1, 0x07);
  spi_write(0xC0, 3, 0xA2, 0x02, 0x84);
  spi_write(0xC1, 1, 0xC5);
  spi_write(0xC2, 2, 0x0A, 0x00);
  spi_write(0xC3, 2, 0x8A, 0x2A);
  spi_write(0xC4, 2, 0x8A, 0xEE);
  spi_write(0xC5, 1, 0x0E);
  spi_write(0x20, 0);
  spi_write(0x36, 1, 0xC8);
  spi_write(0x3A, 1, 0x05);
  spi_write(0x2A, 4, 0x00, 0x02, 0x00, 0x7F+0x02);
  spi_write(0x2B, 4, 0x00, 0x01, 0x00, 0x9F+0x01);
  spi_write(0xE0, 16,
            0x02, 0x1C, 0x07, 0x12,
            0x37, 0x32, 0x29, 0x2D,
            0x29, 0x25, 0x2B, 0x39,
            0x00, 0x01, 0x03, 0x10
            );
  spi_write(0xE1, 16,
            0x03, 0x1D, 0x07, 0x06,
            0x2E, 0x2C, 0x29, 0x2D,
            0x2E, 0x2E, 0x37, 0x3F,
            0x00, 0x00, 0x02, 0x10
            );
  spi_write(0x13, 0);
  sleep(10);
  spi_write(0x29, 0);
  sleep(100);
}

void draw(u8int x0, u8int x1, u8int y0, u8int y1, u16int color){
  u8int hi = color >> 8;
  u8int lo = color;

  spi_write(0x2A, 4, x0 >> 8, x0 & 0xFF, x1 >> 8, x1 & 0xFF);
  spi_write(0x2B, 4, y0 >> 8, y0 & 0xFF, y1 >> 8, y1 & 0xFF);
  spi_write(0x2C, 0);

  for (u16int i = (y1-y0+1)*(x1-x0+1); i > 0; --i){
    spi_data(hi);
    spi_data(lo);
  }
}

// blocks are 10x40 or 40x10
void A(int offset, u16int color) { draw(offset + 13, offset + 53, 10, 20, color); }
void B(int offset, u16int color) { draw(offset + 53, offset + 63, 20, 60, color); }
void C(int offset, u16int color) { draw(offset + 53, offset + 63, 70, 110, color); }
void D(int offset, u16int color) { draw(offset + 13, offset + 53, 110, 120, color); }
void E(int offset, u16int color) { draw(offset + 03, offset + 13, 71, 110, color); }
void F(int offset, u16int color) { draw(offset + 03, offset + 13, 20, 60, color); }
void G(int offset, u16int color) { draw(offset + 13, offset + 53, 60, 70, color); }

void init_digit(int digit, int offset, u16int color){
  int new = BIT_MAP[digit];
  for (int i = 0; i <= 6; ++i){
    if (new & 0x01)
      (*NUMBERS[6-i])(offset, color);
    new >>= 1;
  }
}

void draw_digit(int digit, int offset, u16int color){
  int new = BIT_MAP[digit];
  int old;

  // Initialize the full numbers if they're 0
  if (SCREEN_LEFT_NUM == 0 && offset){
    init_digit(digit, offset, color);
    SCREEN_LEFT_NUM = BIT_MAP[digit];
    return;
  } else if (SCREEN_RIGHT_NUM == 0 && !offset) {
    init_digit(digit, offset, color);
    SCREEN_RIGHT_NUM = BIT_MAP[digit];
    return;
  }

  if (offset){
    old = SCREEN_LEFT_NUM;
    SCREEN_LEFT_NUM = new;
  } else {
    old = SCREEN_RIGHT_NUM;
    SCREEN_RIGHT_NUM = new;
  }

  // Optimizing screen refresh by
  // only updating changes
  for (int i = 0; i <= 6; ++i){
    int old_bit = old & 0x01;
    int new_bit = new & 0x01;

    // Draw new
    if (old_bit != new_bit && new_bit)
      (*NUMBERS[6-i])(offset, color);

    // Clear old
    else if (old_bit != new_bit && !new_bit)
      (*NUMBERS[6-i])(offset, SCREEN_BG_COLOR);

    new >>= 1;
    old >>= 1;
  }
}

void screen_clear(u16int color){
  // This method is very slow.
  // You can very clearly see the
  // refresh going down the screen
  if (SCREEN_BG_COLOR == color)
    return;
  draw(0, SCREEN_WIDTH, 0, SCREEN_HEIGHT, color);
  SCREEN_BG_COLOR = color;
  SCREEN_LEFT_NUM = 0x00;
  SCREEN_RIGHT_NUM = 0x00;
}

void main(){
  gpio_init();
  spi_init();
  onewire_init();
  reset_device();
  screen_init();

  screen_clear(0x0000);
  // Foreground, Background
  u16int theme_cool[2] = { 0xFFFF, 0x00FF };
  u16int theme_warn[2] = { 0x0000, 0xFF00 };
  u16int theme_hot[2] = { 0xF000, 0x0000 };

  while(1){
    u16int foreground = 0x0000;
    int temp = (int)(onewire_read() * 6 + temp / 4) / 100;
    print("Reading: %d\n", temp);

    if (temp < 26) {
      screen_clear(theme_cool[1]);
      foreground = theme_cool[0];
    } else if (temp >= 26 && temp < 27) {
      screen_clear(theme_warn[1]);
      foreground = theme_warn[0];
    } else if (temp >= 27){
      screen_clear(theme_hot[1]);
      foreground = theme_hot[0];
    }

    draw_digit(temp / 10, OFFSET_L, foreground);
    draw_digit(temp % 10, OFFSET_R, foreground);
    sleep(100);
  }
}

// 5c final.c && 5l final.5 && ./5.out
