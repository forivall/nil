#include "nil.h"
#include "nilUtil.h"

namespace nil {

  Controller::Controller( System* system, Device* device ):
  DeviceInstance( system, device )
  {
    //
  }

  Controller::~Controller()
  {
    //
  }

  const Controller::Type Controller::getType()
  {
    return mType;
  }

}