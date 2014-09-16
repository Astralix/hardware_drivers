#include "common.h"
#include "hardware.h"
#include "cmu.h"
#include "syscfg.h"
#include "ddr.h"
#include "ddrconf/ddrconf.h"

#define EN_DLL_CALIB_BYPASS  0x10000000 /* dmw96 revA */
#define DIS_DLL_CALIB_BYPASS 0x00000000 /* dmw96 revB and higher */

/* denali parameter fields */
#define param_add_odt_clk_difftype_diffcs_bits 5
#define param_add_odt_clk_difftype_diffcs_reg  53
#define param_add_odt_clk_difftype_diffcs_offset 24
#define param_add_odt_clk_r2w_samecs_bits 4
#define param_add_odt_clk_r2w_samecs_reg  53
#define param_add_odt_clk_r2w_samecs_offset 8
#define param_add_odt_clk_sametype_diffcs_bits 4
#define param_add_odt_clk_sametype_diffcs_reg  54
#define param_add_odt_clk_sametype_diffcs_offset 0
#define param_add_odt_clk_w2r_samecs_bits 4
#define param_add_odt_clk_w2r_samecs_reg  53
#define param_add_odt_clk_w2r_samecs_offset 16
#define param_addr_cmp_en_bits 1
#define param_addr_cmp_en_reg  42
#define param_addr_cmp_en_offset 8
#define param_addr_pins_0_bits 3
#define param_addr_pins_0_reg  38
#define param_addr_pins_0_offset 0
#define param_addr_pins_1_bits 3
#define param_addr_pins_1_reg  38
#define param_addr_pins_1_offset 8
#define param_age_count_bits 7
#define param_age_count_reg  41
#define param_age_count_offset 24
#define param_ahb0_rdlen_bits 4
#define param_ahb0_rdlen_reg  57
#define param_ahb0_rdlen_offset 16
#define param_ahb1_rdlen_bits 4
#define param_ahb1_rdlen_reg  58
#define param_ahb1_rdlen_offset 0
#define param_ahb2_rdlen_bits 4
#define param_ahb2_rdlen_reg  58
#define param_ahb2_rdlen_offset 16
#define param_ahb3_rdlen_bits 4
#define param_ahb3_rdlen_reg  59
#define param_ahb3_rdlen_offset 0
#define param_ahb0_wrlen_bits 4
#define param_ahb0_wrlen_reg  57
#define param_ahb0_wrlen_offset 8
#define param_ahb1_wrlen_bits 4
#define param_ahb1_wrlen_reg  57
#define param_ahb1_wrlen_offset 24
#define param_ahb2_wrlen_bits 4
#define param_ahb2_wrlen_reg  58
#define param_ahb2_wrlen_offset 8
#define param_ahb3_wrlen_bits 4
#define param_ahb3_wrlen_reg  58
#define param_ahb3_wrlen_offset 24
#define param_ap_bits 1
#define param_ap_reg  10
#define param_ap_offset 16
#define param_aprebit_0_bits 4
#define param_aprebit_0_reg  39
#define param_aprebit_0_offset 0
#define param_aprebit_1_bits 4
#define param_aprebit_1_reg  39
#define param_aprebit_1_offset 8
#define param_arb_cmd_q_threshold_bits 4
#define param_arb_cmd_q_threshold_reg  77
#define param_arb_cmd_q_threshold_offset 24
#define param_arefresh_bits 1
#define param_arefresh_reg  14
#define param_arefresh_offset 24
#define param_auto_refresh_mode_bits 1
#define param_auto_refresh_mode_reg  15
#define param_auto_refresh_mode_offset 0
#define param_auto_tempchk_val_bits 8
#define param_auto_tempchk_val_reg  26
#define param_auto_tempchk_val_offset 16
#define param_axi0_bdw_bits 7
#define param_axi0_bdw_reg  78
#define param_axi0_bdw_offset 0
#define param_axi1_bdw_bits 7
#define param_axi1_bdw_reg  78
#define param_axi1_bdw_offset 24
#define param_axi2_bdw_bits 7
#define param_axi2_bdw_reg  79
#define param_axi2_bdw_offset 16
#define param_axi3_bdw_bits 7
#define param_axi3_bdw_reg  80
#define param_axi3_bdw_offset 8
#define param_axi4_bdw_bits 7
#define param_axi4_bdw_reg  81
#define param_axi4_bdw_offset 0
#define param_axi5_bdw_bits 7
#define param_axi5_bdw_reg  81
#define param_axi5_bdw_offset 24
#define param_axi6_bdw_bits 7
#define param_axi6_bdw_reg  82
#define param_axi6_bdw_offset 16
#define param_axi7_bdw_bits 7
#define param_axi7_bdw_reg  83
#define param_axi7_bdw_offset 8
#define param_axi0_bdw_ovflow_bits 1
#define param_axi0_bdw_ovflow_reg  78
#define param_axi0_bdw_ovflow_offset 8
#define param_axi1_bdw_ovflow_bits 1
#define param_axi1_bdw_ovflow_reg  79
#define param_axi1_bdw_ovflow_offset 0
#define param_axi2_bdw_ovflow_bits 1
#define param_axi2_bdw_ovflow_reg  79
#define param_axi2_bdw_ovflow_offset 24
#define param_axi3_bdw_ovflow_bits 1
#define param_axi3_bdw_ovflow_reg  80
#define param_axi3_bdw_ovflow_offset 16
#define param_axi4_bdw_ovflow_bits 1
#define param_axi4_bdw_ovflow_reg  81
#define param_axi4_bdw_ovflow_offset 8
#define param_axi5_bdw_ovflow_bits 1
#define param_axi5_bdw_ovflow_reg  82
#define param_axi5_bdw_ovflow_offset 0
#define param_axi6_bdw_ovflow_bits 1
#define param_axi6_bdw_ovflow_reg  82
#define param_axi6_bdw_ovflow_offset 24
#define param_axi7_bdw_ovflow_bits 1
#define param_axi7_bdw_ovflow_reg  83
#define param_axi7_bdw_ovflow_offset 16
#define param_axi0_current_bdw_bits 7
#define param_axi0_current_bdw_reg  78
#define param_axi0_current_bdw_offset 16
#define param_axi1_current_bdw_bits 7
#define param_axi1_current_bdw_reg  79
#define param_axi1_current_bdw_offset 8
#define param_axi2_current_bdw_bits 7
#define param_axi2_current_bdw_reg  80
#define param_axi2_current_bdw_offset 0
#define param_axi3_current_bdw_bits 7
#define param_axi3_current_bdw_reg  80
#define param_axi3_current_bdw_offset 24
#define param_axi4_current_bdw_bits 7
#define param_axi4_current_bdw_reg  81
#define param_axi4_current_bdw_offset 16
#define param_axi5_current_bdw_bits 7
#define param_axi5_current_bdw_reg  82
#define param_axi5_current_bdw_offset 8
#define param_axi6_current_bdw_bits 7
#define param_axi6_current_bdw_reg  83
#define param_axi6_current_bdw_offset 0
#define param_axi7_current_bdw_bits 7
#define param_axi7_current_bdw_reg  83
#define param_axi7_current_bdw_offset 24
#define param_axi0_en_size_lt_width_instr_bits 16
#define param_axi0_en_size_lt_width_instr_reg  59
#define param_axi0_en_size_lt_width_instr_offset 8
#define param_axi1_en_size_lt_width_instr_bits 16
#define param_axi1_en_size_lt_width_instr_reg  60
#define param_axi1_en_size_lt_width_instr_offset 8
#define param_axi2_en_size_lt_width_instr_bits 16
#define param_axi2_en_size_lt_width_instr_reg  61
#define param_axi2_en_size_lt_width_instr_offset 8
#define param_axi3_en_size_lt_width_instr_bits 16
#define param_axi3_en_size_lt_width_instr_reg  62
#define param_axi3_en_size_lt_width_instr_offset 8
#define param_axi4_en_size_lt_width_instr_bits 16
#define param_axi4_en_size_lt_width_instr_reg  63
#define param_axi4_en_size_lt_width_instr_offset 8
#define param_axi5_en_size_lt_width_instr_bits 16
#define param_axi5_en_size_lt_width_instr_reg  64
#define param_axi5_en_size_lt_width_instr_offset 8
#define param_axi6_en_size_lt_width_instr_bits 16
#define param_axi6_en_size_lt_width_instr_reg  65
#define param_axi6_en_size_lt_width_instr_offset 8
#define param_axi7_en_size_lt_width_instr_bits 16
#define param_axi7_en_size_lt_width_instr_reg  66
#define param_axi7_en_size_lt_width_instr_offset 8
#define param_axi0_r_priority_bits 3
#define param_axi0_r_priority_reg  59
#define param_axi0_r_priority_offset 24
#define param_axi1_r_priority_bits 3
#define param_axi1_r_priority_reg  60
#define param_axi1_r_priority_offset 24
#define param_axi2_r_priority_bits 3
#define param_axi2_r_priority_reg  61
#define param_axi2_r_priority_offset 24
#define param_axi3_r_priority_bits 3
#define param_axi3_r_priority_reg  62
#define param_axi3_r_priority_offset 24
#define param_axi4_r_priority_bits 3
#define param_axi4_r_priority_reg  63
#define param_axi4_r_priority_offset 24
#define param_axi5_r_priority_bits 3
#define param_axi5_r_priority_reg  64
#define param_axi5_r_priority_offset 24
#define param_axi6_r_priority_bits 3
#define param_axi6_r_priority_reg  65
#define param_axi6_r_priority_offset 24
#define param_axi7_r_priority_bits 3
#define param_axi7_r_priority_reg  66
#define param_axi7_r_priority_offset 24
#define param_axi0_w_priority_bits 3
#define param_axi0_w_priority_reg  60
#define param_axi0_w_priority_offset 0
#define param_axi1_w_priority_bits 3
#define param_axi1_w_priority_reg  61
#define param_axi1_w_priority_offset 0
#define param_axi2_w_priority_bits 3
#define param_axi2_w_priority_reg  62
#define param_axi2_w_priority_offset 0
#define param_axi3_w_priority_bits 3
#define param_axi3_w_priority_reg  63
#define param_axi3_w_priority_offset 0
#define param_axi4_w_priority_bits 3
#define param_axi4_w_priority_reg  64
#define param_axi4_w_priority_offset 0
#define param_axi5_w_priority_bits 3
#define param_axi5_w_priority_reg  65
#define param_axi5_w_priority_offset 0
#define param_axi6_w_priority_bits 3
#define param_axi6_w_priority_reg  66
#define param_axi6_w_priority_offset 0
#define param_axi7_w_priority_bits 3
#define param_axi7_w_priority_reg  67
#define param_axi7_w_priority_offset 0
#define param_bank_split_en_bits 1
#define param_bank_split_en_reg  42
#define param_bank_split_en_offset 16
#define param_bstlen_bits 3
#define param_bstlen_reg  13
#define param_bstlen_offset 0
#define param_caslat_lin_bits 5
#define param_caslat_lin_reg  6
#define param_caslat_lin_offset 8
#define param_cke_delay_bits 3
#define param_cke_delay_reg  20
#define param_cke_delay_offset 0
#define param_cke_status_bits 1
#define param_cke_status_reg  84
#define param_cke_status_offset 8
#define param_cksre_bits 4
#define param_cksre_reg  23
#define param_cksre_offset 0
#define param_cksrx_bits 4
#define param_cksrx_reg  23
#define param_cksrx_offset 8
#define param_column_size_0_bits 3
#define param_column_size_0_reg  38
#define param_column_size_0_offset 16
#define param_column_size_1_bits 3
#define param_column_size_1_reg  38
#define param_column_size_1_offset 24
#define param_command_age_count_bits 7
#define param_command_age_count_reg  42
#define param_command_age_count_offset 0
#define param_concurrentap_bits 1
#define param_concurrentap_reg  10
#define param_concurrentap_offset 24
#define param_config_error_bits 1
#define param_config_error_reg  41
#define param_config_error_offset 16
#define param_cs_map_bits 2
#define param_cs_map_reg  44
#define param_cs_map_offset 8
#define param_cs_msk_0_bits 16
#define param_cs_msk_0_reg  40
#define param_cs_msk_0_offset 16
#define param_cs_msk_1_bits 16
#define param_cs_msk_1_reg  41
#define param_cs_msk_1_offset 0
#define param_cs_val_0_bits 16
#define param_cs_val_0_reg  39
#define param_cs_val_0_offset 16
#define param_cs_val_1_bits 16
#define param_cs_val_1_reg  40
#define param_cs_val_1_offset 0
#define param_dll_rst_adj_dly_bits 8
#define param_dll_rst_adj_dly_reg  85
#define param_dll_rst_adj_dly_offset 0
#define param_dll_rst_delay_bits 16
#define param_dll_rst_delay_reg  84
#define param_dll_rst_delay_offset 16
#define param_dram_class_bits 4
#define param_dram_class_reg  0
#define param_dram_class_offset 8
#define param_dram_clk_disable_bits 2
#define param_dram_clk_disable_reg  86
#define param_dram_clk_disable_offset 16
#define param_drive_dq_dqs_bits 1
#define param_drive_dq_dqs_reg  84
#define param_drive_dq_dqs_offset 0
#define param_eight_bank_mode_0_bits 1
#define param_eight_bank_mode_0_reg  37
#define param_eight_bank_mode_0_offset 16
#define param_eight_bank_mode_1_bits 1
#define param_eight_bank_mode_1_reg  37
#define param_eight_bank_mode_1_offset 24
#define param_enable_quick_srefresh_bits 1
#define param_enable_quick_srefresh_reg  19
#define param_enable_quick_srefresh_offset 24
#define param_fast_write_bits 1
#define param_fast_write_reg  45
#define param_fast_write_offset 0
#define param_inhibit_dram_cmd_bits 2
#define param_inhibit_dram_cmd_reg  44
#define param_inhibit_dram_cmd_offset 0
#define param_initaref_bits 4
#define param_initaref_reg  6
#define param_initaref_offset 0
#define param_int_ack_bits 14
#define param_int_ack_reg  46
#define param_int_ack_offset 16
#define param_int_mask_bits 15
#define param_int_mask_reg  47
#define param_int_mask_offset 0
#define param_int_status_bits 15
#define param_int_status_reg  46
#define param_int_status_offset 0
#define param_lowpower_auto_enable_bits 5
#define param_lowpower_auto_enable_reg  22
#define param_lowpower_auto_enable_offset 0
#define param_lowpower_control_bits 5
#define param_lowpower_control_reg  20
#define param_lowpower_control_offset 8
#define param_lowpower_external_cnt_bits 16
#define param_lowpower_external_cnt_reg  21
#define param_lowpower_external_cnt_offset 16
#define param_lowpower_internal_cnt_bits 16
#define param_lowpower_internal_cnt_reg  22
#define param_lowpower_internal_cnt_offset 8
#define param_lowpower_power_down_cnt_bits 16
#define param_lowpower_power_down_cnt_reg  20
#define param_lowpower_power_down_cnt_offset 16
#define param_lowpower_refresh_enable_bits 2
#define param_lowpower_refresh_enable_reg  22
#define param_lowpower_refresh_enable_offset 24
#define param_lowpower_self_refresh_cnt_bits 16
#define param_lowpower_self_refresh_cnt_reg  21
#define param_lowpower_self_refresh_cnt_offset 0
#define param_lpddr2_s4_bits 1
#define param_lpddr2_s4_reg  44
#define param_lpddr2_s4_offset 24
#define param_max_col_reg_bits 4
#define param_max_col_reg_reg  1
#define param_max_col_reg_offset 8
#define param_max_cs_reg_bits 2
#define param_max_cs_reg_reg  1
#define param_max_cs_reg_offset 16
#define param_max_row_reg_bits 4
#define param_max_row_reg_reg  1
#define param_max_row_reg_offset 0
#define param_mr0_data_0_bits 15
#define param_mr0_data_0_reg  27
#define param_mr0_data_0_offset 0
#define param_mr0_data_1_bits 15
#define param_mr0_data_1_reg  30
#define param_mr0_data_1_offset 8
#define param_mr1_data_0_bits 15
#define param_mr1_data_0_reg  27
#define param_mr1_data_0_offset 16
#define param_mr1_data_1_bits 15
#define param_mr1_data_1_reg  31
#define param_mr1_data_1_offset 0
#define param_mr2_data_0_bits 15
#define param_mr2_data_0_reg  28
#define param_mr2_data_0_offset 0
#define param_mr2_data_1_bits 15
#define param_mr2_data_1_reg  31
#define param_mr2_data_1_offset 16
#define param_mr3_data_0_bits 15
#define param_mr3_data_0_reg  29
#define param_mr3_data_0_offset 0
#define param_mr3_data_1_bits 15
#define param_mr3_data_1_reg  32
#define param_mr3_data_1_offset 16
#define param_mr8_data_0_bits 8
#define param_mr8_data_0_reg  29
#define param_mr8_data_0_offset 16
#define param_mr8_data_1_bits 8
#define param_mr8_data_1_reg  33
#define param_mr8_data_1_offset 0
#define param_mr16_data_0_bits 8
#define param_mr16_data_0_reg  29
#define param_mr16_data_0_offset 24
#define param_mr16_data_1_bits 8
#define param_mr16_data_1_reg  33
#define param_mr16_data_1_offset 8
#define param_mr17_data_0_bits 8
#define param_mr17_data_0_reg  30
#define param_mr17_data_0_offset 0
#define param_mr17_data_1_bits 8
#define param_mr17_data_1_reg  33
#define param_mr17_data_1_offset 16
#define param_mrsingle_data_0_bits 15
#define param_mrsingle_data_0_reg  28
#define param_mrsingle_data_0_offset 16
#define param_mrsingle_data_1_bits 15
#define param_mrsingle_data_1_reg  32
#define param_mrsingle_data_1_offset 0
#define param_mrw_status_bits 8
#define param_mrw_status_reg  25
#define param_mrw_status_offset 0
#define param_no_auto_mrr_init_bits 1
#define param_no_auto_mrr_init_reg  5
#define param_no_auto_mrr_init_offset 24
#define param_no_cmd_init_bits 1
#define param_no_cmd_init_reg  12
#define param_no_cmd_init_offset 16
#define param_no_zq_init_bits 1
#define param_no_zq_init_reg  37
#define param_no_zq_init_offset 0
#define param_ocd_adjust_pdn_cs_0_bits 5
#define param_ocd_adjust_pdn_cs_0_reg  56
#define param_ocd_adjust_pdn_cs_0_offset 24
#define param_ocd_adjust_pup_cs_0_bits 5
#define param_ocd_adjust_pup_cs_0_reg  57
#define param_ocd_adjust_pup_cs_0_offset 0
#define param_odt_alt_en_bits 1
#define param_odt_alt_en_reg  91
#define param_odt_alt_en_offset 8
#define param_odt_rd_map_cs0_bits 2
#define param_odt_rd_map_cs0_reg  52
#define param_odt_rd_map_cs0_offset 8
#define param_odt_rd_map_cs1_bits 2
#define param_odt_rd_map_cs1_reg  52
#define param_odt_rd_map_cs1_offset 24
#define param_odt_wr_map_cs0_bits 2
#define param_odt_wr_map_cs0_reg  52
#define param_odt_wr_map_cs0_offset 16
#define param_odt_wr_map_cs1_bits 2
#define param_odt_wr_map_cs1_reg  53
#define param_odt_wr_map_cs1_offset 0
#define param_out_of_range_addr_bits 33
#define param_out_of_range_addr_reg  48
#define param_out_of_range_addr_offset 0
#define param_out_of_range_length_bits 7
#define param_out_of_range_length_reg  49
#define param_out_of_range_length_offset 8
#define param_out_of_range_source_id_bits 8
#define param_out_of_range_source_id_reg  49
#define param_out_of_range_source_id_offset 24
#define param_out_of_range_type_bits 6
#define param_out_of_range_type_reg  49
#define param_out_of_range_type_offset 16
#define param_peripheral_mrr_data_bits 16
#define param_peripheral_mrr_data_reg  26
#define param_peripheral_mrr_data_offset 0
#define param_placement_en_bits 1
#define param_placement_en_reg  42
#define param_placement_en_offset 24
#define param_port_cmd_error_addr_bits 33
#define param_port_cmd_error_addr_reg  50
#define param_port_cmd_error_addr_offset 0
#define param_port_cmd_error_id_bits 8
#define param_port_cmd_error_id_reg  51
#define param_port_cmd_error_id_offset 8
#define param_port_cmd_error_type_bits 4
#define param_port_cmd_error_type_reg  51
#define param_port_cmd_error_type_offset 16
#define param_port_data_error_id_bits 8
#define param_port_data_error_id_reg  51
#define param_port_data_error_id_offset 24
#define param_port_data_error_type_bits 3
#define param_port_data_error_type_reg  52
#define param_port_data_error_type_offset 0
#define param_power_down_bits 1
#define param_power_down_reg  17
#define param_power_down_offset 0
#define param_priority_en_bits 1
#define param_priority_en_reg  43
#define param_priority_en_offset 0
#define param_pwrup_srefresh_exit_bits 1
#define param_pwrup_srefresh_exit_reg  19
#define param_pwrup_srefresh_exit_offset 8
#define param_q_fullness_bits 4
#define param_q_fullness_reg  45
#define param_q_fullness_offset 8
#define param_r2r_diffcs_dly_bits 3
#define param_r2r_diffcs_dly_reg  54
#define param_r2r_diffcs_dly_offset 8
#define param_r2r_samecs_dly_bits 3
#define param_r2r_samecs_dly_reg  55
#define param_r2r_samecs_dly_offset 8
#define param_r2w_diffcs_dly_bits 3
#define param_r2w_diffcs_dly_reg  54
#define param_r2w_diffcs_dly_offset 16
#define param_r2w_samecs_dly_bits 3
#define param_r2w_samecs_dly_reg  55
#define param_r2w_samecs_dly_offset 16
#define param_rdlat_adj_bits 5
#define param_rdlat_adj_reg  90
#define param_rdlat_adj_offset 5 bits
#define param_read_modereg_bits 17
#define param_read_modereg_reg  25
#define param_read_modereg_offset 8
#define param_reduc_bits 1
#define param_reduc_reg  44
#define param_reduc_offset 16
#define param_refresh_per_auto_tempchk_bits 8
#define param_refresh_per_auto_tempchk_reg  26
#define param_refresh_per_auto_tempchk_offset 24
#define param_refresh_per_zq_bits 8
#define param_refresh_per_zq_reg  35
#define param_refresh_per_zq_offset 24
#define param_reg_dimm_enable_bits 1
#define param_reg_dimm_enable_reg  14
#define param_reg_dimm_enable_offset 16
#define param_resync_dll_bits 1
#define param_resync_dll_reg  45
#define param_resync_dll_offset 16
#define param_resync_dll_per_aref_en_bits 1
#define param_resync_dll_per_aref_en_reg  45
#define param_resync_dll_per_aref_en_offset 24
#define param_rw_same_en_bits 1
#define param_rw_same_en_reg  43
#define param_rw_same_en_offset 8
#define param_srefresh_bits 1
#define param_srefresh_reg  19
#define param_srefresh_offset 0
#define param_srefresh_exit_no_refresh_bits 1
#define param_srefresh_exit_no_refresh_reg  19
#define param_srefresh_exit_no_refresh_offset 16
#define param_start_bits 1
#define param_start_reg  0
#define param_start_offset 0
#define param_swap_en_bits 1
#define param_swap_en_reg  43
#define param_swap_en_offset 16
#define param_swap_port_rw_same_en_bits 1
#define param_swap_port_rw_same_en_reg  43
#define param_swap_port_rw_same_en_offset 24
#define param_tbst_int_interval_bits 3
#define param_tbst_int_interval_reg  6
#define param_tbst_int_interval_offset 24
#define param_tccd_bits 5
#define param_tccd_reg  7
#define param_tccd_offset 0
#define param_tcke_bits 3
#define param_tcke_reg 9
#define param_tcke_offset 24
#define param_tckesr_bits 5
#define param_tckesr_reg  10
#define param_tckesr_offset 0
#define param_tcpd_bits 16
#define param_tcpd_reg  13
#define param_tcpd_offset 16
#define param_tdal_bits 5
#define param_tdal_reg  11
#define param_tdal_offset 24
#define param_tdfi_ctrl_delay_bits 4
#define param_tdfi_ctrl_delay_reg  90
#define param_tdfi_ctrl_delay_offset 16
#define param_tdfi_ctrlupd_max_bits 16
#define param_tdfi_ctrlupd_max_reg  87
#define param_tdfi_ctrlupd_max_offset 0
#define param_tdfi_ctrlupd_min_bits 4
#define param_tdfi_ctrlupd_min_reg  86
#define param_tdfi_ctrlupd_min_offset 24
#define param_tdfi_dram_clk_disable_bits 3
#define param_tdfi_dram_clk_disable_reg  90
#define param_tdfi_dram_clk_disable_offset 24
#define param_tdfi_dram_clk_enable_bits 4
#define param_tdfi_dram_clk_enable_reg  91
#define param_tdfi_dram_clk_enable_offset 0
#define param_tdfi_phy_rdlat_bits 5
#define param_tdfi_phy_rdlat_reg  85
#define param_tdfi_phy_rdlat_offset 24
#define param_tdfi_phy_wrlat_bits 4
#define param_tdfi_phy_wrlat_reg  85
#define param_tdfi_phy_wrlat_offset 8
#define param_tdfi_phy_wrlat_base_bits 4
#define param_tdfi_phy_wrlat_base_reg  85
#define param_tdfi_phy_wrlat_base_offset 16
#define param_tdfi_phyupd_resp_bits 16
#define param_tdfi_phyupd_resp_reg  89
#define param_tdfi_phyupd_resp_offset 16
#define param_tdfi_phyupd_type0_bits 16
#define param_tdfi_phyupd_type0_reg  87
#define param_tdfi_phyupd_type0_offset 16
#define param_tdfi_phyupd_type1_bits 16
#define param_tdfi_phyupd_type1_reg  88
#define param_tdfi_phyupd_type1_offset 0
#define param_tdfi_phyupd_type2_bits 16
#define param_tdfi_phyupd_type2_reg  88
#define param_tdfi_phyupd_type2_offset 16
#define param_tdfi_phyupd_type3_bits 16
#define param_tdfi_phyupd_type3_reg  89
#define param_tdfi_phyupd_type3_offset 0
#define param_tdfi_rddata_en_bits 5
#define param_tdfi_rddata_en_reg  86
#define param_tdfi_rddata_en_offset 0
#define param_tdfi_rddata_en_base_bits 5
#define param_tdfi_rddata_en_base_reg  86
#define param_tdfi_rddata_en_base_offset 8
#define param_tdll_bits 16
#define param_tdll_reg  12
#define param_tdll_offset 0
#define param_tdqsck_max_bits 2
#define param_tdqsck_max_reg  56
#define param_tdqsck_max_offset 8
#define param_tdqsck_min_bits 2
#define param_tdqsck_min_reg  56
#define param_tdqsck_min_offset 16
#define param_tfaw_bits 6
#define param_tfaw_reg  13
#define param_tfaw_offset 8
#define param_tinit_bits 24
#define param_tinit_reg  2
#define param_tinit_offset 0
#define param_tinit3_bits 24
#define param_tinit3_reg  3
#define param_tinit3_offset 0
#define param_tinit4_bits 24
#define param_tinit4_reg  4
#define param_tinit4_offset 0
#define param_tinit5_bits 24
#define param_tinit5_reg  5
#define param_tinit5_offset 0
#define param_tmod_bits 8
#define param_tmod_reg  9
#define param_tmod_offset 0
#define param_tmrd_bits 5
#define param_tmrd_reg  8
#define param_tmrd_offset 24
#define param_tmrr_bits 4
#define param_tmrr_reg  12
#define param_tmrr_offset 24
#define param_tpdex_bits 16
#define param_tpdex_reg  17
#define param_tpdex_offset 8
#define param_tras_lockout_bits 1
#define param_tras_lockout_reg  11
#define param_tras_lockout_offset 0
#define param_tras_max_bits 16
#define param_tras_max_reg  9
#define param_tras_max_offset 8
#define param_tras_min_bits 8
#define param_tras_min_reg  7
#define param_tras_min_offset 24
#define param_trc_bits 8
#define param_trc_reg  7
#define param_trc_offset 16
#define param_trcd_int_bits 8
#define param_trcd_int_reg  11
#define param_trcd_int_offset 8
#define param_tref_bits 16
#define param_tref_reg  16
#define param_tref_offset 0
#define param_tref_enable_bits 1
#define param_tref_enable_reg  15
#define param_tref_enable_offset 8
#define param_tref_interval_bits 14
#define param_tref_interval_reg  16
#define param_tref_interval_offset 16
#define param_trfc_bits 10
#define param_trfc_reg  15
#define param_trfc_offset 16
#define param_trp_bits 5
#define param_trp_reg  8
#define param_trp_offset 8
#define param_trp_ab_0_bits 5
#define param_trp_ab_0_reg  14
#define param_trp_ab_0_offset 0
#define param_trp_ab_1_bits 5
#define param_trp_ab_1_reg  14
#define param_trp_ab_1_offset 8
#define param_trrd_bits 8
#define param_trrd_reg  7
#define param_trrd_offset 8
#define param_trtp_bits 3
#define param_trtp_reg  8
#define param_trtp_offset 16
#define param_twr_int_bits 5
#define param_twr_int_reg  11
#define param_twr_int_offset 16
#define param_twtr_bits 4
#define param_twtr_reg  8
#define param_twtr_offset 0
#define param_txsnr_bits 16
#define param_txsnr_reg  18
#define param_txsnr_offset 16
#define param_txsr_bits 16
#define param_txsr_reg  18
#define param_txsr_offset 0
#define param_version_bits 16
#define param_version_reg  0
#define param_version_offset 16
#define param_w2r_diffcs_dly_bits 3
#define param_w2r_diffcs_dly_reg  54
#define param_w2r_diffcs_dly_offset 24
#define param_w2r_samecs_dly_bits 3
#define param_w2r_samecs_dly_reg  55
#define param_w2r_samecs_dly_offset 24
#define param_w2w_diffcs_dly_bits 3
#define param_w2w_diffcs_dly_reg  55
#define param_w2w_diffcs_dly_offset 0
#define param_w2w_samecs_dly_bits 3
#define param_w2w_samecs_dly_reg  56
#define param_w2w_samecs_dly_offset 0
#define param_write_modereg_bits 26
#define param_write_modereg_reg  24
#define param_write_modereg_offset 0
#define param_writeinterp_bits 1
#define param_writeinterp_reg  10
#define param_writeinterp_offset 8
#define param_wrlat_bits 4
#define param_wrlat_reg  6
#define param_wrlat_offset 16
#define param_wrlat_adj_bits 4
#define param_wrlat_adj_reg  90
#define param_wrlat_adj_offset 4 bits
#define param_wrtprct_enable_bits 2
#define param_wrtprct_enable_reg  77
#define param_wrtprct_enable_offset 16
#define param_wrtprct_error_address_bits 32
#define param_wrtprct_error_address_reg  76
#define param_wrtprct_error_address_offset 0
#define param_wrtprct_error_length_bits 7
#define param_wrtprct_error_length_reg  77
#define param_wrtprct_error_length_offset 8
#define param_wrtprct_lower_address_0_bits 32
#define param_wrtprct_lower_address_0_reg  70
#define param_wrtprct_lower_address_0_offset 0
#define param_wrtprct_lower_address_1_bits 32
#define param_wrtprct_lower_address_1_reg  74
#define param_wrtprct_lower_address_1_offset 0
#define param_wrtprct_upper_address_0_bits 32
#define param_wrtprct_upper_address_0_reg  68
#define param_wrtprct_upper_address_0_offset 0
#define param_wrtprct_upper_address_1_bits 32
#define param_wrtprct_upper_address_1_reg  72
#define param_wrtprct_upper_address_1_offset 0
#define param_zq_in_progress_bits 1
#define param_zq_in_progress_reg  36
#define param_zq_in_progress_offset 8
#define param_zq_on_sref_exit_bits 4
#define param_zq_on_sref_exit_reg  36
#define param_zq_on_sref_exit_offset 0
#define param_zq_req_bits 4
#define param_zq_req_reg  35
#define param_zq_req_offset 16
#define param_zqcl_bits 12
#define param_zqcl_reg  34
#define param_zqcl_offset 16
#define param_zqcs_bits 12
#define param_zqcs_reg  35
#define param_zqcs_offset 0
#define param_zqcs_rotate_bits 1
#define param_zqcs_rotate_reg  37
#define param_zqcs_rotate_offset 8
#define param_zqinit_bits 12
#define param_zqinit_reg  34
#define param_zqinit_offset 0
#define param_zqreset_bits 12
#define param_zqreset_reg  36
#define param_zqreset_offset 16

