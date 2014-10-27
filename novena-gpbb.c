#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include "gpio.h"

#include "novena-gpbb.h"
#include "dac101c085.h"
#include "adc108s022.h"

static int fd = 0;
static int   *mem_32 = 0;
static short *mem_16 = 0;
static char  *mem_8  = 0;
static int *prev_mem_range = 0;

int read_kernel_memory(long offset, int virtualized, int size) {
  int result;

  int *mem_range = (int *)(offset & ~0xFFFF);
  if( mem_range != prev_mem_range ) {
    //        fprintf(stderr, "New range detected.  Reopening at memory range %p\n", mem_range);
    prev_mem_range = mem_range;

    if(mem_32)
      munmap(mem_32, 0xFFFF);
    if(fd)
      close(fd);

    if(virtualized) {
      fd = open("/dev/kmem", O_RDWR);
      if( fd < 0 ) {
	perror("Unable to open /dev/kmem");
	fd = 0;
	return -1;
      }
    }
    else {
      fd = open("/dev/mem", O_RDWR);
      if( fd < 0 ) {
	perror("Unable to open /dev/mem");
	fd = 0;
	return -1;
      }
    }

    mem_32 = mmap(0, 0xffff, PROT_READ | PROT_WRITE, MAP_SHARED, fd, offset&~0xFFFF);
    if( -1 == (int)mem_32 ) {
      perror("Unable to mmap file");

      if( -1 == close(fd) )
	perror("Also couldn't close file");

      fd=0;
      return -1;
    }
    mem_16 = (short *)mem_32;
    mem_8  = (char  *)mem_32;
  }

  int scaled_offset = (offset-(offset&~0xFFFF));
  //    fprintf(stderr, "Returning offset 0x%08x\n", scaled_offset);
  if(size==1)
    result = mem_8[scaled_offset/sizeof(char)];
  else if(size==2)
    result = mem_16[scaled_offset/sizeof(short)];
  else
    result = mem_32[scaled_offset/sizeof(long)];

  return result;
}


int write_kernel_memory(long offset, long value, int virtualized, int size) {
  int old_value = read_kernel_memory(offset, virtualized, size);
  int scaled_offset = (offset-(offset&~0xFFFF));
  if(size==1)
    mem_8[scaled_offset/sizeof(char)]   = value;
  else if(size==2)
    mem_16[scaled_offset/sizeof(short)] = value;
  else
    mem_32[scaled_offset/sizeof(long)]  = value;
  return old_value;
}

void setvddio(int high) {
  volatile unsigned short *cs0;

  if(mem_16)
    munmap(mem_16, 0xFFFF);
  if(fd)
    close(fd);

  fd = open("/dev/mem", O_RDWR);
  if( fd < 0 ) {
    perror("Unable to open /dev/mem");
    fd = 0;
    return;
  }

  mem_16 = mmap(0, 0xffff, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0x08040000);
  cs0 = (volatile unsigned short *)mem_16;

  if( high )  // set to 5V
    cs0[F(FPGA_W_GPBB_CTL)] |= 0x8000;
  else
    cs0[F(FPGA_W_GPBB_CTL)] &= 0x7FFF;

}

void oe_state(int drive, int channel) {
  volatile unsigned short *cs0;

  if(mem_16)
    munmap(mem_16, 0xFFFF);
  if(fd)
    close(fd);

  fd = open("/dev/mem", O_RDWR);
  if( fd < 0 ) {
    perror("Unable to open /dev/mem");
    fd = 0;
    return;
  }

  mem_16 = mmap(0, 0xffff, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0x08040000);
  cs0 = (volatile unsigned short *)mem_16;
  
  if( channel == OE_A ) {
    if( drive ) 
      cs0[F(FPGA_W_GPBB_CTL)] |= 0x1;
    else
      cs0[F(FPGA_W_GPBB_CTL)] &= 0xFFFE;
  } else {
    if( drive ) 
      cs0[F(FPGA_W_GPBB_CTL)] |= 0x2;
    else
      cs0[F(FPGA_W_GPBB_CTL)] &= 0xFFFD;
  }
}


