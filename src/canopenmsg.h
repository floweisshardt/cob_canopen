#ifndef CANOPENMSG_H
#define CANOPENMSG_H

#include <boost/bimap.hpp>
#include <string>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <fcntl.h>    // O_RDWR
#include <libpcan.h>
#include <vector>
#include <stdint.h>
#include <inttypes.h>
#include <thread>


namespace canopen {
  extern HANDLE h;
 

  namespace bmi = boost::multi_index;
  typedef boost::bimap<std::string, uint32_t> bmtype;

  struct Constant {
  Constant(std::string constName, uint64_t mask, uint64_t value):
    constName_(constName), mask_(mask), value_(value) {};
    
    std::string constName_;
    uint64_t mask_;
    uint64_t value_;
  };
  
  struct mask_value_key:bmi::composite_key<
  Constant,
    BOOST_MULTI_INDEX_MEMBER(Constant, uint64_t, mask_),
    BOOST_MULTI_INDEX_MEMBER(Constant, uint64_t, value_)
    >{};

  typedef bmi::multi_index_container<Constant,
    bmi::indexed_by<
    bmi::ordered_unique<mask_value_key>,
    bmi::ordered_unique<bmi::member<Constant, std::string, &Constant::constName_> >
    >
    > ConstantContainer;

  class Constants {
  public:
    Constants() {};
    uint64_t getMask(std::string name);
    uint64_t getValue(std::string name);
    std::string getName(uint64_t mask, uint64_t value);
    void insert(std::string name, uint64_t mask, uint64_t value);
  private:
    ConstantContainer constants_;
  };


  class EDSClass {
  public:
    std::string alias_;
    std::string index_subindex_; // todo: remove!
    uint16_t index_;
    uint8_t subindex_;
    uint8_t length_;
    std::string attr_;
    std::vector<uint64_t> masks_;  // todo:

    //  private:
    Constants constants_;
    // std::vector<bmtype> constants_; // todo!

  EDSClass(std::string alias, std::string index_subindex, uint16_t index, uint8_t subindex,
	   uint8_t length, std::string attr, Constants constants):
    alias_(alias), index_subindex_(index_subindex), index_(index), subindex_(subindex),
      length_(length), attr_(attr), constants_(constants) {}

  EDSClass(std::string alias, std::string index_subindex, uint16_t index, uint8_t subindex,
	   uint8_t length, std::string attr):
    alias_(alias), index_subindex_(index_subindex), index_(index), subindex_(subindex),
      length_(length), attr_(attr) {}
  };

  struct index_subindex_key : bmi::composite_key<
    EDSClass,
    BOOST_MULTI_INDEX_MEMBER(EDSClass, uint16_t, index_),
    BOOST_MULTI_INDEX_MEMBER(EDSClass, uint8_t, subindex_)
    >{};

  struct byAlias;
  struct byIndex;
  struct byCobID;
  typedef bmi::multi_index_container<EDSClass,
    bmi::indexed_by<
    bmi::ordered_unique<index_subindex_key>,
    bmi::ordered_unique<bmi::tag<byAlias>, bmi::member<EDSClass, std::string, &EDSClass::alias_> >
    
    // bmi::ordered_unique<bmi::tag<byIndex>, bmi::member<EDSClass, std::string, &EDSClass::index_subindex_> >
    >
    > EDSClassSet;

  // uint32_t getEDSconst(std::string alias, std::string constname);

  class EDSDict {
  public:
    EDSDict();
    uint32_t getConst(std::string alias, std::string constname);
    uint32_t getMask(std::string alias, std::string constname);
    uint8_t getLen(std::string alias);
    uint16_t getIndex(std::string alias);
    uint8_t getSubindex(std::string alias);
    std::string getAlias(uint16_t index, uint8_t subindex);

    // private:
    EDSClassSet d_;
  };

  struct PDOClass {  // todo:
    std::string alias_;
    uint8_t nodeID_;
    uint16_t cobID_;
    std::vector<std::string> components_; // entries into EDSDict

  PDOClass(std::string alias, uint8_t nodeID, uint16_t cobID, std::vector<std::string> components):
    alias_(alias), nodeID_(nodeID), cobID_(cobID), components_(components) {}
  };
  typedef bmi::multi_index_container<PDOClass,
    bmi::indexed_by<
    bmi::ordered_unique<bmi::tag<byAlias>, bmi::member<PDOClass, std::string, &PDOClass::alias_> >,
    bmi::ordered_unique<bmi::tag<byCobID>, bmi::member<PDOClass, std::uint16_t, &PDOClass::cobID_> >
    >
    > PDOClassSet;

  class PDODict {
  public:
    PDODict();
    std::vector<std::string> getComponents(std::string alias);
    uint16_t getCobID(std::string alias);
    std::string getAlias(uint16_t cobID);
    uint8_t getNodeID(std::string alias);
    bool cobIDexists(uint16_t cobID);
  private:
    PDOClassSet d_;
  };

  class Message {  // abstract representation of CANopen messages
  public:
    Message(TPCANRdMsg m); // construct message from raw CAN message (for messages coming in from bus)
    Message(uint8_t nodeID, std::string alias, uint32_t value=0); // user-constructed non-PDO message
    Message(std::string alias, std::vector<uint32_t> values); // user-constructed PDO message
    void writeCAN(bool writeMode=true);
    static Message* readCAN(bool blocking=true);
    // std::vector<std::string> parse(); // todo: not needed? / translate message from device into human-readable 
    Message* waitForSDOAnswer();
    void debugPrint();
    void debugPrintFlags();
    bool checkForConstant(std::string constName); // only for SDOs so far
    std::vector<std::string> parseFlags();
  private:
    uint8_t nodeID_; // PDOs already have this in their PDOClass entry, but duplicated here for convenience
    std::string alias_; // entry into EDSDict, or PDODict, depending on message
    std::vector<uint32_t> values_;

    std::string createMsgHash();
    std::string createMsgHash(TPCANMsg m);
  };
  
  // todo: add a flag to openConnection + static member variable in Message that performs logging of in- and outgoing messages with timestamp
  
  void debug_show_pendingSDOReplies();

  // composite of multiple CANopen commands
  bool initDevice(uint16_t deviceID);
  void initListener();
  void listener_func();
  void homing(uint16_t deviceID, uint32_t sleep_ms=0);
  
  // wrapper for single CANopen commands
  void stopRemoteNode();
  void sendPos(std::string alias, uint32_t pos);
  bool openConnection(std::string devName);
  void closeConnection();
  void ipMode(uint16_t deviceID);
  void sendSync();
  void modesOfOperationDisplay(uint16_t deviceID);
  Message* statusWord(uint16_t deviceID); // , uint16_t waitForValue=0);
}

// todo: message caching; data changing of existing messages (for max. efficiency)
#endif
