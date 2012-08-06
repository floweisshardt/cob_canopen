#include "canopenmsg.h"
#include <iostream>
#include <string>
#include <libpcan.h>
#include <set>
#include <map>

namespace canopen {

  HANDLE h;
  EDSDict eds;
  PDODict pdo;
  std::set<std::string> pendingReplies;
  // std::multiset<std::string> pendingSDOReplies;  todo: not needed anymore (?)
  // todo: make this map threadsafe
  
  // std::map<std::string, TPCANMsg* > pendingSDOReplies;  // todo: change to Message*
  std::map<std::string, Message* > pendingSDOReplies;

  // std::thread listener_thread;
  // listener_thread(listener_func);
  // todo: message queue

  /*std::string Message::createMsgHash(TPCANRdMsg m) { // COBID_index_subindex (decimal)
    std::string ss = std::to_string(nodeID_) + "_" +
      std::to_string(m.Msg.DATA[1] + (m.Msg.DATA[2]<<8)) + "_" +
      std::to_string(m.Msg.DATA[3]);
    return ss;
    }*/

  std::string Message::createMsgHash(TPCANMsg m) { // COBID_index_subindex (decimal)
    std::string ss = std::to_string(nodeID_) + "_" +
      std::to_string(m.DATA[1] + (m.DATA[2]<<8)) + "_" +
      std::to_string(m.DATA[3]);
    return ss;
  }

  std::string Message::createMsgHash() { // COBID_index_subindex (decimal)
    uint16_t index = eds.getIndex(alias_);
    uint8_t subindex = eds.getSubindex(alias_);
    std::string ss = std::to_string(nodeID_) + "_" +
      std::to_string(index) + "_" +
      std::to_string(subindex);
    return ss;
  }

  Message::Message(TPCANRdMsg m) {  // process messages coming in from the devices
    // printf("hey\n");
    // printf("%02x\n", m.Msg.ID);
    if (pdo.cobIDexists(m.Msg.ID)) {  // PDO
      /*  std::cout << "PDO recognized!" << std::endl;
      
      alias_ = pdo.getAlias(m.Msg.ID);
      nodeID_ = pdo.getNodeID(alias_);
      
      std::vector<std::string> components = pdo.getComponents(alias_);
      std::cout << "COMPONENTS:" << std::endl;
      for (int i=0; i<components.size(); i++)
	std::cout << components[i] << "  ";
      std::cout << std::endl;

      std::vector<uint32_t> lengths;

      for (auto comp:components) lengths.push_back(eds.getLen(comp));
      int pos = 0;
      for (int i=0; i<components.size(); i++) {
	values_.push_back(0);
	for (int j=0; j<lengths[i]; j++) {
	  values_[i] += (static_cast<uint32_t>(m.Msg.DATA[pos]) << (j*8));
	  pos++;
	}
      }

      std::cout << "LENGTHS:" << std::endl;
      for (int i=0; i<lengths.size(); i++)
	std::cout << lengths[i] << "  ";
      std::cout << std::endl;

     
      std::cout << "VALUES:" << std::endl;
      for (int i=0; i<values_.size(); i++) std::cout << values_[i] << "  ";
      std::cout << std::endl;
      */
      
    } else if (m.Msg.ID >= 0x580 && m.Msg.ID <= 0x5ff) { // SDO replies
      // std::this_thread::sleep_for(std::chrono::milliseconds(500));
      std::cout << "------SDO reply recognized!--------" << std::endl;
      printf("%02x %d %d\n", m.Msg.ID, m.Msg.MSGTYPE, m.Msg.LEN);
      for (int i=0; i<m.Msg.LEN; i++) printf("%02x ", m.Msg.DATA[i]);
      printf("\n");

      uint16_t index = m.Msg.DATA[1] + (m.Msg.DATA[2]<<8);
      uint8_t subindex = m.Msg.DATA[3];

      nodeID_ = m.Msg.ID - 0x580;
      alias_ =  eds.getAlias(index, subindex);
      values_.push_back(m.Msg.DATA[4] + (m.Msg.DATA[5]<<8) + (m.Msg.DATA[6]<<8) + (m.Msg.DATA[7]<<8));

      std::cout << "Pending replies:" << std::endl;
      debug_show_pendingSDOReplies();

      std::string ss = createMsgHash();
      std::cout << "message hash: " << ss << std::endl;
      /* TPCANMsg *m1 = new TPCANMsg;
      m1->ID = m.Msg.ID;
      m1->MSGTYPE = m.Msg.MSGTYPE;
      m1->LEN = m.Msg.LEN;
      for (int i=0; i<m.Msg.LEN; i++) m1->DATA[i] = m.Msg.DATA[i]; */
      // pendingSDOReplies[ss] = m1;
      pendingSDOReplies[ss] = this;

      // pendingSDOReplies[ss] = &m.Msg; // put received reply in hashtable
      // pendingReplies.erase(ss);
    }

    // std::cout << dd << std::endl;
  }