void print_usage(char *progname) {
  printf("Usage:\n"
        "%s [-h]\n"
        "\t-h  This help message\n"
	"\t-v  Read out the version code of the FPGA\n"
	"\t-da <value> set DAC A to value (0-1024 decimal)\n"
	"\t-db <value> set DAC B to value (0-1024 decimal)\n"
	"\t-a  <chan> set and read channel <chan> from ADC\n"
	"\t-hv set VDD-IO to high (5V) voltage\n"
	"\t-lv set VDD-IO to low (nom 3.3V unless you trimmed it) voltage\n"
	"\t-oea <value> drive I/O bank A (value = 1 means drive, 0 means tristate)\n"
	"\t-oeb <value> drive I/O bank B (value = 1 means drive, 0 means tristate)\n"
	"\t-testcs1 Check that burst-access area (CS1) works\n"
	 "", progname);
}


void setup_fpga() {
  int i;
  //  printf( "setting up EIM CS0 (register interface) pads and configuring timing\n" );
  // set up pads to be mapped to EIM
  for( i = 0; i < 16; i++ ) {
    write_kernel_memory( 0x20e0114 + i*4, 0x0, 0, 4 );  // mux mapping
    write_kernel_memory( 0x20e0428 + i*4, 0xb0b1, 0, 4 ); // pad strength config'd for a 100MHz rate 
  }

  // mux mapping
  write_kernel_memory( 0x20e046c - 0x314, 0x0, 0, 4 ); // BCLK
  write_kernel_memory( 0x20e040c - 0x314, 0x0, 0, 4 ); // CS0
  write_kernel_memory( 0x20e0410 - 0x314, 0x0, 0, 4 ); // CS1
  write_kernel_memory( 0x20e0414 - 0x314, 0x0, 0, 4 ); // OE
  write_kernel_memory( 0x20e0418 - 0x314, 0x0, 0, 4 ); // RW
  write_kernel_memory( 0x20e041c - 0x314, 0x0, 0, 4 ); // LBA
  write_kernel_memory( 0x20e0468 - 0x314, 0x0, 0, 4 ); // WAIT
  write_kernel_memory( 0x20e0408 - 0x314, 0x0, 0, 4 ); // A16
  write_kernel_memory( 0x20e0404 - 0x314, 0x0, 0, 4 ); // A17
  write_kernel_memory( 0x20e0400 - 0x314, 0x0, 0, 4 ); // A18

  // pad strength
  write_kernel_memory( 0x20e046c, 0xb0b1, 0, 4 ); // BCLK
  write_kernel_memory( 0x20e040c, 0xb0b1, 0, 4 ); // CS0
  write_kernel_memory( 0x20e0410, 0xb0b1, 0, 4 ); // CS1
  write_kernel_memory( 0x20e0414, 0xb0b1, 0, 4 ); // OE
  write_kernel_memory( 0x20e0418, 0xb0b1, 0, 4 ); // RW
  write_kernel_memory( 0x20e041c, 0xb0b1, 0, 4 ); // LBA
  write_kernel_memory( 0x20e0468, 0xb0b1, 0, 4 ); // WAIT
  write_kernel_memory( 0x20e0408, 0xb0b1, 0, 4 ); // A16
  write_kernel_memory( 0x20e0404, 0xb0b1, 0, 4 ); // A17
  write_kernel_memory( 0x20e0400, 0xb0b1, 0, 4 ); // A18

  write_kernel_memory( 0x020c4080, 0xcf3, 0, 4 ); // ungate eim slow clocks

  // rework timing for sync use
  // 0011 0  001 1   001    0   001 00  00  1  011  1    0   1   1   1   1   1   1
  // PSZ  WP GBC AUS CSREC  SP  DSZ BCS BCD WC BL   CREP CRE RFL WFL MUM SRD SWR CSEN
  //
  // PSZ = 0011  64 words page size
  // WP = 0      (not protected)
  // GBC = 001   min 1 cycles between chip select changes
  // AUS = 0     address shifted according to port size
  // CSREC = 001 min 1 cycles between CS, OE, WE signals
  // SP = 0      no supervisor protect (user mode access allowed)
  // DSZ = 001   16-bit port resides on DATA[15:0]
  // BCS = 00    0 clock delay for burst generation
  // BCD = 00    divide EIM clock by 0 for burst clock
  // WC = 1      write accesses are continuous burst length
  // BL = 011    32 word memory wrap length
  // CREP = 1    non-PSRAM, set to 1
  // CRE = 0     CRE is disabled
  // RFL = 1     fixed latency reads
  // WFL = 1     fixed latency writes
  // MUM = 1     multiplexed mode enabled
  // SRD = 1     synch reads
  // SWR = 1     synch writes
  // CSEN = 1    chip select is enabled

  //  write_kernel_memory( 0x21b8000, 0x5191C0B9, 0, 4 );
  write_kernel_memory( 0x21b8000, 0x31910BBF, 0, 4 );

  // EIM_CS0GCR2   
  //  MUX16_BYP_GRANT = 1
  //  ADH = 1 (1 cycles)
  //  0x1001
  write_kernel_memory( 0x21b8004, 0x1000, 0, 4 );


  // EIM_CS0RCR1   
  // 00 000101 0 000   0   000   0 000 0 000 0 000 0 000
  //    RWSC     RADVA RAL RADVN   OEA   OEN   RCSA  RCSN
  // RWSC 000101    5 cycles for reads to happen
  //
  // 0000 0111 0000   0011   0000 0000 0000 0000
  //  0    7     0     3      0  0    0    0
  // 0000 0101 0000   0000   0 000 0 000 0 000 0 000
//  write_kernel_memory( 0x21b8008, 0x05000000, 0, 4 );
//  write_kernel_memory( 0x21b8008, 0x0A024000, 0, 4 );
  write_kernel_memory( 0x21b8008, 0x09014000, 0, 4 );
  // EIM_CS0RCR2  
  // 0000 0000 0   000 00 00 0 010  0 001 
  //           APR PAT    RL   RBEA   RBEN
  // APR = 0   mandatory because MUM = 1
  // PAT = XXX because APR = 0
  // RL = 00   because async mode
  // RBEA = 000  these match RCSA/RCSN from previous field
  // RBEN = 000
  // 0000 0000 0000 0000 0000  0000
  write_kernel_memory( 0x21b800c, 0x00000000, 0, 4 );

  // EIM_CS0WCR1
  // 0   0    000100 000   000   000  000  010 000 000  000
  // WAL WBED WWSC   WADVA WADVN WBEA WBEN WEA WEN WCSA WCSN
  // WAL = 0       use WADVN
  // WBED = 0      allow BE during write
  // WWSC = 000100 4 write wait states
  // WADVA = 000   same as RADVA
  // WADVN = 000   this sets WE length to 1 (this value +1)
  // WBEA = 000    same as RBEA
  // WBEN = 000    same as RBEN
  // WEA = 010     2 cycles between beginning of access and WE assertion
  // WEN = 000     1 cycles to end of WE assertion
  // WCSA = 000    cycles to CS assertion
  // WCSN = 000    cycles to CS negation
  // 1000 0111 1110 0001 0001  0100 0101 0001
  // 8     7    E    1    1     4    5    1
  // 0000 0111 0000 0100 0000  1000 0000 0000
  // 0      7    0   4    0     8    0     0
  // 0000 0100 0000 0000 0000  0100 0000 0000
  //  0    4    0    0     0    4     0    0

  write_kernel_memory( 0x21b8010, 0x09080800, 0, 4 );
  //  write_kernel_memory( 0x21b8010, 0x02040400, 0, 4 );

  // EIM_WCR
  // BCM = 1   free-run BCLK
  // GBCD = 0  don't divide the burst clock
  write_kernel_memory( 0x21b8090, 0x701, 0, 4 );

  // EIM_WIAR 
  // ACLK_EN = 1
  write_kernel_memory( 0x21b8094, 0x10, 0, 4 );

  //  printf( "done.\n" );
}

