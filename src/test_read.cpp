#include <thread>
#include <chrono>
#include "canopenmsg.h"

void listener_func() {
  TPCANRdMsg m;
  canopen::Message* msg;

  while (true) {
    msg = canopen::Message::readCAN(true);
  }
}

int main() {

  if (!canopen::openConnection("/dev/pcan32")) {
    std::cout << "Cannot open CAN device" << std::endl;
    return -1;
  }

  std::thread listener_thread(listener_func);
  
  canopen::initDevice(12);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  canopen::homing(12);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  canopen::statusWord(12);
  canopen::modesOfOperationDisplay(12);

  std::this_thread::sleep_for(std::chrono::seconds(5));
  canopen::ipMode(12);
  canopen::sendSync();
  std::this_thread::sleep_for(std::chrono::milliseconds(10)); 

  /*
  int pos = 0;
  for (int i=0; i<1000; i++) {
    canopen::sendPos("schunk_default_rPDO_12", pos);
    pos += 100;
    std::this_thread::sleep_for(std::chrono::milliseconds(10)); 
    canopen::sendSync();
    }
  */
  std::this_thread::sleep_for(std::chrono::seconds(5));
  // listener_thread.join();

  canopen::closeConnection();
  return 0;
}