  Message::Message(uint8_t nodeID, std::string alias, uint32_t value):nodeID_(nodeID), alias_(alias) {
    values_.push_back(value);
  }

  bool Message::checkForConstant(std::string constName) {
    uint32_t value = values_[0];
    uint32_t constValue = eds.getConst(alias_, constName);
    uint32_t mask = eds.getMask(alias_, constName);
    std::cout << "constValue: " << constValue << std::endl; // todo: remove output
    std::cout << "constMask: " << mask << std::endl;
    std::cout << "value: " << value << std::endl;
    std::cout << "value & constMask: " << (value & mask) << std::endl;
    return (value & mask) == constValue;
  }

  Message::Message(std::string alias, std::vector<uint32_t> values):alias_(alias), values_(values) {}

  void Message::writeCAN(bool writeMode) {
    TPCANMsg msg;
    for (int i=0; i<8; i++) msg.DATA[i]=0;

    if (values_.size() > 1) {  // PDO
      std::vector<std::string> components = pdo.getComponents(alias_);
      std::vector<uint32_t> lengths;
      for (auto comp:components) lengths.push_back(eds.getLen(comp));
      int pos = 0;
      for (int i=0; i<components.size(); i++) {
	for (int j=0; j<lengths[i]; j++) {
	  msg.DATA[pos] = static_cast<uint8_t>(values_[i] >> (j*8) & 0xFF);
	  pos++;
	}
      }
      msg.ID = pdo.getCobID(alias_);
      msg.MSGTYPE = 0x00;
      msg.LEN = 8;
      CAN_Write(h, &msg);
    
      printf("%02x %d %d\n", msg.ID, msg.MSGTYPE, msg.LEN);
      for (int i=0; i<8; i++) printf("%02x ", msg.DATA[i]);
      printf("\n");
    } else if (alias_ == "NMT") {
      std::cout << "hi, NMT" << std::endl;
      msg.ID = 0;
      msg.MSGTYPE = 0x00;  // standard message
      msg.LEN = 2;
      msg.DATA[0] = values_[0];
      msg.DATA[1] = nodeID_;
      CAN_Write(h, &msg);
      
    } else if (alias_ == "Sync") {
      std::cout << "hi, sync" << std::endl;
      msg.ID = 0x80;
      msg.MSGTYPE = 0x00;
      msg.LEN = 0;
      CAN_Write(h, &msg);
    
    } else { // SDO
      std::cout << "hi, SDO" << std::endl;
      msg.ID = 0x600 + nodeID_;
      msg.MSGTYPE = 0x00;
      uint8_t len = eds.getLen(alias_);
      if (writeMode == true) {
	msg.LEN = 4 + len; // 0x2F(or 0x2b or 0x23) / index / subindex / actual data
	if (len==1) {
	  msg.DATA[0] = 0x2F;
	} else if (len==2) {
	  msg.DATA[0] = 0x2B;
	} else { // len==4
	  msg.DATA[0] = 0x23;
	}
      } else { // writeMode==false
	msg.LEN = 4; // only 0x40 / index / subindex
	msg.DATA[0] = 0x40;
      }
      uint16_t index = eds.getIndex(alias_);
      msg.DATA[1] = static_cast<uint8_t>(index & 0xFF);
      msg.DATA[2] = static_cast<uint8_t>((index >> 8) & 0xFF);
      msg.DATA[3] = eds.getSubindex(alias_);
      if (writeMode == true) {
	uint32_t v = values_[0];
	for (int i=0; i<len; i++) msg.DATA[4+i] = static_cast<uint8_t>( (v >> (8*i)) & 0xFF );
      }

      printf("%02x %d %d\n", msg.ID, msg.MSGTYPE, msg.LEN);
      for (int i=0; i<8; i++) printf("%02x ", msg.DATA[i]);
      printf("\n");

      // put on multiset
      std::string ss = createMsgHash(msg);
      pendingSDOReplies.insert(std::make_pair(ss, nullptr));
      std::cout << "Message hash: " << ss << std::endl;

      CAN_Write(h, &msg);
    }  // end SDO

  }