/* denali parameter access */
#define denali_set(_name, _val)                               \
({                                                            \
	uint32_t msk = (1 << param_##_name##_bits) - 1;       \
	uint32_t reg;                                         \
                                                              \
	reg = denali_readl(param_##_name##_reg);              \
	reg &= ~(msk << param_##_name##_offset);              \
	reg |= (_val) << param_##_name##_offset;              \
	denali_writel(reg, param_##_name##_reg);              \
})

#define denali_get(_name)                                     \
({                                                            \
	uint32_t msk = (1 << param_##_name##_bits) - 1;       \
	uint32_t reg;                                         \
                                                              \
	reg = denali_readl(param_##_name##_reg);              \
	reg >>= param_##_name##_offset;                       \
	reg & msk;                                            \
})

static uint32_t denali_readl(int regno)
{
	return readl(DMW96_SDC_BASE + regno * 4);
}

static void denali_writel(uint32_t val, int regno)
{
	writel(val, DMW96_SDC_BASE + regno * 4);
}

#if 0
static uint32_t denali_phy_readl(int regno)
{
	return denali_readl(128 + regno);
}
#endif

static void denali_phy_writel(uint32_t val, int regno)
{
	denali_writel(val, 128 + regno);
}

static void dram_conf(void)
{
	syscfg_dram_set_coherency_control();
	syscfg_dram_set_sus(DRAM_PIN_GROUP4, 1); /* DQS0 */
	syscfg_dram_set_sus(DRAM_PIN_GROUP5, 1); /* DQS1 */
	syscfg_dram_set_sus(DRAM_PIN_GROUP6, 1); /* DQS2 */
	syscfg_dram_set_sus(DRAM_PIN_GROUP7, 1); /* DQS3 */
}

static void denali_init(void)
{
	reset_release(BLOCK_DRAMCTL);
	clk_enable(BLOCK_DRAMCTL);
}

static void setbitsl(unsigned long addr, unsigned long bits)
{
	unsigned long val;

	val = readl(addr);
	val |= bits;
	writel(val, addr);
}

static void clrbitsl(unsigned long addr, unsigned long bits)
{
	unsigned long val;

	val = readl(addr);
	val &= ~bits;
	writel(val, addr);
}

#define DMW_BOOTROM_BASE        0x01200000
#define DMW_SYSCFG_BASE         0x05200000
#define DMW_SYSCFG_CHIP_REV     0x20
#define DMW_SYSCFG_DRAMCTL_CFG0 0x130
#define DMW_SYSCFG_DRAMCTL_CFG1 0x134
#define DMW_SYSCFG_DRAMCTL_CFG2 0x138
#define DMW_SYSCFG_DRAMCTL_CFG3 0x13C
#define DMW_SYSCFG_DRAMCTL_CFG4 0x140
#define DMW_CHIP_DMW96_REV_A    0
#define DMW_CHIP_DMW96_REV_B    1
static void syscfg_enable_dynamic_input_buffers(void)
{
	setbitsl(DMW_SYSCFG_BASE + DMW_SYSCFG_DRAMCTL_CFG4, 0x4000);
	clrbitsl(DMW_SYSCFG_BASE + DMW_SYSCFG_DRAMCTL_CFG0, 0x804020);
	clrbitsl(DMW_SYSCFG_BASE + DMW_SYSCFG_DRAMCTL_CFG1, 0x804020);
	clrbitsl(DMW_SYSCFG_BASE + DMW_SYSCFG_DRAMCTL_CFG2, 0x804020);
	clrbitsl(DMW_SYSCFG_BASE + DMW_SYSCFG_DRAMCTL_CFG3, 0x804020);
	clrbitsl(DMW_SYSCFG_BASE + DMW_SYSCFG_DRAMCTL_CFG4, 0x20);
}

static int dmw_detect_chip_rev_96(void)
{
	int ret;

	ret = readl(DMW_SYSCFG_BASE + DMW_SYSCFG_CHIP_REV) & 0xffff;
	if (ret > 0)
		return ret + 1;

	/*
	 * Revision A and B share the same code in SYSCFG CHIP_REV. :( Look
	 * into the bootrom code to distinguis both.
	 */
	switch (readb(DMW_BOOTROM_BASE + 68)) {
	case 0x30: ret = DMW_CHIP_DMW96_REV_A; break;
	case 0x90: ret = DMW_CHIP_DMW96_REV_B; break;
	default:   ret = -1;
	}

	return ret;
}

static int is_dll_bypass_reg(int regno)
{
	/* registers 14, 16, 18, 20, 22 contains DLL_BYPASS bit, which is set dynamically */
	return (regno >= 14) && (regno <= 22) && (regno % 2 == 0);
}

void lpddr2_init(void)
{
	uint32_t int_status;
	uint32_t dll_bypass_val;
	int i;
	int is_dmw96_rev_a = dmw_detect_chip_rev_96() == DMW_CHIP_DMW96_REV_A;

	/* already running ? */
	if (denali_get(start))
		return;

	dram_conf();
	denali_init();

	for (i = 0; i < 92; i++)
		denali_writel(denali_setup[i], i);

	/* only for dmw96 revA we have to enable DLL_CALIB_BYPASS */
	dll_bypass_val = is_dmw96_rev_a ? EN_DLL_CALIB_BYPASS : DIS_DLL_CALIB_BYPASS;

	for (i = 0; i < 39; i++) {
		uint32_t val = denali_phy_setup[i];
		if (is_dll_bypass_reg(i))
			val |= dll_bypass_val;

		denali_phy_writel(val, i);
	}

	if (!is_dmw96_rev_a)
		syscfg_enable_dynamic_input_buffers();

	/*
	 * port arbitration settings
	 */

	/* video encoder */
	denali_set(axi0_r_priority, 3);
	denali_set(axi0_w_priority, 3);
	denali_set(axi0_bdw,        100);
	denali_set(axi0_bdw_ovflow, 1);

	/* video decoder */
	denali_set(axi1_r_priority, 3);
	denali_set(axi1_w_priority, 3);
	denali_set(axi1_bdw,        100);
	denali_set(axi1_bdw_ovflow, 1);

	/* gpu */
	denali_set(axi2_r_priority, 6);
	denali_set(axi2_w_priority, 6);
	denali_set(axi2_bdw,        100);
	denali_set(axi2_bdw_ovflow, 1);

	/* cortex */
	denali_set(axi3_r_priority, 6);
	denali_set(axi3_w_priority, 6);
	denali_set(axi3_bdw,        100);
	denali_set(axi3_bdw_ovflow, 1);

	/* css */
	denali_set(axi4_r_priority, 6);
	denali_set(axi4_w_priority, 6);
	denali_set(axi4_bdw,        100);
	denali_set(axi4_bdw_ovflow, 1);

	/* peripherals */
	denali_set(axi5_r_priority, 5);
	denali_set(axi5_w_priority, 5);
	denali_set(axi5_bdw,        100);
	denali_set(axi5_bdw_ovflow, 1);

	/* lcdc: highest priority -> no underruns */
	denali_set(axi6_r_priority, 1);
	denali_set(axi6_w_priority, 1);
	denali_set(axi6_bdw,        100);
	denali_set(axi6_bdw_ovflow, 1);

	/* video, osdm, ciu: bandwidth limitation because of high peaks */
	denali_set(axi7_r_priority, 3);
	denali_set(axi7_w_priority, 3);
	denali_set(axi7_bdw,        20);
	denali_set(axi7_bdw_ovflow, 1);

	/*
	 * parameter overrides
	 */
	denali_set(lowpower_control, 0);
	denali_set(swap_port_rw_same_en, 0); /* Denali bug #132897 */

	delay(0xff);
	denali_set(start, 1);

	/* wait for init completion */
	do {
		int_status = denali_get(int_status) & 0x0010;
	} while (int_status != 0x0010);

	denali_set(int_ack, 0x10);
}
