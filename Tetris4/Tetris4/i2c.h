#define TWI_REG     0x70

struct twi
{
    uint8_t     twbr;
    uint8_t     twsr;
    uint8_t     twar;
    uint8_t     twdr;
    uint8_t     twcr;
};


#define TW_STS      0xF8
#define TWINT       0x80
#define TWEA        0x40
#define TWSTA       0x20
#define TWSTO       0x10
#define TWWC        0x08
#define TWEN        0x04
#define TWIE        0x01

#define TWI_SCL     0x20
#define TWI_SDA     0x10
#define TWI_RD      0x01
#define TWI_WR      0x00

#define TWI_START			0x08
#define TWI_RESTART			0x10

/* Master Transmitter */
#define TWI_MT_SLA_ACK		0x18
#define TWI_MT_SLA_NACK		0x20
#define TWI_MT_DATA_ACK		0x28
#define TWI_MT_DATA_NACK	0x30
#define TWI_MT_ARB_LOST		0x38

/* Master Receiver */
#define TWI_MR_ARB_LOST		0x38
#define TWI_MR_SLA_ACK		0x40
#define TWI_MR_SLA_NACK		0x48
#define TWI_MR_DATA_ACK		0x50
#define TWI_MR_DATA_NACK	0x58

int twi_write(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, int len);
int twi_read(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, int len);
void twi_init(void);
int twi_tx_start(uint8_t dev_addr);
int twi_tx(uint8_t data);
int twi_tx_end(void);
