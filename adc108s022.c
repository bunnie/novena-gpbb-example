///
// This API may be a little confusing, so here's a system diagram to keep in mind:
// CPU <--I2C--> FPGA <--SPI--> ADC108S022
// In other words, the CPU-side API looks like I2C, but what's actually happening
// is the FPGA is translating this into a local SPI implementation. 
///

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include <linux/i2c-dev.h>
#include <sys/ioctl.h>

#include "adc108s022.h"
#include "novena-gpbb.h"

#define ADC108S022_I2C_ADR  (0x3c >> 1) // actually, the whole FPGA sits here

//#define DEBUG
//#define DEBUG_STANDALONE   // add a main routine for stand-alone debug

#ifdef DEBUG
# define DEBUG_PRINT(x) printf x
#else
# define DEBUG_PRINT(x) do {} while (0)
#endif

#ifdef DEBUG  // discipline to only use dump in debug mode
void dump(void *buf, int count) {
  int i = 0;
  for( i = 0; i < count; i++ ) {
    if( (i%8) == 0 ) 
      printf( "\n" );
    printf( "%02x ", ((char *) buf)[i] );
  }
}
#endif

int adc108s022_write_byte( unsigned char adr, unsigned char data ) {
  int i2cfd;
  char i2cbuf[2]; 
  int slave_address = -1;

  struct i2c_msg msg[2];
		
  struct i2c_ioctl_rdwr_data {
    struct i2c_msg *msgs;  /* ptr to array of simple messages */              
    int nmsgs;             /* number of messages to exchange */ 
  } msgst;
  
  slave_address = ADC108S022_I2C_ADR;
  
  i2cfd = open("/dev/i2c-2", O_RDWR);
  if( i2cfd < 0 ) {
    perror("Unable to open /dev/i2c-2\n");
    i2cfd = 0;
    return 1;
  }
  if( ioctl( i2cfd, I2C_SLAVE, slave_address) < 0 ) {
    perror("Unable to set I2C slave device\n" );
    printf( "Address: %02x\n", slave_address );
    close( i2cfd );
    return 1;
  }

  i2cbuf[0] = adr; i2cbuf[1] = data;
  // set address for read
  msg[0].addr = slave_address;
  msg[0].flags = 0; // no flag means do a write
  msg[0].len = 2;
  msg[0].buf = i2cbuf;
  
#ifdef DEBUG
  dump(i2cbuf, 2);
#endif

  msgst.msgs = msg;	
  msgst.nmsgs = 1;

  if (ioctl(i2cfd, I2C_RDWR, &msgst) < 0){
    perror("Transaction failed\n" );
    close( i2cfd );
    return -1;
  }

  close( i2cfd );
  
  return 0;
}


int adc108s022_read_byte( unsigned char adr, unsigned char *data ) {
  int i2cfd;
  int slave_address = -1;

  struct i2c_msg msg[2];
		
  struct i2c_ioctl_rdwr_data {
    struct i2c_msg *msgs;  /* ptr to array of simple messages */              
    int nmsgs;             /* number of messages to exchange */ 
  } msgst;
  
  slave_address = ADC108S022_I2C_ADR;
  
  i2cfd = open("/dev/i2c-2", O_RDWR);
  if( i2cfd < 0 ) {
    perror("Unable to open /dev/i2c-2\n");
    i2cfd = 0;
    return -1;
  }
  if( ioctl( i2cfd, I2C_SLAVE, slave_address) < 0 ) {
    perror("Unable to set I2C slave device\n" );
    printf( "Address: %02x\n", slave_address );
    close( i2cfd );
    return -1;
  }

  // set write address
  msg[0].addr = slave_address;
  msg[0].flags = 0;
  msg[0].len = 1;
  msg[0].buf = (char *) &adr;

  // set readback buffer
  msg[1].addr = slave_address;
  msg[1].flags = I2C_M_NOSTART | I2C_M_RD;
  //  msg[1].flags = I2C_M_RD;
  msg[1].len = 1;
  msg[1].buf = (char *) data;

  msgst.msgs = msg;	
  msgst.nmsgs = 2;

  if (ioctl(i2cfd, I2C_RDWR, &msgst) < 0){
    perror("Transaction failed\n" );
    close( i2cfd );
    return -1;
  }

  close( i2cfd );
  
  return 0;
}

void adc_chan(unsigned int chan) {
  unsigned char data;
  unsigned char chan_change = 0;

  if( chan > 0x7 ) {
    printf( "Warning: channel out of range; aborting.\n" );
    return;
  }
  
  adc108s022_read_byte( FPGA_I2C_ADC_CTL, &data );
  if( (data & 7) != (chan & 7) ) {
    chan_change = 1;
  }

  adc108s022_write_byte( FPGA_I2C_ADC_CTL, (unsigned char) (chan & 0x7) );

  if( chan_change ) {
    adc_read(); // throw away result
  }

#ifdef DEBUG
  adc108s022_read_byte( FPGA_I2C_ADC_CTL, &data );
  printf (" ADC108S022 readback: %02x\n", data );
#endif

}

unsigned int adc_read() {
  unsigned char chan;
  unsigned char valid = 0;
  unsigned char data;
  unsigned int retval;
  
  adc108s022_read_byte( FPGA_I2C_ADC_CTL, &chan );

  adc108s022_write_byte( FPGA_I2C_ADC_CTL, chan | 0x8 ); // initiate conversion

  while( !valid ) {
    adc108s022_read_byte( FPGA_I2C_ADC_VALID, &valid );
  }

  adc108s022_read_byte( FPGA_I2C_ADC_DAT_L, &data );
  retval = data;
  adc108s022_read_byte( FPGA_I2C_ADC_DAT_H, &data );
  retval |= (data << 8);

  adc108s022_write_byte( FPGA_I2C_ADC_CTL, chan ); // clear initiation bit

  return retval;
}


#ifdef DEBUG_STANDALONE
int main() {
  unsigned int rval;
  unsigned char chan = 7;

  adc_chan(chan);
  rval = adc_read();
  printf( "Channel %d: %d\n", chan, rval );
}
#endif
