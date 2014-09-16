#define IIC_RX		0x00
#define IIC_TX		0x00
#define IIC_STS		0x04
#define IIC_CTL		0x08
#define IIC_CLKHI	0x0C
#define IIC_CLKLO	0x10
#define IIC_ADDR	0x14
#define IIC_HOLDDAT	0x18
#define IIC_TXS		0x28
// Bit field
#define START_BIT			0x1<<8
#define STOP_BIT				0x1<<9

#define NACK 				0x1<<2
#define IIC_BUS_BUSY		0x1<<5 	