  void debug_show_pendingSDOReplies() {
    std::cout << "DEBUG. Pending_queue_size = " << pendingSDOReplies.size() << std::endl;
    
    for (auto it : pendingSDOReplies) {
      std::cout << it.first;
      if (it.second == nullptr) std::cout << "(nullptr)";
      std::cout << ", ";
    }
    std::cout << std::endl;
  }

  void Message::debugPrint() {
    std::cout << "MESSAGE debugPrint:" << std::endl;
    std::cout << "nodeID: " << static_cast<int>(nodeID_) << ", alias: " << alias_ << ", value: " << values_[0] << std::endl;
  }

  Message* Message::readCAN(bool blocking) { // todo: different blocking modes
    
    TPCANRdMsg m;
    if ((errno = LINUX_CAN_Read(canopen::h, &m))) {
      perror("receivetest: LINUX_CAN_Read()");
      // return errno;
    }
    // std::cout << "MESSAGE received!" << std::endl;
    // msg = new Message(m);
    Message* msg = new Message(m);
    return msg;
  }

  // TPCANMsg
  Message* Message::waitForSDOAnswer() { // uint16_t deviceID, std::string alias) {
    std::cout << "now in waitForSDOAnswer" << std::endl;
    std::string ss = createMsgHash();
    while (pendingSDOReplies[ss] == nullptr) {
      // debug_show_pendingSDOReplies();
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // TPCANMsg* m = pendingSDOReplies[ss];
    Message* m = pendingSDOReplies[ss];
    std::cout << "waitForSDOAnswer ended!!!!" << std::endl;
    std::cout << "Here is the reply:" << std::endl;
    m->debugPrint();

    /*printf("%02x %d %d\n", m->ID, m->MSGTYPE, m->LEN);
    for (int i=0; i<8; i++) printf("%02x ", m->DATA[i]);
    printf("\n");*/

    pendingSDOReplies.erase(ss);
    // delete m;
    debug_show_pendingSDOReplies();
    return m;
  } 

  bool openConnection(std::string devName) {
    h = LINUX_CAN_Open(devName.c_str(), O_RDWR);
    if (!h) {
      std::cout << "Cannot open CAN device" << std::endl;
      return false;
    }
    errno = CAN_Init(h, CAN_BAUD_500K, CAN_INIT_TYPE_ST);
    return true;
  }

  void listener_func() {
    TPCANRdMsg m;
    Message* msg;
    
    while (true) {
      // std::cout << "hi, listener" << std::endl;
      msg = Message::readCAN(true);
      // std::cout << "message received listener" << std::endl;
    }
  }


  void initListener() {
    std::thread listener_thread(listener_func);
    listener_thread.detach();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }


  void ipMode(uint16_t deviceID) {
    Message(deviceID, "modes_of_operation",
	    eds.getConst("modes_of_operation", "interpolated_position_mode")).writeCAN();
  }
  void sendSync() { Message(0, "Sync").writeCAN(); }
  void sendPos(std::string alias, uint32_t pos) {
    std::vector<uint32_t> v;
    v.push_back(eds.getConst("controlword", "start_homing|enable_ip_mode"));
    v.push_back(0);
    v.push_back(pos);
    Message(alias, v).writeCAN();
  }
  void closeConnection() { CAN_Close(h); }
  void stopRemoteNode() { // only for testing
    std::cout << "stop remote node" << std::endl;
    Message(0, "NMT", eds.getConst("NMT", "stop_remote_node")).writeCAN();
  }

  bool initDevice(uint16_t deviceID) {
    Message m(deviceID, "controlword", eds.getConst("controlword", "reset_fault"));
    m.writeCAN();
    m.waitForSDOAnswer();
    std::this_thread::sleep_for(std::chrono::milliseconds(20)); 

    // Message(0, "NMT", eds.getConst("NMT", "reset_application")).writeCAN();
    //  std::this_thread::sleep_for(std::chrono::milliseconds(20)); 

    Message(0, "NMT", eds.getConst("NMT", "stop_remote_node")).writeCAN();
    std::this_thread::sleep_for(std::chrono::milliseconds(20)); 
    
    Message(0, "NMT", eds.getConst("NMT", "start_remote_node")).writeCAN();
    std::this_thread::sleep_for(std::chrono::milliseconds(20)); 

    m = Message(deviceID, "controlword", eds.getConst("controlword", "sm_shutdown"));
    m.writeCAN();
    m.waitForSDOAnswer();

    m = Message(deviceID, "controlword", eds.getConst("controlword", "sm_switch_on"));
    m.writeCAN();
    m.waitForSDOAnswer();

    m = Message(deviceID, "controlword", eds.getConst("controlword", "sm_enable_operation"));
    m.writeCAN();
    m.waitForSDOAnswer();

    Message* statusMsg = canopen::statusWord(deviceID);

    if (statusMsg->checkForConstant("operation_enable")) {  // operation_enable
      return true;
    } else {
      return false;
    }
  }
  void homing(uint16_t deviceID, uint32_t sleep_ms) {
    Message(deviceID, "modes_of_operation", eds.getConst("modes_of_operation", "homing_mode")).writeCAN();
    Message(deviceID, "controlword", eds.getConst("controlword", "start_homing|enable_ip_mode")).writeCAN();
    std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms)); 
  }
  Message* statusWord(uint16_t deviceID) { 
    Message m(deviceID, "statusword");
    m.writeCAN(false);
    std::cout << "statusword waiting for answer...." << std::endl;
    // TPCANMsg msg = 
    return m.waitForSDOAnswer();
    // std::cout << "statusword received answer." << std::endl;
  }  // todo check
  
  void modesOfOperationDisplay(uint16_t deviceID) { Message(deviceID, "statusword").writeCAN(false); }

  EDSDict::EDSDict() {
    d_.insert(EDSClass("notused16", "FFFF_FF", 0xFFFF, 0xFF, 2, "rw"));   // dummy entry for unused 16 bits in a PDO message
    d_.insert(EDSClass("notused32", "FFFF_FE", 0xFFFF, 0xFE, 4, "rw"));   // dummy entry for unused 32 bit in a PDO message
    d_.insert(EDSClass("notused64", "FFFF_FD", 0xFFFF, 0xFD, 8, "rw"));   // dummy entry for unused 64 bit in a PDO message

    d_.insert(EDSClass("interpolation_data_record:ip_data_position", "60C1_1", 
		       0x60C1, 0x1, 4, "rw")); // interpolation_data_record: ip_data_position
    d_.insert(EDSClass("sync_timeout_factor", "200e_0", 
		       0x200e, 0x0, 1, "rw"));   // sync_timeout_factor
    // NMT
    //bmtype bm0;
    //bm0.insert(bmtype::value_type("stop_remote_node",0x02));
    //bm0.insert(bmtype::value_type("start_remote_node",0x01));
    
    // d_.insert(EDSClass("NMT","0", 0x0, 0x0, 1, "wo", bm0));
    EDSClass NMT("NMT","0", 0x0, 0x0, 1, "wo");
    NMT.constants_.insert("stop_remote_node", 0xFF, 0x02);
    NMT.constants_.insert("start_remote_node", 0xFF, 0x01);
    // NMT.constants_.insert("reset_application", 0xFF, 0x81);
    d_.insert(NMT);
    //NMT.constants_.insert(Constant("bla", 10,20));
    //d_.insert(NMT); 
    // d_.insert(EDSClass("NMT","0", 0x0, 0x0, 1, "wo", bm0));

    EDSClass ControlWord("controlword", "6040_0", 0x6040, 0x0, 2, "rw");
    ControlWord.constants_.insert("sm_shutdown", 0xFFFF, 0x6);
    ControlWord.constants_.insert("sm_switch_on", 0xFFFF, 0x7);
    ControlWord.constants_.insert("sm_enable_operation", 0xFFFF, 0x6);
    ControlWord.constants_.insert("start_homing|enable_ip_mode", 0xFFFF, 0x1f);
    ControlWord.constants_.insert("reset_fault", 0xFFFF, 0x80);
    d_.insert(ControlWord);

    // statusword (0x6041): // todo
    EDSClass StatusWord("statusword", "6041_0", 0x6041, 0x0, 2, "ro");
    StatusWord.constants_.insert("not_ready_to_switch_on", 0x004F, 0);
    StatusWord.constants_.insert("switch_on_disabled", 0x004F, 0x0040);
    StatusWord.constants_.insert("ready_to_switch_on", 0x006F, 0x0021);
    StatusWord.constants_.insert("switched_on", 0x006F, 0x0023);
    StatusWord.constants_.insert("operation_enable", 0x006F, 0x0027);
    StatusWord.constants_.insert("quick_stop_active", 0x006F, 0x0007);
    StatusWord.constants_.insert("fault_reaction_active", 0x004F, 0x000F);
    StatusWord.constants_.insert("fault_ds402", 0x004F, 0x0008);
    d_.insert(StatusWord);
    /*bmtype bm6041;
    bm6041.insert(bmtype::value_type("sm_shutdown",0x6));
    d_.insert(EDSClass("statusword","6041_0", 0x6041, 0x0, 2, "ro", bm6041));*/

    
    // modes_of_operation (0x6060)
    EDSClass ModesOfOperation("modes_of_operation", "6060_0", 0x6060, 0x0, 1, "wo");
    ModesOfOperation.constants_.insert("homing_mode", 0xFF, 6);
    ModesOfOperation.constants_.insert("profile_position_mode", 0xFF, 1);
    ModesOfOperation.constants_.insert("profile_velocity_mode", 0xFF, 3);
    ModesOfOperation.constants_.insert("torque_profile_mode", 0xFF, 4);
    ModesOfOperation.constants_.insert("interpolated_position_mode", 0xFF, 7);
    d_.insert(ModesOfOperation);

    /*bmtype bm6061 = bm6060;
      d_.insert(EDSClass("modes_of_operation_display", "6061_0", 0x6061, 0x0, 1, "ro", bm6061)); */

    /*
    // device status indices:
    d_.insert(EDSClass("position_actual_value","6064_0", 0x6064, 0x0, 4, "rw"));
    d_.insert(EDSClass("torque_actual_value","6077_0", 0x6077, 0x0, 2, "rw"));

    // interpolation_time_period: ip_time_units (0x60C2, 01h)
    d_.insert(EDSClass("ip_time_units", "60C2_1", 0x60C2, 0x1, 1, "rw"));

    // interpolation_time_period: ip_time_index (0x60C2, 02h)
    d_.insert(EDSClass("ip_time_index", "60C2_2", 0x60C2, 0x2, 1, "rw"));
    */
  }

  PDODict::PDODict() {
    // rPDOs: 0x200+nodeID, 0x300+nodeID
    // tPDOs: 0x180 0x280 0x380 0x480

    // Schunk default rPDO for device ID 12:
    std::vector<std::string> comp1;
    comp1.push_back("controlword");
    comp1.push_back("notused16"); // not sure what these two bytes are used for
    comp1.push_back("interpolation_data_record:ip_data_position");
    d_.insert(PDOClass("schunk_default_rPDO_12", 0x0c, 0x200 + 0x0c, comp1));

    // Schunk default tPDO for device ID 12:
    std::vector<std::string> comp2;
    comp2.push_back("statusword");
    comp2.push_back("torque_actual_value");
    comp2.push_back("position_actual_value");
    d_.insert(PDOClass("schunk_default_tPDO_12", 0x0c, 0x180 + 0x0c, comp2));

    // Schunk default tPDO for device ID 12:
    std::vector<std::string> comp3;
    comp3.push_back("notused64");
    d_.insert(PDOClass("schunk_4th_tPDO_12", 0x0c, 0x480 + 0x0c, comp3));


  }

  std::vector<std::string> PDODict::getComponents(std::string alias) {
    typedef PDOClassSet::nth_index<0>::type PDOClassSet_by_alias;
    PDOClassSet_by_alias::iterator it=d_.get<0>().find(alias);
    return it->components_;
  }

  uint16_t PDODict::getCobID(std::string alias) {
    typedef PDOClassSet::nth_index<0>::type PDOClassSet_by_alias;
    PDOClassSet_by_alias::iterator it=d_.get<0>().find(alias);
    return it->cobID_;
  }

  std::string PDODict::getAlias(uint16_t cobID) {
    typedef PDOClassSet::nth_index<1>::type PDOClassSet_by_cobID;
    PDOClassSet_by_cobID::iterator it=d_.get<1>().find(cobID);
    return it->alias_;
  }

  uint8_t PDODict::getNodeID(std::string alias) {
    typedef PDOClassSet::nth_index<0>::type PDOClassSet_by_alias;
    PDOClassSet_by_alias::iterator it=d_.get<0>().find(alias);
    return it->nodeID_;

  }

  bool PDODict::cobIDexists(uint16_t cobID) {
    typedef PDOClassSet::nth_index<1>::type PDOClassSet_by_cobID;
    PDOClassSet_by_cobID::iterator it=d_.get<1>().find(cobID);
    return it != d_.get<1>().end();
  }

  uint32_t EDSDict::getConst(std::string alias, std::string constname) {
    typedef EDSClassSet::nth_index<1>::type EDSClassSet_by_alias;
    EDSClassSet_by_alias::iterator it=d_.get<1>().find(alias);
    Constants cc = it->constants_;
    return cc.getValue(constname);
  }

  uint32_t EDSDict::getMask(std::string alias, std::string constname) {
    typedef EDSClassSet::nth_index<1>::type EDSClassSet_by_alias;
    EDSClassSet_by_alias::iterator it=d_.get<1>().find(alias);
    Constants cc = it->constants_;
    return cc.getMask(constname);
  }


  uint8_t EDSDict::getLen(std::string alias) {
    bmtype bm;
    typedef EDSClassSet::nth_index<1>::type EDSClassSet_by_alias;
    EDSClassSet_by_alias::iterator it=d_.get<1>().find(alias);
    return it->length_;
  }

  uint16_t EDSDict::getIndex(std::string alias) {
    bmtype bm;
    typedef EDSClassSet::nth_index<1>::type EDSClassSet_by_alias;
    EDSClassSet_by_alias::iterator it=d_.get<1>().find(alias);
    return it->index_;
  }

  uint8_t EDSDict::getSubindex(std::string alias) {
    bmtype bm;
    typedef EDSClassSet::nth_index<1>::type EDSClassSet_by_alias;
    EDSClassSet_by_alias::iterator it=d_.get<1>().find(alias);
    return it->subindex_;
  }

  std::string EDSDict::getAlias(uint16_t index, uint8_t subindex) {
    EDSClassSet::iterator it = d_.find(boost::make_tuple(index, subindex));
    return it->alias_;
  }

  // Constants
  uint64_t Constants::getMask(std::string name) {
    typedef ConstantContainer::nth_index<1>::type ConstantContainer_by_name;
    ConstantContainer_by_name::iterator it=constants_.get<1>().find(name);
    return it->mask_;
  }

  uint64_t Constants::getValue(std::string name) {
    typedef ConstantContainer::nth_index<1>::type ConstantContainer_by_name;
    ConstantContainer_by_name::iterator it=constants_.get<1>().find(name);
    return it->value_;
  }

  std::string Constants::getName(uint64_t mask, uint64_t value) {
    return "test";
  }

  void Constants::insert(std::string name, uint64_t mask, uint64_t value) {
    constants_.insert(Constant(name, mask, value));
  }
  
}
