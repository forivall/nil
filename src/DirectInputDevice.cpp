#include "nil.h"
#include "nilUtil.h"

namespace nil {

  DirectInputDevice::DirectInputDevice( DeviceID id, LPCDIDEVICEINSTANCEW instance ):
  Device( id, Device_Controller ), mProductID( instance->guidProduct ),
  mInstanceID( instance->guidInstance )
  {
    unsigned long deviceType = GET_DIDEVICE_TYPE( instance->dwDevType );

    switch ( deviceType )
    {
      case DI8DEVTYPE_MOUSE:
        mType = Device_Mouse;
      break;
      case DI8DEVTYPE_KEYBOARD:
        mType = Device_Keyboard;
      break;
    }
  }

  const Device::Handler DirectInputDevice::getHandler()
  {
    return Device::Handler_DirectInput;
  }

  const GUID DirectInputDevice::getProductID()
  {
    return mProductID;
  }

  const GUID DirectInputDevice::getInstanceID()
  {
    return mInstanceID;
  }

}