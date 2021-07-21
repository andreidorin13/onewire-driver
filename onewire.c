// One-Wire Protocol implementation for DS18B20 Temperature Sensor

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

#define PIN 4

extern int gpioin(uint);

static void pin_low(void){
  gpiosel(PIN, Output);
  gpioout(PIN, 0);
}

static void pin_in(void){
  gpiosel(PIN, Input);
}

static int wire_reset(void){
  int data;

  pin_low();
  microdelay(480);
  pin_in();
  data = gpioin(PIN);
  microdelay(480);

  return data;
}

static void wire_write(u8int data){
	for(int i = 0; i < 8; i++){
    if(data & 0x01){
      // Write 1
      pin_low();
      microdelay(5);
      pin_in();
      microdelay(56);
    } else {
      // Write 0
      pin_low();
      microdelay(60);
      pin_in();
      microdelay(1);
    }
		data >>= 1;
  }
}

static u16int wire_read(void){
 	u16int data = 0;
	pin_in();

  for(int i = 0; i < 16; i++){
		data >>= 1;

    pin_low();
    microdelay(5);
    pin_in();
    microdelay(10);

    if(gpioin(PIN))
      data |= 0x8000;
    microdelay (45);
	}

	return(data);
}

static get_temp(void){
  wire_reset();
  wire_write(0xCC); // Skip Rom
  wire_write(0x44); // Convert

  microdelay(750000);

  wire_reset();
  wire_write(0xCC); // Skip Rom
  wire_write(0xBE); // Read Pad

  return wire_read();
}

static long driver_read(Chan *, void *buf, long n, vlong){
  char res[16];
  char *end = res + sizeof(res);

  u16int temp = get_temp();

  seprint(res, end, "0x%-4x\n", temp);
  return readstr(0, buf, n, res);
}

static long driver_write(Chan *, void *, long, vlong){
  return 0;
}

// Arch server linking
void onewirelink(void){
  addarchfile("onewire", 0666, driver_read, driver_write);
}