void setup_fpga_cs1() { 
  int i;
  //  printf( "setting up EIM CS1 (burst interface) pads and configuring timing\n" );
  // ASSUME: setup_fpga() is already called to configure gpio mux setting.
  // this just gets the pads set to high-speed mode

  // set up pads to be mapped to EIM
  for( i = 0; i < 16; i++ ) {
    write_kernel_memory( 0x20e0428 + i*4, 0xb0f1, 0, 4 ); // pad strength config'd for a 200MHz rate 
  }

  // pad strength
  write_kernel_memory( 0x20e046c, 0xb0f1, 0, 4 ); // BCLK
  //  write_kernel_memory( 0x20e040c, 0xb0b1, 0, 4 ); // CS0
  write_kernel_memory( 0x20e0410, 0xb0f1, 0, 4 ); // CS1
  write_kernel_memory( 0x20e0414, 0xb0f1, 0, 4 ); // OE
  write_kernel_memory( 0x20e0418, 0xb0f1, 0, 4 ); // RW
  write_kernel_memory( 0x20e041c, 0xb0f1, 0, 4 ); // LBA
  write_kernel_memory( 0x20e0468, 0xb0f1, 0, 4 ); // WAIT
  write_kernel_memory( 0x20e0408, 0xb0f1, 0, 4 ); // A16
  write_kernel_memory( 0x20e0404, 0xb0f1, 0, 4 ); // A17
  write_kernel_memory( 0x20e0400, 0xb0f1, 0, 4 ); // A18

  // EIM_CS1GCR1   
  // 0011 0  001 1   001    0   001 00  00  1  011  1    0   1   1   1   1   1   1
  // PSZ  WP GBC AUS CSREC  SP  DSZ BCS BCD WC BL   CREP CRE RFL WFL MUM SRD SWR CSEN
  //
  // PSZ = 0011  64 words page size
  // WP = 0      (not protected)
  // GBC = 001   min 1 cycles between chip select changes
  // AUS = 0     address shifted according to port size
  // CSREC = 001 min 1 cycles between CS, OE, WE signals
  // SP = 0      no supervisor protect (user mode access allowed)
  // DSZ = 001   16-bit port resides on DATA[15:0]
  // BCS = 00    0 clock delay for burst generation
  // BCD = 00    divide EIM clock by 0 for burst clock
  // WC = 1      write accesses are continuous burst length
  // BL = 011    32 word memory wrap length
  // CREP = 1    non-PSRAM, set to 1
  // CRE = 0     CRE is disabled
  // RFL = 1     fixed latency reads
  // WFL = 1     fixed latency writes
  // MUM = 1     multiplexed mode enabled
  // SRD = 1     synch reads
  // SWR = 1     synch writes
  // CSEN = 1    chip select is enabled

  // 0101 0111 1111    0001 1100  0000  1011   1   0   0   1
  // 0x5  7    F        1   C     0     B    9

  // 0101 0001 1001    0001 1100  0000  1011   1001
  // 5     1    9       1    c     0     B      9

  // 0011 0001 1001    0001 0000  1011  1011   1111

  write_kernel_memory( 0x21b8000 + 0x18, 0x31910BBF, 0, 4 );

  // EIM_CS1GCR2   
  //  MUX16_BYP_GRANT = 1
  //  ADH = 0 (0 cycles)
  //  0x1000
  write_kernel_memory( 0x21b8004 + 0x18, 0x1000, 0, 4 );


  // 9 cycles is total length of read
  // 2 cycles for address
  // +4 more cycles for first data to show up

  // EIM_CS1RCR1   
  // 00 000100 0 000   0   001   0 010 0 000 0 000 0 000
  //    RWSC     RADVA RAL RADVN   OEA   OEN   RCSA  RCSN
  //
  // 00 001001 0 000   0   001   0 110 0 000 0 000 0 000
  //    RWSC     RADVA RAL RADVN   OEA   OEN   RCSA  RCSN
  //
  // 0000 0111 0000   0011   0000 0000 0000 0000
  //  0    7     0     3      0  0    0    0
  // 0000 0101 0000   0000   0 000 0 000 0 000 0 000
//  write_kernel_memory( 0x21b8008, 0x05000000, 0, 4 );
  // 0000 0011 0000   0001   0001 0000 0000 0000

  // 0000 1001 0000   0001   0110 0000 0000 0000
  // 
  write_kernel_memory( 0x21b8008 + 0x18, 0x09014000, 0, 4 );

  // EIM_CS1RCR2  
  // 0000 0000 0   000 00 00 0 010  0 001 
  //           APR PAT    RL   RBEA   RBEN
  // APR = 0   mandatory because MUM = 1
  // PAT = XXX because APR = 0
  // RL = 00   because async mode
  // RBEA = 000  these match RCSA/RCSN from previous field
  // RBEN = 000
  // 0000 0000 0000 0000 0000  0000
  write_kernel_memory( 0x21b800c + 0x18, 0x00000200, 0, 4 );

  // EIM_CS1WCR1
  // 0   0    000010 000   001   000  000  010 000 000  000
  // WAL WBED WWSC   WADVA WADVN WBEA WBEN WEA WEN WCSA WCSN
  // WAL = 0       use WADVN
  // WBED = 0      allow BE during write
  // WWSC = 000100 4 write wait states
  // WADVA = 000   same as RADVA
  // WADVN = 000   this sets WE length to 1 (this value +1)
  // WBEA = 000    same as RBEA
  // WBEN = 000    same as RBEN
  // WEA = 010     2 cycles between beginning of access and WE assertion
  // WEN = 000     1 cycles to end of WE assertion
  // WCSA = 000    cycles to CS assertion
  // WCSN = 000    cycles to CS negation
  // 1000 0111 1110 0001 0001  0100 0101 0001
  // 8     7    E    1    1     4    5    1
  // 0000 0111 0000 0100 0000  1000 0000 0000
  // 0      7    0   4    0     8    0     0
  // 0000 0100 0000 0000 0000  0100 0000 0000
  //  0    4    0    0     0    4     0    0

  // 0000 0010 0000 0000 0000  0010 0000 0000
  // 0000 0010 0000 0100 0000  0100 0000 0000

  write_kernel_memory( 0x21b8010 + 0x18, 0x02040400, 0, 4 );

  // EIM_WCR
  // BCM = 1   free-run BCLK
  // GBCD = 0  divide the burst clock by 1
  // add timeout watchdog after 1024 bclk cycles
  write_kernel_memory( 0x21b8090, 0x701, 0, 4 );

  // EIM_WIAR 
  // ACLK_EN = 1
  write_kernel_memory( 0x21b8094, 0x10, 0, 4 );

  //  printf( "resetting CS0 space to 64M and enabling 64M CS1 space.\n" );
  write_kernel_memory( 0x20e0004, 
		       (read_kernel_memory(0x20e0004, 0, 4) & 0xFFFFFFC0) |
		       0x1B, 0, 4);

  //  printf( "done.\n" );
}


