/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

/*! \file sgwc_eps_bearer_context.hpp
  \brief
  \author Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/
#ifndef FILE_PGW_EPS_BEARER_CONTEXT_HPP_SEEN
#define FILE_PGW_EPS_BEARER_CONTEXT_HPP_SEEN

#include <map>
#include <mutex>
#include <memory>
#include <utility>
#include <vector>

#include "3gpp_24.008.h"
#include "3gpp_29.274.h"
#include "common_root_types.h"
#include "itti_msg_s5s8.hpp"
#include "pgwc_procedure.hpp"
#include "uint_generator.hpp"


namespace pgwc {


class pgw_eps_bearer {
public:
  pgw_eps_bearer() {
    clear();
  }

  void clear() {
    ebi.ebi = EPS_BEARER_IDENTITY_UNASSIGNED;
    tft = {};
    sgw_fteid_s5_s8_up = {};
    pgw_fteid_s5_s8_up = {};
    eps_bearer_qos = {};
    pdr_id_ul = {};
    pdr_id_dl = {};
    precedence = {};
    far_id_ul = {};
    far_id_dl = {};
    released = false;
  }

  void deallocate_ressources();
  void release_access_bearer();
  std::string toString() const;

  ebi_t                   ebi;//EPS Bearer Id   //An EPS bearer identity uniquely identifies an EPS bearer for one UE accessing via E-UTRAN.
  traffic_flow_template_t tft;                  // Traffic Flow Template
  //ip_address_t            sgw_ip_address_s5_s8_up;// S-GW Address in Use (user plane): The IP address of the S-GW currently used for sending user plane traffic.
  //teid_t                  sgw_teid_s5_s8_up;      // S-GW TEID for S5/S8 (user plane): S-GW Tunnel Endpoint Identifier for the S5/S8 interface for the user plane.
  // replaced by
  fteid_t                 sgw_fteid_s5_s8_up;

  //ip_address_t            pgw_address_in_use_up;  // P-GW IP address for user plane data received from PDN GW.
  //teid_t                  pgw_teid_s5_s8_up;      // P-GW Tunnel Endpoint Identifier for the GTP Based S5/S8 interface for user plane.
  fteid_t                 pgw_fteid_s5_s8_up;

  bearer_qos_t            eps_bearer_qos;           // EPS Bearer QoS: ARP, GBR, MBR, QCI.
  // NOT NEEDED        charging_id                  // Charging Id: Charging identifier, identifies charging records generated by S-GW and PDN GW.

  // PFCP
  // Packet Detection Rule ID
  pfcp::pdr_id_t                    pdr_id_ul;
  pfcp::pdr_id_t                    pdr_id_dl;
  pfcp::precedence_t                precedence;
  //pfcp::pdi                         pdi;
  // may use std::optional ? (fragment memory)
  std::pair<bool, pfcp::far_id_t>   far_id_ul;
  std::pair<bool, pfcp::far_id_t>   far_id_dl;
  bool                                    released; // finally seems necessary, TODO try to find heuristic ?
  //std::pair<bool, pfcp::urr_id_t>   urr_id;
  //std::pair<bool, pfcp::qer_id_t>   qer_id;
  //std::pair<bool, pfcp::activate_predefined_rules_t>  activate_predefined_rules;
};


class pgw_pdn_connection : public std::enable_shared_from_this<pgw_pdn_connection> {
public:
  pgw_pdn_connection() : eps_bearers() {
    clear();
  }

  void clear() {
    ipv4 = false;
    ipv6 = false;
    ipv4_address.s_addr = INADDR_ANY;
    ipv6_address = in6addr_any;
    pdn_type = {};
    sgw_fteid_s5_s8_cp = {};
    pgw_fteid_s5_s8_cp = {};
    default_bearer.ebi = EPS_BEARER_IDENTITY_UNASSIGNED;
    seid = 0;
    up_fseid = {};
    eps_bearers.clear();
    released = false;
  }

  pgw_pdn_connection(pgw_pdn_connection& b) = delete;

  bool get_eps_bearer(const pfcp::pdr_id_t& pdr_id, pgw_eps_bearer& b) {
    for (auto it : eps_bearers) {
      if (it.second.pdr_id_ul.rule_id == pdr_id.rule_id) {
        b = it.second;
        return true;
      }
      if (it.second.pdr_id_dl.rule_id == pdr_id.rule_id) {
        b = it.second;
        return true;
      }
    }
    return false;
  }
  bool get_eps_bearer(const pfcp::far_id_t& far_id, pgw_eps_bearer& b) {
    for (auto it : eps_bearers) {
      if ((it.second.far_id_ul.first) && (it.second.far_id_ul.second.far_id == far_id.far_id)) {
        b = it.second;
        return true;
      }
      if ((it.second.far_id_dl.first) && (it.second.far_id_dl.second.far_id == far_id.far_id)) {
        b = it.second;
        return true;
      }
    }
    return false;
  }
  bool get_eps_bearer(const ebi_t& ebi, pgw_eps_bearer& b) {
    for (auto it : eps_bearers) {
      if (it.second.ebi == ebi) {
        b = it.second;
        return true;
      }
    }
    return false;
  }
  //pgw_eps_bearer& get_eps_bearer(const ebi_t& ebi) {return eps_bearers[ebi.ebi];}
  void add_eps_bearer(pgw_eps_bearer& eps_bearer);
  pgw_eps_bearer& get_eps_bearer(const ebi_t& ebi);
  bool find_eps_bearer(const pfcp::pdr_id_t& pdr_id, pgw_eps_bearer& bearer);
  bool has_eps_bearer(const pfcp::pdr_id_t& pdr_id, ebi_t& ebi);
  void remove_eps_bearer(const ebi_t& ebi);
  void remove_eps_bearer(pgw_eps_bearer& bearer);
  void set(const paa_t& paa);

  // Called by GTPV2-C DELETE_SESSION_REQUEST
  // deallocate_ressources is for releasing LTE resources prior to the deletion of objects
  // since shared_ptr is actually heavy used for handling objects, deletion of object instances cannot be always guaranteed
  // when removing them from a collection, so that is why actually the deallocation of resources is not done in the destructor of objects.
  void deallocate_ressources(const std::string& apn);

  std::string toString() const;



  void generate_seid();
  void generate_pdr_id(pfcp::pdr_id_t& pdr_id);
  void release_pdr_id(const pfcp::pdr_id_t& pdr_id);
  void generate_far_id(pfcp::far_id_t& far_id);
  void release_far_id(const pfcp::far_id_t& far_id);
  void create_procedure(itti_s5s8_create_session_response& m);
  void insert_procedure(pgw_procedure* proc);


  bool ipv4;                                // IP Address(es): IPv4 address and/or IPv6 prefix
  bool ipv6;                                // IP Address(es): IPv4 address and/or IPv6 prefix
  struct in_addr  ipv4_address;             // IP Address(es): IPv4 address and/or IPv6 prefix
  struct in6_addr ipv6_address;             // IP Address(es): IPv4 address and/or IPv6 prefix
  pdn_type_t           pdn_type;            // IPv4, IPv6, IPv4v6 or Non-IP
  // S-GW Address in Use (control plane): The IP address of the S-GW currently used for sending control plane signalling.
  // S-GW TEID for S5/S8 (control plane): S-GW Tunnel Endpoint Identifier for the S5/S8 interface for the control plane. (For GTP-based S5/S8 only).
  fteid_t               sgw_fteid_s5_s8_cp;
  // S-GW GRE Key for downlink traffic (user plane): Serving GW assigned GRE Key for the S5/S8 interface for the user plane for downlink traffic.
  //                                                 (For PMIP-based S5/S8 only).
  // P-GW IP address for S5/S8 (control plane): P-GW IP address for the S5/S8 for the control plane signalling.
  // P-GW TEID for S5/S8 (control plane): P-GW Tunnel Endpoint Identifier for the S5/S8 control plane interface. (For GTP-based S5/S8 only).
  fteid_t               pgw_fteid_s5_s8_cp;
  // P-GW Address in Use (user plane): The IP address of the P-GW currently used for sending user plane traffic. (For PMIP-based S5/S8 only).
  // P-GW GRE Key for uplink traffic (user plane): PDN GW assigned GRE Key for the S5/S8 interface for the user plane for uplink traffic.
  //                                               (For PMIP-based S5/S8 only).
  // MS Info Change Reporting support indication: The MME and/or SGSN serving the UE support(s) procedures for reporting User Location Information
  //                                              and/or User CSG Information.
  // MS Info Change Reporting Action: Denotes whether the MME and/or the SGSN is/are requested to send changes in User Location Information change.
  // CSG Information Reporting Action: Denotes whether the MME and/or the SGSN is/are requested to send changes in User CSG Information change.
  //                                   This field denotes separately whether the MME/SGSN are requested to send changes in User CSG Information
  //                                   for
  //                                   (a) CSG cells,
  //                                   (b) hybrid cells in which the subscriber is a CSG member, and
  //                                   (c) hybrid cells in which the subscriber is not a CSG member, or any combination of the above.
  // Presence Reporting Area Action: Denotes whether the MME and/or the SGSN is/arerequested to send changes of UE presence in Presence
  //                                 Reporting Area.This field denotes separately the Presence Reporting Area identifier and the list
  //                                 of Presence Reporting Area elements.
  // BCM: The negotiated Bearer Control Mode for GERAN/UTRAN.
  ebi_t           default_bearer;        //Default Bearer: Identifies the default bearer within the PDN connection by its EPS Bearer Id.
  //                                       The default bearer is the one which is established first within the PDN connection. (For GTP based
  //                                       S5/S8 or for PMIP based S5/S8 if multiple PDN connections to the same APN are supported).
  // EPS PDN Charging Characteristics: The charging characteristics of this PDN connection e.g. normal, prepaid, flat-rate and/or hot billing.
  // Serving PLMN-Rate-Control: The Serving PLMN-Rate-Control limits the maximum number of uplink/downlink messages per a specific time unit (e.g.
  //                            minute, hour, day, week) for a PDN connection.
  // 3GPP PS Data Off Status: Current 3GPP PS Data Off status of the UE.
  // eps bearers
  std::map<uint8_t,pgw_eps_bearer> eps_bearers;
  bool                             released; //(release access bearers request)

  //----------------------------------------------------------------------------
  // PFCP related members
  //----------------------------------------------------------------------------
  // PFCP Session
  uint64_t seid;
  pfcp::fseid_t  up_fseid;
  //
  util::uint_generator<uint16_t>   pdr_id_generator;
  util::uint_generator<uint32_t>   far_id_generator;
};


class apn_context {

public:
  apn_context() : m_context(), in_use(false), pdn_connections() {
    apn_ambr = {0};
  }

  apn_context(apn_context& b) = delete;

  void insert_pdn_connection(std::shared_ptr<pgw_pdn_connection>& sp);
  bool find_pdn_connection(const teid_t xgw_s5s8c_teid, const bool is_local_teid, std::shared_ptr<pgw_pdn_connection>& pdn);
  bool find_pdn_connection(const pfcp::pdr_id_t& pdr_id, std::shared_ptr<pgw_pdn_connection>& pdn, ebi_t& ebi);
  void delete_pdn_connection(std::shared_ptr<pgw_pdn_connection>& pdn_connection);
  int get_num_pdn_connections() const {return pdn_connections.size();};
  // deallocate_ressources is for releasing LTE resources prior to the deletion of objects
  // since shared_ptr is actually heavy used for handling objects, deletion of object instances cannot be always guaranteed
  // when removing them from a collection, so that is why actually the deallocation of resources is not done in the destructor of objects.
  void deallocate_ressources();

  std::string toString() const;

  bool                  in_use;
  std::string           apn_in_use;               // The APN currently used, as received from the SGW.
  ambr_t          apn_ambr;                 // APN AMBR: The maximum aggregated uplink and downlink MBR values to be shared across all Non-GBR bearers, which are established for this APN.
  // APN Rate Control: The APN-Rate-Control limits the maximum number of uplink/downlink packets and the maximum number of
  //                   additional exception report packets per a specific time unit (e.g. minute, hour, day, week) for this APN. It includes an
  //                   indication as to whether or not Exception reports may still be sent when the limit has been met.
  // key is local s5s8 teid
  //map<teid_t, shared_ptr<pgw_pdn_connection>> pdn_connections;
  std::vector<std::shared_ptr<pgw_pdn_connection>> pdn_connections; // was list
  mutable std::recursive_mutex                     m_context;
};

class pgw_context;

typedef std::pair<std::shared_ptr<apn_context>, std::shared_ptr<pgw_pdn_connection>> pdn_duo_t;

class pgw_context : public std::enable_shared_from_this<pgw_context> {

public:
  pgw_context() : m_context(), imsi(), imsi_unauthenticated_indicator(false), apns(), pending_procedures(), msisdn() {}

  pgw_context(pgw_context& b) = delete;

  //void create_procedure(itti_s5s8_create_session_request& csreq);
  void insert_procedure(std::shared_ptr<pgw_procedure>& sproc);
  bool find_procedure(const uint64_t& trxn_id, std::shared_ptr<pgw_procedure>& proc);
  void remove_procedure(pgw_procedure* proc);

#define IS_FIND_PDN_WITH_LOCAL_TEID true
#define IS_FIND_PDN_WITH_PEER_TEID  false

  bool find_pdn_connection(const std::string& apn, const teid_t xgw_s5s8c_teid, const bool is_local_teid, pdn_duo_t& pdn_connection);
  bool find_pdn_connection(const teid_t xgw_s5s8c_teid, const bool is_local_teid, pdn_duo_t& pdn_connection);
  bool find_pdn_connection(const pfcp::pdr_id_t& pdr_id, std::shared_ptr<pgw_pdn_connection>& pdn, ebi_t& ebi);
  void insert_apn(std::shared_ptr<apn_context>& sa);
  bool find_apn_context(const std::string& apn, std::shared_ptr<apn_context>& apn_context);
  int get_num_apn_contexts() {return apns.size();};

  void delete_apn_context(std::shared_ptr<apn_context>& sa);
  void delete_pdn_connection(std::shared_ptr<apn_context>& sa , std::shared_ptr<pgw_pdn_connection>& sp);

  void handle_itti_msg (std::shared_ptr<itti_s5s8_create_session_request> s5_trigger);
  void handle_itti_msg (std::shared_ptr<itti_s5s8_delete_session_request> s5_trigger);
  void handle_itti_msg (std::shared_ptr<itti_s5s8_modify_bearer_request> s5_trigger);
  void handle_itti_msg (std::shared_ptr<itti_s5s8_release_access_bearers_request> s5_trigger);
  void handle_itti_msg (itti_s5s8_downlink_data_notification_acknowledge& );
  void handle_itti_msg (itti_sxab_session_establishment_response& );
  void handle_itti_msg (itti_sxab_session_modification_response& );
  void handle_itti_msg (itti_sxab_session_deletion_response& );
  void handle_itti_msg (std::shared_ptr<itti_sxab_session_report_request>&);

  std::string  toString() const;


  imsi_t         imsi;                           // IMSI (International Mobile Subscriber Identity) is the subscriber permanent identity.
  bool                 imsi_unauthenticated_indicator; // This is an IMSI indicator to show the IMSI is unauthenticated.
  // TO BE CHECKED me_identity_t    me_identity;       // Mobile Equipment Identity (e.g. IMEI/IMEISV).
  msisdn_t               msisdn;                       // The basic MSISDN of the UE. The presence is dictated by its storage in the HSS.
  //  selected_cn_operator_id                          // Selected core network operator identity (to support networksharing as defined in TS 23.251
  // NOT IMPLEMENTED RAT type  Current RAT (implicit)
  // NOT IMPLEMENTED Trace reference                        // Identifies a record or a collection of records for a particular trace.
  // NOT IMPLEMENTED Trace type                             // Indicates the type of trace
  // NOT IMPLEMENTED Trigger id                             // Identifies the entity that initiated the trace
  // NOT IMPLEMENTED OMC identity                           // Identifies the OMC that shall receive the trace record(s).

  std::vector<std::shared_ptr<apn_context>> apns; // was list

  //--------------------------------------------
  // internals
  std::vector<std::shared_ptr<pgw_procedure>> pending_procedures;

  // Big recursive lock
  mutable std::recursive_mutex                m_context;
};
}

#endif
