/////////////// novena FPGA I2C register set
#define FPGA_I2C_ADR        (0x3C >> 1) 
#define FPGA_I2C_LOOPBACK   0x0

#define FPGA_I2C_ADC_CTL    0x2

#define FPGA_I2C_ADC_DAT_L  0x40
#define FPGA_I2C_ADC_DAT_H  0x41
#define FPGA_I2C_ADC_VALID  0x42

#define FPGA_I2C_V_MIN_L    0xfc
#define FPGA_I2C_V_MIN_H    0xfd
#define FPGA_I2C_V_MAJ_L    0xfe
#define FPGA_I2C_V_MAJ_H    0xff


#define FPGA_REG_OFFSET        0x08040000
#define FPGA_CS1_REG_OFFSET    0x0C040000

#define FPGA_MAP(x)         ( (x - FPGA_REG_OFFSET) >> 1 )
#define F(x)                ( (x - FPGA_REG_OFFSET) >> 1 )
#define F1(x)               ( (x - FPGA_CS1_REG_OFFSET) >> 3 )

//////////// novena FPGA EIM register set
// CS0 registers. 
#define FPGA_W_TEST0        0x08040000
#define FPGA_W_TEST1        0x08040002

#define FPGA_W_CPU_TO_DUT   0x08040010
#define FPGA_W_GPBB_CTL     0x08040012

#define FPGA_R_TEST0        0x08041000
#define FPGA_R_TEST1        0x08041002

#define FPGA_R_DUT_TO_CPU   0x08041010
#define FPGA_R_GPBB_STAT    0x08041012

#define FPGA_R_V_MINOR      0x08041FFC
#define FPGA_R_V_MAJOR      0x08041FFE

// burst access registers (in CS1 bank -- only 64-bit access allowed)
#define FPGA_WB_LOOP0       0x0C040000
#define FPGA_WB_LOOP1       0x0C040008

#define FPGA_RB_LOOP0       0x0C041000
#define FPGA_RB_LOOP1       0x0C041008
