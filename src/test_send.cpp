#include "canopenmsg.h"
#include <thread>
#include <chrono>

int main() {
  if (!canopen::openConnection("/dev/pcan32")) {
    std::cout << "Cannot open CAN device" << std::endl;
    return -1;
  }

  canopen::initDevice(12);
  canopen::homing(12);

  std::this_thread::sleep_for(std::chrono::seconds(5)); 

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

  canopen::closeConnection();
  return 0;
}
