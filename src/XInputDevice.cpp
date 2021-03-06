#include "nil.h"
#include "nilUtil.h"
#include "nilWindows.h"

namespace Nil {

  const utf8String cXInputDefaultName = "XInput Controller";

#if(_WIN32_WINNT >= _WIN32_WINNT_WIN8)
  const long cMaxXInputModels = 11;
  static std::pair<int,utf8String> cXInputModels[cMaxXInputModels] = {
    std::make_pair( XINPUT_DEVSUBTYPE_UNKNOWN, cXInputDefaultName ),
    std::make_pair( XINPUT_DEVSUBTYPE_GAMEPAD, "XBOX 360 Gamepad" ),
    std::make_pair( XINPUT_DEVSUBTYPE_WHEEL, "XBOX 360 Racing Wheel" ),
    std::make_pair( XINPUT_DEVSUBTYPE_ARCADE_STICK, "XBOX 360 Arcade Stick" ),
    std::make_pair( XINPUT_DEVSUBTYPE_FLIGHT_STICK, "XBOX 360 Flight Stick" ),
    std::make_pair( XINPUT_DEVSUBTYPE_DANCE_PAD, "XBOX 360 Dance Pad" ),
    std::make_pair( XINPUT_DEVSUBTYPE_GUITAR, "XBOX 360 Guitar" ),
    std::make_pair( XINPUT_DEVSUBTYPE_GUITAR_ALTERNATE, "XBOX 360 Alternate Guitar" ),
    std::make_pair( XINPUT_DEVSUBTYPE_GUITAR_BASS, "XBOX 360 Bass Guitar" ),
    std::make_pair( XINPUT_DEVSUBTYPE_DRUM_KIT, "XBOX 360 Drum Kit" ),
    std::make_pair( XINPUT_DEVSUBTYPE_ARCADE_PAD, "XBOX 360 Arcade Pad" )
  };
#else
  const long cMaxXInputModels = 1;
  static std::pair<int,utf8String> cXInputModels[cMaxXInputModels] = {
    std::make_pair( XINPUT_DEVSUBTYPE_GAMEPAD, "XBOX 360 Gamepad" )
  };
#endif

  XInputDevice::XInputDevice( System* system, DeviceID id, int xinputID ):
  Device( system, id, Device_Controller ), mXInputID( xinputID ),
  mIdentified( false )
  {
    memset( &mCapabilities, NULL, sizeof( XINPUT_CAPABILITIES ) );
    mName = cXInputDefaultName;
  }

  void XInputDevice::identify()
  {
    if ( mSystem->getXInput()->mFunctions.pfnXInputGetCapabilities( mXInputID, 0, &mCapabilities ) != ERROR_SUCCESS )
      return;

    for ( int i = 0; i < cMaxXInputModels; i++ ) {
      if ( cXInputModels[i].first == mCapabilities.SubType )
        mName = cXInputModels[i].second;
    }

    mIdentified = true;
  }

  const DeviceID XInputDevice::getStaticID() const
  {
    // Static ID for XInput devices:
    // 4 bits of handler ID, 28 bits of XInput controller ID (1-4)

    DeviceID id = ( mXInputID | ( ( Handler_XInput + 1 ) << 28 ) );
    return id;
  }

  void XInputDevice::onConnect()
  {
    if ( !mIdentified )
      identify();

    Device::onConnect();
  }

  void XInputDevice::onDisconnect()
  {
    Device::onDisconnect();
  }

  void XInputDevice::setStatus( Status status )
  {
    if ( status == Status_Connected && !mIdentified )
      identify();

    Device::setStatus( status );
  }

  const Device::Handler XInputDevice::getHandler() const
  {
    return Device::Handler_XInput;
  }

  const int XInputDevice::getXInputID() const
  {
    return mXInputID;
  }

  const XINPUT_CAPABILITIES& XInputDevice::getCapabilities() const
  {
    return mCapabilities;
  }

}