int testcs1() {
  unsigned long long i;
  unsigned long long retval;
  volatile unsigned long long *cs1;
  unsigned long long testbuf[16];
  unsigned long long origbuf[16];

  if(mem_32)
    munmap(mem_32, 0xFFFF);
  if(fd)
    close(fd);

  fd = open("/dev/mem", O_RDWR);
  if( fd < 0 ) {
    perror("Unable to open /dev/mem");
    fd = 0;
    return 0;
  }

  mem_32 = mmap(0, 0xffff, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0x0C040000);
  cs1 = (unsigned long long *)mem_32;

  for( i = 0; i < 2; i++ ) {
    testbuf[i] = i | (i + 64) << 16 | (i + 8) << 32 | (i + 16) << 48 ;
  }
  testbuf[0] = 0xdeadbeeffeedfaceLL;
  //  testbuf[0] = 0x0LL;
  testbuf[1] = 0x5555aaaa33339999LL;


  retval = 0;

  //  memcpy( (void *) cs1, testbuf, 2*8);
  origbuf[0] = testbuf[0];
  origbuf[1] = testbuf[1];
  cs1[0] = testbuf[0];
  cs1[1] = testbuf[1];

  for( i = 0; i < 2; i++ ) {
    testbuf[i] = 0;
  }

  memcpy(testbuf,(void *) cs1, 8);
  memcpy(&(testbuf[1]),(void *)cs1 + 8, 8);
  
  for( i = 0; i < 2; i++ ) {
    printf( "%lld: %016llx\n", i, origbuf[i] );
    printf( "%lld: %016llx\n", i, testbuf[i] );
  }

  //  cs1[0] = 0xdeadbeeffeedfaceLL;
  //  cs1[1] = 0x12456789abcdef01LL;
  //  cs1[2] = 0xf0f0f0f0f0f0f0f0LL;
  //  cs1[3] = 0x12345555aaaa9876LL;

  return retval;
}


