/**
 *
 * \section COPYRIGHT
 *
 * Copyright 2013-2021 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#ifndef SRSRAN_SCHED_NR_INTERFACE_H
#define SRSRAN_SCHED_NR_INTERFACE_H

#include "srsran/adt/bounded_bitset.h"
#include "srsran/adt/bounded_vector.h"
#include "srsran/adt/optional.h"
#include "srsran/adt/span.h"
#include "srsran/common/common_nr.h"
#include "srsran/common/phy_cfg_nr.h"
#include "srsran/common/slot_point.h"
#include "srsran/interfaces/gnb_interfaces.h"
#include "srsran/phy/phch/dci_nr.h"

namespace srsenb {

const static size_t   SCHED_NR_MAX_CARRIERS     = 4;
const static uint16_t SCHED_NR_INVALID_RNTI     = 0;
const static size_t   SCHED_NR_MAX_NOF_RBGS     = 18;
const static size_t   SCHED_NR_MAX_TB           = 1;
const static size_t   SCHED_NR_MAX_HARQ         = SRSRAN_DEFAULT_HARQ_PROC_DL_NR;
const static size_t   SCHED_NR_MAX_BWP_PER_CELL = 2;
const static size_t   SCHED_NR_MAX_LCID         = srsran::MAX_NR_NOF_BEARERS;
const static size_t   SCHED_NR_MAX_LC_GROUP     = 7;

struct sched_nr_ue_cc_cfg_t {
  bool     active = false;
  uint32_t cc     = 0;
};

struct sched_nr_ue_cfg_t {
  uint32_t                                                            maxharq_tx = 4;
  srsran::bounded_vector<sched_nr_ue_cc_cfg_t, SCHED_NR_MAX_CARRIERS> carriers;
  std::array<mac_lc_ch_cfg_t, SCHED_NR_MAX_LCID>                      ue_bearers = {};
  srsran::phy_cfg_nr_t                                                phy_cfg    = {};
};

class sched_nr_interface
{
public:
  static const size_t MAX_GRANTS  = mac_interface_phy_nr::MAX_GRANTS;
  static const size_t MAX_SIBS    = 2;
  static const size_t MAX_SUBPDUS = 8;

  ///// Configuration /////

  struct bwp_cfg_t {
    uint32_t               start_rb        = 0;
    uint32_t               rb_width        = 100;
    srsran_pdcch_cfg_nr_t  pdcch           = {};
    srsran_sch_hl_cfg_nr_t pdsch           = {};
    srsran_sch_hl_cfg_nr_t pusch           = {};
    uint32_t               rar_window_size = 10; // See TS 38.331, ra-ResponseWindow: {1, 2, 4, 8, 10, 20, 40, 80}
    uint32_t               numerology_idx  = 0;
  };

  struct cell_cfg_sib_t {
    uint32_t len;
    uint32_t period_rf;
    uint32_t si_window_slots;
  };

  struct cell_cfg_t {
    srsran_carrier_nr_t                                          carrier = {};
    srsran_duplex_config_nr_t                                    duplex  = {};
    srsran::phy_cfg_nr_t::ssb_cfg_t                              ssb     = {};
    srsran::bounded_vector<bwp_cfg_t, SCHED_NR_MAX_BWP_PER_CELL> bwps{1}; // idx0 for BWP-common
    srsran_mib_nr_t                                              mib;
    srsran::bounded_vector<cell_cfg_sib_t, MAX_SIBS>             sibs;
  };

  struct sched_args_t {
    bool        pdsch_enabled      = true;
    bool        pusch_enabled      = true;
    bool        auto_refill_buffer = false;
    int         fixed_dl_mcs       = 28;
    int         fixed_ul_mcs       = 28;
    std::string logger_name        = "MAC-NR";
  };

  using ue_cc_cfg_t = sched_nr_ue_cc_cfg_t;
  using ue_cfg_t    = sched_nr_ue_cfg_t;

  ////// RA signalling //////

  struct rar_info_t {
    uint32_t   preamble_idx; // is this the RAPID?
    uint32_t   ofdm_symbol_idx;
    uint32_t   freq_idx;
    uint32_t   ta_cmd;
    uint16_t   temp_crnti;
    uint32_t   msg3_size = 7;
    slot_point prach_slot;
  };
  struct msg3_grant_t {
    rar_info_t         data;
    srsran_dci_ul_nr_t msg3_dci = {};
  };
  struct rar_t {
    srsran::bounded_vector<msg3_grant_t, MAX_GRANTS> grants;
  };

  ////// DL data signalling //////

  struct dl_pdu_t {
    srsran::bounded_vector<uint32_t, MAX_SUBPDUS> subpdus;
  };

  ///// Sched Result /////

  using dl_sched_t = mac_interface_phy_nr::dl_sched_t;
  using ul_res_t   = mac_interface_phy_nr::ul_sched_t;

  using sched_sib_list_t    = srsran::bounded_vector<uint32_t, MAX_GRANTS>; /// list of SI indexes
  using sched_rar_list_t    = srsran::bounded_vector<rar_t, MAX_GRANTS>;
  using sched_dl_pdu_list_t = srsran::bounded_vector<dl_pdu_t, MAX_GRANTS>;
  struct dl_res_t {
    dl_sched_t          phy;
    sched_dl_pdu_list_t data;
    sched_rar_list_t    rar;
    sched_sib_list_t    sib_idxs;
  };

  virtual ~sched_nr_interface() = default;
  virtual int  config(const sched_args_t& sched_cfg, srsran::const_span<sched_nr_interface::cell_cfg_t> ue_cfg) = 0;
  virtual void ue_cfg(uint16_t rnti, const ue_cfg_t& ue_cfg)                                                    = 0;
  virtual void ue_rem(uint16_t rnti)                                                                            = 0;

  virtual void      slot_indication(slot_point slot_tx)           = 0;
  virtual dl_res_t* get_dl_sched(slot_point slot_rx, uint32_t cc) = 0;
  virtual ul_res_t* get_ul_sched(slot_point slot_rx, uint32_t cc) = 0;

  virtual void dl_ack_info(uint16_t rnti, uint32_t cc, uint32_t pid, uint32_t tb_idx, bool ack) = 0;
  virtual void ul_crc_info(uint16_t rnti, uint32_t cc, uint32_t pid, bool crc)                  = 0;
  virtual void ul_sr_info(uint16_t rnti)                                                        = 0;
  virtual void ul_bsr(uint16_t rnti, uint32_t lcg_id, uint32_t bsr)                             = 0;
};

} // namespace srsenb

#endif // SRSRAN_SCHED_NR_INTERFACE_H
