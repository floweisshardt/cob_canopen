#include "canopenmsg.h"
#include <thread>
#include <chrono>

int main() {
  if (!canopen::openConnection("/dev/pcan32")) {
    std::cout << "Cannot open CAN device" << std::endl;
    return -1;
  }
  canopen::Message* SDOreply; // SDO replies will be stored here
  canopen::initListener();    // initialize listener thread


  SDOreply = canopen::statusWord(12);

  if (SDOreply->checkForConstant("not_ready_to_switch_on")) {
    std::cout << "not_ready_to_switch_on" << std::endl;
  } else {
    std::cout << "other state" << std::endl;
  }

  bool deviceStatus = canopen::initDevice(12); // todo: check why this does not work yet
  std::cout << "deviceStatus: " << deviceStatus << std::endl;

  return 0;
  

  SDOreply = canopen::statusWord(12);

  if (SDOreply->checkForConstant("switched_on")) {
    std::cout << "switched_on" << std::endl;
  } else {
    std::cout << "other state" << std::endl;
  }


  // SDOreply->parseFlags();
  // for (auto it : SDOreply) std::cout << it << ", ";
  // std::endl;

  // if (SDOreply == "....") {}

  // canopen::stopRemoteNode();

  canopen::debug_show_pendingSDOReplies();
  
  while (true) {}
  return 0;

  canopen::homing(12, 5000);
  /*
  canopen::ipMode(12);
  canopen::sendSync();
  std::this_thread::sleep_for(std::chrono::milliseconds(10)); 

  int pos = 0;
  for (int i=0; i<1000; i++) {
    canopen::sendPos("schunk_default_rPDO_12", pos);
    pos += 100;
    std::this_thread::sleep_for(std::chrono::milliseconds(10)); 
    canopen::sendSync();
  }
*/
  while (true) {}
  canopen::closeConnection(); 

  return 0;
}
