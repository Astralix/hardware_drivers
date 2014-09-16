#ifndef __BOARD_DSPG_DW_CHARGER_H
#define __BOARD_DSPG_DW_CHARGER_H

int dp_charger_init(int auxbgp, int atten, int gain);
int dp_charger_set(void);
void dp_charger_reset(void);
void dp_aux_enable(u16 bandgap);
void dp_aux_mux(unsigned short mux);
signed int get_temperature(u16 auxbgp, u16 tempref);
int get_batt_ntc(u16 auxbgp);
int get_mVoltage(u16 auxbgp, u16 atten);
void power_off(void);

void do_dpm_fastcharge(unsigned int auxbgp, unsigned int atten42, unsigned int atten38, unsigned int tempref, unsigned int min_mv);

#define BATTERY_THERMISTOR_MAX_TABLE_LENGTH 40

struct battery_thermistor {
	int voltage_mul_10000[BATTERY_THERMISTOR_MAX_TABLE_LENGTH];
	int temperature_in_c_mul_10[BATTERY_THERMISTOR_MAX_TABLE_LENGTH];
	int table_length;
};

#endif
