#include "ddrconf.h"

uint32_t ddrconf_get_pll3mult(void)   { return 41; }
uint32_t ddrconf_get_pll3outdiv(void) { return  2; }

uint32_t denali_setup[92] = {
[ 0] =	0x00000400,
[ 1] =	0x00000000,
[ 2] =	0x000061a8,
[ 3] =	0x00000000,
[ 4] =	0x00000000,
[ 5] =	0x00000000,
[ 6] =	0x02030802,
[ 7] =	0x05070202,
[ 8] =	0x02020201,
[ 9] =	0x03222e02,
[10] =	0x01000003,
[11] =	0x04020201,
[12] =	0x000000c8,
[13] =	0x00320702,
[14] =	0x00000303,
[15] =	0x00190100,
[16] =	0x000503c8,
[17] =	0x00000000,
[18] =	0x001a00c8,
[19] =	0x01000000,
[20] =	0x00000000,
[21] =	0x00000000,
[22] =	0x00000000,
[23] =	0x00000303,
[24] =	0x00000000,
[25] =	0x00000000,
[26] =	0x00000000,
[27] =	0x00440242,
[28] =	0x00000000,
[29] =	0x00000000,
[30] =	0x00024200,
[31] =	0x00000004,
[32] =	0x00000000,
[33] =	0x00000000,
[34] =	0x01000200,
[35] =	0x40000040,
[36] =	0x00000002,
[37] =	0x01010100,
[38] =	0x02020101,
[39] =	0x00000a0a,
[40] =	0x0fff1000,
[41] =	0x7f000fff,
[42] =	0x0101017f,
[43] =	0x01010101,
[44] =	0x00000300,
[45] =	0x01000000,
[46] =	0x00000000,
[47] =	0x00000000,
[48] =	0x00000000,
[49] =	0x00000000,
[50] =	0x00000000,
[51] =	0x00000000,
[52] =	0x02010100,
[53] =	0x00000002,
[54] =	0x01020100,
[55] =	0x00020001,
[56] =	0x00010100,
[57] =	0x0f0f0f00,
[58] =	0x0f0f0f0f,
[59] =	0x04FFFF0f,
[60] =	0x04ffFF04,
[61] =	0x04FFff04,
[62] =	0x04FFFF04,
[63] =	0x04000104,
[64] =	0x04000104,
[65] =	0x04000104,
[66] =	0x04000104,
[67] =	0x00000004,
[68] =	0x00000000,
[69] =	0x00000000,
[70] =	0x00000000,
[71] =	0x00000000,
[72] =	0x00000000,
[73] =	0x00000000,
[74] =	0x00000000,
[75] =	0x00000000,
[76] =	0x00000000,
[77] =	0x07000000,
[78] =	0x19000119,
[79] =	0x01190001,
[80] =	0x00011900,
[81] =	0x19000119,
[82] =	0x01190001,
[83] =	0x00011900,
[84] =	0x00000001,
[85] =	0x06020000,
[86] =	0x00000200,
[87] =	0x03c803c8,
[88] =	0x03c803c8,
[89] =	0x03c803c8,
[90] =	0x00020304,
[91] =	0x00000101,
};

uint32_t denali_phy_setup[39] = {
[ 0] =	0x00000000,
[ 1] =	0x000f2100,
[ 2] =	0xf4013a27,
[ 3] =	0x26c002c0,
[ 4] =	0xf4013a27,
[ 5] =	0x26c002c0,
[ 6] =	0xf4013a27,
[ 7] =	0x26c002c0,
[ 8] =	0xf4013a27,
[ 9] =	0x26c002c0,
[10] =	0xf4013a27,
[11] =	0x26c002c0,
[12] =	0x00000000,
[13] =	0x00000005,
[14] =	0xe02032A5,
[15] =	0x00149f04,
[16] =	0xe02032A5,
[17] =	0x00149f04,
[18] =	0xe02032A5,
[19] =	0x00149f04,
[20] =	0xe02032A5,
[21] =	0x00149f04,
[22] =	0xe02032A5,
[23] =	0x00149f04,
[24] =	0x00000000,
[25] =	0x00000000,
[26] =	0x00000000,
[27] =	0x00000000,
[28] =	0x00000000,
[29] =	0x00000000,
[30] =	0x00000000,
[31] =	0x00000000,
[32] =	0x00000000,
[33] =	0x00000000,
[34] =	0x00000000,
[35] =	0x00000000,
[36] =	0x00000000,
[37] =	0x00000000,
[38] =	0x00000000,
};