int main(int argc, char **argv) {
  char *prog = argv[0];
  unsigned int a1;
  
  argv++;
  argc--;

  setup_fpga();
  setup_fpga_cs1();

  if(!argc) {
    print_usage(prog);
    return 1;
  }

  while(argc > 0) {
    if(!strcmp(*argv, "-h")) {
      argc--;
      argv++;
      print_usage(prog);
    } 
    else if(!strcmp(*argv, "-v")) {
      argc--;
      argv++;
      printf( "FPGA version code: %04hx.%04hx\n", 
	      read_kernel_memory(FPGA_R_V_MINOR, 0, 2),
	      read_kernel_memory(FPGA_R_V_MAJOR, 0, 2) );
    }

    else if(!strcmp(*argv, "-da")) {
      argc--;
      argv++;
      if( argc != 1 ) {
	printf( "usage -da <code>\n" );
	return 1;
      }

      a1 = strtoul(*argv, NULL, 10);
      argc--;
      argv++;
      dac_a_set(a1 * 4); // implementation sends 2 dummy bits
    }

    else if(!strcmp(*argv, "-db")) {
      argc--;
      argv++;
      if( argc != 1 ) {
	printf( "usage -db <code>\n" );
	return 1;
      }

      a1 = strtoul(*argv, NULL, 10);
      argc--;
      argv++;
      dac_b_set(a1 * 4);
    }

    else if(!strcmp(*argv, "-a")) {
      argc--;
      argv++;
      if( argc != 1 ) {
	printf( "usage -a <chan>\n" );
	return 1;
      }

      a1 = strtoul(*argv, NULL, 10);
      argc--;
      argv++;
      adc_chan(a1);
      
      printf( "ADC channel %d: %d\n", a1, adc_read() );;
    }

    else if(!strcmp(*argv, "-hv")) {
      argc--;
      argv++;
      setvddio(1);
    }

    else if(!strcmp(*argv, "-lv")) {
      argc--;
      argv++;
      setvddio(0);
    }

    else if(!strcmp(*argv, "-oea")) {
      argc--;
      argv++;
      if( argc != 1 ) {
	printf( "usage -oea <drive>\n" );
	return 1;
      }

      a1 = strtoul(*argv, NULL, 10);
      argc--;
      argv++;
      oe_state(a1, OE_A);
    }

    else if(!strcmp(*argv, "-oeb")) {
      argc--;
      argv++;
      if( argc != 1 ) {
	printf( "usage -oeb <drive>\n" );
	return 1;
      }

      a1 = strtoul(*argv, NULL, 10);
      argc--;
      argv++;
      oe_state(a1, OE_B);
    }

    else if(!strcmp(*argv, "-testcs1")) {
      argc--;
      argv++;
      testcs1();
    }

    else {
      print_usage(prog);
      return 1;
    }
  }

  return 0;
}
