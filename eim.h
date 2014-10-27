#ifndef __eim_h__
#define __eim_h__

enum eim_type {
	fpga_w_test0        = 0x0000,
	fpga_w_test1        = 0x0002,
	fpga_w_gpioa_dout   = 0x0010,
	fpga_w_gpioa_dir    = 0x0012,

	fpga_w_ddr3_p2_cmd  = 0x0020,
	fpga_w_ddr3_p2_ladr = 0x0022,
	fpga_w_ddr3_p2_hadr = 0x0024,
	fpga_w_ddr3_p2_wen  = 0x0026,
	fpga_w_ddr3_p2_ldat = 0x0028,
	fpga_w_ddr3_p2_hdat = 0x002A,

	fpga_w_ddr3_p3_cmd  = 0x0030,
	fpga_w_ddr3_p3_ladr = 0x0032,
	fpga_w_ddr3_p3_hadr = 0x0034,
	fpga_w_ddr3_p3_ren  = 0x0036,

	fpga_w_nand_uk_ctl  = 0x0100,
	fpga_w_nand_power   = 0x0102,

	fpga_r_test0        = 0x1000,
	fpga_r_test1        = 0x1002,
	fpga_r_ddr3_cal     = 0x1004,
	fpga_r_gpioa_din    = 0x1010,

	fpga_r_ddr3_p2_stat = 0x1020,
	fpga_r_ddr3_p3_stat = 0x1030,
	fpga_r_ddr3_p3_ldat = 0x1032,
	fpga_r_ddr3_p3_hdat = 0x1034,

	// every time I read it auto-advances the queue
	fpga_r_nand_uk_data = 0x1100,
	fpga_r_nand_uk_stat = 0x1102,

	fpga_r_ddr3_v_minor = 0x1FFC,
	fpga_r_ddr3_v_major = 0x1FFE,

};

uint16_t *eim_get(enum eim_type type);

#endif // __eim_h__
