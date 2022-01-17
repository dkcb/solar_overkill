

#ifndef JBD_H_
#define JBD_H_

#include <stdint.h>

#define JBD_MAX_CELLS 8
#define JBD_MAX_NTC 4

enum {
    JBD_PROT_OVP_CELL       = 1 << 0,
    JBD_PROT_UVP_CELL       = 1 << 1,
    JBD_PROT_OVP_PACK       = 1 << 2,
    JBD_PROT_UVP_PACK       = 1 << 3,

    JBD_PROT_OTP_CHARGE     = 1 << 4,
    JBD_PROT_UPT_CHARGE     = 1 << 5,
    JBD_PROT_OTP_DISCARGE   = 1 << 6,
    JBD_PROT_UTP_DISCARGE   = 1 << 7,
    JBD_PROT_OCP_CHARGE     = 1 << 8,
    JBD_PROT_OCP_DISCHARGE  = 1 << 9,
    JBD_PROT_OCP_SHORT      = 1 << 10,
    JBD_PROT_IC_ERROR       = 1 << 11,
    JBD_PROT_FET_LOCK       = 1 << 12
};

enum {
    JDB_FET_CHARGE          = 1 << 0,
    JDB_FET_DISCHARGE       = 1 << 1
};

typedef struct {
    uint16_t voltage;
    uint16_t current;
    uint16_t rem_capacity;
    uint16_t typ_capacity;
    uint16_t cycles;
    uint16_t prod_date;
    uint32_t balance_status;
    uint16_t prot_status;
    uint8_t  software_version;
    uint8_t  rem_cap_pct;
    uint8_t  fet_status;
    uint8_t  cell_cnt;
    uint8_t  ntc_cnt;
    uint16_t temps[JBD_MAX_NTC]
} jbd_basic_info_t;

typedef uint16_t[JBD_MAX_CELLS] jbd_cell_volts_t;


typedef struct {

} 

#endif
