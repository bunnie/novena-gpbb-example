#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include <linux/i2c-dev.h>
#include <sys/ioctl.h>

#include "dac101c085.h"

#define DAC101C085_A_I2C_ADR  (0x14 >> 1)
#define DAC101C085_B_I2C_ADR  (0x12 >> 1)

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

int dac101c085_write_byte( unsigned short data, dacType dac ) {
  int i2cfd;
  char i2cbuf[2]; 
  int slave_address = -1;

  struct i2c_msg msg[2];
		
  struct i2c_ioctl_rdwr_data {
    struct i2c_msg *msgs;  /* ptr to array of simple messages */              
    int nmsgs;             /* number of messages to exchange */ 
  } msgst;
  
  if( dac == DAC_A )
    slave_address = DAC101C085_A_I2C_ADR;
  else if( dac == DAC_B )
    slave_address = DAC101C085_B_I2C_ADR;
  else
    slave_address = -1;
  
  i2cfd = open("/dev/i2c-1", O_RDWR);
  if( i2cfd < 0 ) {
    perror("Unable to open /dev/i2c-1\n");
    i2cfd = 0;
    return 1;
  }
  if( ioctl( i2cfd, I2C_SLAVE, slave_address) < 0 ) {
    perror("Unable to set I2C slave device\n" );
    printf( "Address: %02x\n", slave_address );
    close( i2cfd );
    return 1;
  }

  i2cbuf[0] = ((data & 0xFF00) >> 8); i2cbuf[1] = (data & 0xFF);
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


int dac101c085_read_byte( unsigned short *data, dacType dac ) {
  int i2cfd;
  int slave_address = -1;
  unsigned char buff[2];

  struct i2c_msg msg[2];
		
  struct i2c_ioctl_rdwr_data {
    struct i2c_msg *msgs;  /* ptr to array of simple messages */              
    int nmsgs;             /* number of messages to exchange */ 
  } msgst;
  
  if( dac == DAC_A )
    slave_address = DAC101C085_A_I2C_ADR;
  else if( dac == DAC_B )
    slave_address = DAC101C085_B_I2C_ADR;
  else
    slave_address = -1;
  
  i2cfd = open("/dev/i2c-1", O_RDWR);
  if( i2cfd < 0 ) {
    perror("Unable to open /dev/i2c-1\n");
    i2cfd = 0;
    return -1;
  }
  if( ioctl( i2cfd, I2C_SLAVE, slave_address) < 0 ) {
    perror("Unable to set I2C slave device\n" );
    printf( "Address: %02x\n", slave_address );
    close( i2cfd );
    return -1;
  }

  // set readback buffer
  msg[0].addr = slave_address;
  msg[0].flags = I2C_M_NOSTART | I2C_M_RD;
  //  msg[1].flags = I2C_M_RD;
  msg[0].len = 2;
  msg[0].buf = (char *) data;

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

void dac_a_set(unsigned int code) {
  if( code > 0xFFF ) {
    printf( "Warning: code larger than 0xFFF; truncating.\n" );
    code = 0xFFF;
  }
  
  dac101c085_write_byte( (unsigned short) (code & 0xFFFF), DAC_A );

#ifdef DEBUG
  unsigned short data;
  dac101c085_read_byte( &data, DAC_A );
  printf (" DAC101C085 readback: %03x\n", data );
#endif

}

void dac_b_set(unsigned int code) {
  if( code > 0xFFF ) {
    printf( "Warning: code larger than 0xFFF; truncating.\n" );
    code = 0xFFF;
  }
  
  dac101c085_write_byte( (unsigned short) (code & 0xFFFF), DAC_B );

#ifdef DEBUG
  unsigned short data;
  dac101c085_read_byte( &data, DAC_B );
  printf (" DAC101C085 readback: %03x\n", data );
#endif

}


#ifdef DEBUG_STANDALONE
int main() {
  unsigned short data;
  int i;
  int readcount;
  unsigned char status;

  dac101c085_write_byte( 0x100, DAC_B );
  dac101c085_read_byte( &data, DAC_B );
  printf (" DAC101C085 readback: %03x\n", data );

#if 1
  while( 1 ) {
    for( i = 0; i < 0x1000; i++ ) {
      dac101c085_write_byte( i, DAC_B );
    }
    for( i = 0xFFF; i >= 0; i-- ) {
      dac101c085_write_byte( i, DAC_B );
    }
  }
#endif
}
#endif
