#include <stdio.h> 
#include "i2c.h"

volatile struct twi *const twi = (void*)TWI_REG;



static inline void twiStart(void)
{
    twi->twcr = TWINT | TWSTA | TWEN;
}

static inline void twiStop(void)
{
    twi->twcr = TWINT | TWSTO | TWEN;
}

static inline void twiWaitAck(void)
{
    while (!(twi->twcr & TWINT));
}

static inline uint8_t twiChkAck(void)
{
    return twi->twsr & TW_STS;
}

static inline void twiSendAck(void)
{
    twi->twcr |= TWEA;
}

static inline void twiSendNack(void)
{
    twi->twcr &= ~TWEA;
}

static inline void twiSendByte(uint8_t data)
{
    twi->twdr = data;
    twi->twcr = TWINT | TWEN;
}

static inline void twiRcvNackByte(void)
{
    twi->twcr = TWINT | TWEN;
}

static inline void twiRcvAckByte(void)
{
    twi->twcr = TWINT | TWEN | TWEA;
}

static int twi_start(void)
{
    twiStart();
    twiWaitAck();
    if (twiChkAck() != TWI_START)
        return -1;

    return 0;
}

static void twi_stop(void)
{
    twiStop();
}

static int twi_restart(void)
{
    twiStart();
    twiWaitAck();
    if (twiChkAck() != TWI_RESTART)
        return -1;

    return 0;
}

static int twi_send_dev_addr(uint8_t addr)
{
    uint8_t flag = addr & 1 ? TWI_MR_SLA_ACK : TWI_MT_SLA_ACK;

    twiSendByte(addr);
    twiWaitAck();
    if (twiChkAck() != flag)
        return -1;

    return 0;
}

static int twi_send_data(uint8_t data)
{
    twiSendByte(data);
    twiWaitAck();
    if (twiChkAck() != TWI_MT_DATA_ACK)
        return -1;

    return 0;
}

static int twi_read_ack_data(uint8_t *data)
{
    twiRcvAckByte();
    twiWaitAck();
    if (twiChkAck() != TWI_MR_DATA_ACK)
        return -1;

    *data = twi->twdr;
    return 0;
}

static int twi_read_nack_data(uint8_t *data)
{
    twiRcvNackByte();
    twiWaitAck();
    if (twiChkAck() != TWI_MR_DATA_NACK)
        return -1;

    *data = twi->twdr;
    return 0;
}

int twi_write(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, int len)
{
    int     i;

    if (twi_start())
        return -1;

    if (twi_send_dev_addr((dev_addr << 1) | TWI_WR))
        return -1;

    if (twi_send_data(reg_addr))
        return -1;

    for (i=0; i<len; i++)
    {
        if (twi_send_data(*data++))
            return -1;
    }

    twi_stop();

    return 0;
}

int twi_tx_start(uint8_t dev_addr)
{
    if (twi_start())
        return -1;

    if (twi_send_dev_addr((dev_addr << 1) | TWI_WR))
        return -1;

    return 0;
}

int twi_tx(uint8_t data)
{
    if (twi_send_data(data))
        return -1;

    return 0;
}

int twi_tx_end(void)
{
    twi_stop();

    return 0;
}

int twi_read(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, int len)
{
    int     i;

    if (twi_start())
        return -1;

    if (twi_send_dev_addr((dev_addr << 1) | TWI_WR))
        return -1;

    if (twi_send_data(reg_addr))
        return -1;

    if (twi_restart())
        return -1;

    if (twi_send_dev_addr((dev_addr << 1) | TWI_RD))
        return -1;

    for (i=0; i<len-1; i++)
    {
        if (twi_read_ack_data(data + i))
            return -1;
    }
    if (twi_read_nack_data(data + i))
        return -1;

    twi_stop();

    return 0;
}

void twi_init(void)
{
    twi->twbr = 5;
    twi->twsr = 0;
}


