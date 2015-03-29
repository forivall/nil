#include "nil.h"
#include "nilUtil.h"
#include "nilLogitech.h"

namespace Nil {

  void System::Internals::store()
  {
    swapMouseButtons = ( GetSystemMetrics( SM_SWAPBUTTON ) != 0 );

    storedStickyKeys.cbSize = sizeof( storedStickyKeys );
    SystemParametersInfoW( SPI_GETSTICKYKEYS, sizeof( STICKYKEYS ), &storedStickyKeys, 0 );

    storedToggleKeys.cbSize = sizeof( storedToggleKeys );
    SystemParametersInfoW( SPI_GETTOGGLEKEYS, sizeof( TOGGLEKEYS ), &storedToggleKeys, 0 );

    storedFilterKeys.cbSize = sizeof( storedFilterKeys );
    SystemParametersInfoW( SPI_GETFILTERKEYS, sizeof( FILTERKEYS ), &storedFilterKeys, 0 );
  }

  void System::Internals::disableHotKeyHarassment()
  {
    // Don't touch stickykeys/togglekeys/filterkeys if they're being used,
    // but if they aren't, make sure Windows doesn't harass the user about 
    // maybe enabling them.

    auto stickyKeys = storedStickyKeys;
    if ( ( stickyKeys.dwFlags & SKF_STICKYKEYSON ) == 0 )
    {
      stickyKeys.dwFlags &= ~SKF_HOTKEYACTIVE;
      stickyKeys.dwFlags &= ~SKF_CONFIRMHOTKEY;
      SystemParametersInfoW( SPI_SETSTICKYKEYS, sizeof( STICKYKEYS ), &stickyKeys, 0 );
    }

    auto toggleKeys = storedToggleKeys;
    if ( ( toggleKeys.dwFlags & TKF_TOGGLEKEYSON ) == 0 )
    {
      toggleKeys.dwFlags &= ~TKF_HOTKEYACTIVE;
      toggleKeys.dwFlags &= ~TKF_CONFIRMHOTKEY;
      SystemParametersInfoW( SPI_SETTOGGLEKEYS, sizeof( TOGGLEKEYS ), &toggleKeys, 0 );
    }

    auto filterKeys = storedFilterKeys;
    if ( ( filterKeys.dwFlags & FKF_FILTERKEYSON ) == 0 )
    {
      filterKeys.dwFlags &= ~FKF_HOTKEYACTIVE;
      filterKeys.dwFlags &= ~FKF_CONFIRMHOTKEY;
      SystemParametersInfoW( SPI_SETFILTERKEYS, sizeof( FILTERKEYS ), &filterKeys, 0 );
    }
  }

  void System::Internals::restore()
  {
    SystemParametersInfoW( SPI_SETSTICKYKEYS, sizeof( STICKYKEYS ), &storedStickyKeys, 0 );
    SystemParametersInfoW( SPI_SETTOGGLEKEYS, sizeof( TOGGLEKEYS ), &storedToggleKeys, 0 );
    SystemParametersInfoW( SPI_SETFILTERKEYS, sizeof( FILTERKEYS ), &storedFilterKeys, 0 );
  }

  System::System( HINSTANCE instance, HWND window, const Cooperation coop,
  SystemListener* listener ): mCooperation( coop ),
  mWindow( window ), mInstance( instance ), mDirectInput( nullptr ),
  mMonitor( nullptr ), mIDPool( 0 ), mInitializing( true ),
  mHIDManager( nullptr ), mLogitechGKeys( nullptr ), mLogitechLEDs( nullptr ),
  mListener( listener ), mMouseIndexPool( 0 ), mKeyboardIndexPool( 0 ),
  mControllerIndexPool( 0 ), mXInput( nullptr )
  {
    assert( mListener );

    // Validate the passes window handle
    if ( !IsWindow( mWindow ) )
      NIL_EXCEPT( "Window handle is invalid" );

    // Store accessibility feature states, and tell Windows not to be annoying
    mInternals.store();
    mInternals.disableHotKeyHarassment();

    // Init Logitech SDKs where available
    mLogitechGKeys = new Logitech::GKeySDK();
    mLogitechLEDs = new Logitech::LedSDK();

    // Init XInput subsystem
    mXInput = new XInput();
    if ( mXInput->initialize() != ExternalModule::Initialization_OK )
      NIL_EXCEPT( "Loading XInput failed" );

    // Create DirectInput instance
    auto hr = DirectInput8Create( mInstance, DIRECTINPUT_VERSION,
      IID_IDirectInput8W, (LPVOID*)&mDirectInput, NULL );
    if ( FAILED( hr ) )
      NIL_EXCEPT_DINPUT( hr, "Could not instantiate DirectInput 8" );

    // Initialize our event monitor
    mMonitor = new EventMonitor( mInstance, mCooperation );

    // Initialize our HID manager
    mHIDManager = new HIDManager();

    // Register the HID manager and ourselves as PnP event listeners
    mMonitor->registerPnPListener( mHIDManager );
    mMonitor->registerPnPListener( this );

    // Register ourselves as a raw event listener
    mMonitor->registerRawListener( this );
    
    // Fetch initial devices
    initializeDevices();
    refreshDevices();

    // Update the monitor once, to receive initial Raw devices
    mMonitor->update();

    mInitializing = false;
  }

  DeviceID System::getNextID()
  {
    return mIDPool++;
  }

  void System::onPnPPlug( const GUID& deviceClass, const wideString& devicePath )
  {
    // Refresh all currently connected devices,
    // since IDirectInput8::FindDevice doesn't do jack shit
    refreshDevices();
  }

  void System::onPnPUnplug( const GUID& deviceClass, const wideString& devicePath )
  {
    // Refresh all currently connected devices,
    // since IDirectInput8::FindDevice doesn't do jack shit
    refreshDevices();
  }

  void System::onRawArrival( HANDLE handle )
  {
    UINT pathLength = 0;

    if ( GetRawInputDeviceInfoW( handle, RIDI_DEVICENAME, NULL, &pathLength ) )
      NIL_EXCEPT_WINAPI( "GetRawInputDeviceInfoW failed" );

    wideString rawPath( pathLength, '\0' );

    GetRawInputDeviceInfoW( handle, RIDI_DEVICENAME, &rawPath[0], &pathLength );
    rawPath.resize( rawPath.length() - 1 );

    for ( auto device : mDevices )
    {
      if ( device->getHandler() != Device::Handler_RawInput )
        continue;

      auto rawDevice = static_cast<RawInputDevice*>( device );

      if ( !_wcsicmp( rawDevice->getRawPath().c_str(), rawPath.c_str() ) )
      {
        deviceConnect( rawDevice );
        return;
      }
    }

    auto device = new RawInputDevice( this, getNextID(), handle, rawPath );

    if ( isInitializing() )
      device->setStatus( Device::Status_Connected );
    else
      deviceConnect( device );

    mDevices.push_back( device );
  }

  void System::onRawMouseInput( HANDLE handle,
  const RAWMOUSE& input, const bool sinked )
  {
    if ( mInitializing || !handle )
      return;

    auto it = mMouseMapping.find( handle );
    if ( it != mMouseMapping.end() )
      it->second->onRawInput( input );
  }

  void System::onRawKeyboardInput( HANDLE handle,
  const RAWKEYBOARD& input, const bool sinked )
  {
    if ( mInitializing || !handle )
      return;

    auto it = mKeyboardMapping.find( handle );
    if ( it != mKeyboardMapping.end() )
      it->second->onRawInput( input );
  }

  void System::onRawRemoval( HANDLE handle )
  {
    for ( auto device : mDevices )
    {
      if ( device->getHandler() != Device::Handler_RawInput )
        continue;

      auto rawDevice = static_cast<RawInputDevice*>( device );

      if ( rawDevice->getRawHandle() == handle )
      {
        deviceDisconnect( rawDevice );
        return;
      }
    }
  }

  void System::mapMouse( HANDLE handle, RawInputMouse* mouse )
  {
    mMouseMapping[handle] = mouse;
  }

  void System::unmapMouse( HANDLE handle )
  {
    mMouseMapping.erase( handle );
  }

  void System::mapKeyboard( HANDLE handle, RawInputKeyboard* keyboard )
  {
    mKeyboardMapping[handle] = keyboard;
  }

  void System::unmapKeyboard( HANDLE handle )
  {
    mKeyboardMapping.erase( handle );
  }

  void System::initializeDevices()
  {
    mXInputIDs.resize( XUSER_MAX_COUNT );
    for ( int i = 0; i < XUSER_MAX_COUNT; i++ )
    {
      mXInputIDs[i] = getNextID();
      auto device = new XInputDevice( this, mXInputIDs[i], i );
      mDevices.push_back( device );
    }
  }

  void System::refreshDevices()
  {
    identifyXInputDevices();

    for ( Device* device : mDevices )
      if ( device->getHandler() == Device::Handler_DirectInput )
      {
        device->saveStatus();
        device->setStatus( Device::Status_Pending );
      }

    auto hr = mDirectInput->EnumDevices( DI8DEVCLASS_GAMECTRL,
      diDeviceEnumCallback, this, DIEDFL_ATTACHEDONLY );
    if ( FAILED( hr ) )
      NIL_EXCEPT_DINPUT( hr, "Could not enumerate DirectInput devices!" );

    for ( Device* device : mDevices )
      if ( device->getHandler() == Device::Handler_DirectInput
        && device->getSavedStatus() == Device::Status_Connected
        && device->getStatus() == Device::Status_Pending )
        deviceDisconnect( device );

    XINPUT_STATE state;
    for ( Device* device : mDevices )
    {
      if ( device->getHandler() == Device::Handler_XInput )
      {
        auto xDevice = static_cast<XInputDevice*>( device );
        auto status = mXInput->mFunctions.pfnXInputGetState( xDevice->getXInputID(), &state );
        if ( status == ERROR_DEVICE_NOT_CONNECTED )
        {
          if ( xDevice->getStatus() == Device::Status_Connected )
            deviceDisconnect( xDevice );
          else if ( xDevice->getStatus() == Device::Status_Pending )
            xDevice->setStatus( Device::Status_Disconnected );
        }
        else if ( status == ERROR_SUCCESS )
        {
          if ( xDevice->getStatus() == Device::Status_Disconnected )
            deviceConnect( xDevice );
          else if ( xDevice->getStatus() == Device::Status_Pending )
            xDevice->setStatus( Device::Status_Connected );
        }
        else
          NIL_EXCEPT( "XInputGetState failed" );
      }
    }
  }

  BOOL CALLBACK System::diDeviceEnumCallback( LPCDIDEVICEINSTANCEW instance,
  LPVOID referer )
  {
    auto system = reinterpret_cast<System*>( referer );

    for ( auto identifier : system->mXInputDeviceIDs )
      if ( instance->guidProduct.Data1 == identifier )
        return DIENUM_CONTINUE;

    for ( auto device : system->mDevices )
    {
      if ( device->getHandler() != Device::Handler_DirectInput )
        continue;

      auto diDevice = static_cast<DirectInputDevice*>( device );

      if ( diDevice->getInstanceID() == instance->guidInstance )
      {
        if ( device->getSavedStatus() == Device::Status_Disconnected )
          system->deviceConnect( device );
        else
          device->setStatus( Device::Status_Connected );

        return DIENUM_CONTINUE;
      }
    }

    Device* device = new DirectInputDevice( system, system->getNextID(), instance );

    if ( system->isInitializing() )
      device->setStatus( Device::Status_Connected );
    else
      system->deviceConnect( device );

    system->mDevices.push_back( device );

    return DIENUM_CONTINUE;
  }

  void System::deviceConnect( Device* device )
  {
    device->onConnect();
    mListener->onDeviceConnected( device );
  }

  void System::deviceDisconnect( Device* device )
  {
    device->onDisconnect();
    mListener->onDeviceDisconnected( device );
  }

  void System::mouseEnabled( Device* device, Mouse* instance )
  {
    mListener->onMouseEnabled( device, instance );
  }

  void System::mouseDisabled( Device* device, Mouse* instance )
  {
    mListener->onMouseDisabled( device, instance );
  }

  void System::keyboardEnabled( Device* device, Keyboard* instance )
  {
    mListener->onKeyboardEnabled( device, instance );
  }

  void System::keyboardDisabled( Device* device, Keyboard* instance )
  {
    mListener->onKeyboardDisabled( device, instance );
  }

  void System::controllerEnabled( Device* device, Controller* instance )
  {
    mListener->onControllerEnabled( device, instance );
  }

  void System::controllerDisabled( Device* device, Controller* instance )
  {
    mListener->onControllerDisabled( device, instance );
  }

  void System::identifyXInputDevices()
  {
    mXInputDeviceIDs.clear();
    for ( auto hidRecord : mHIDManager->getRecords() )
      if ( hidRecord->isXInput() )
        mXInputDeviceIDs.push_back( hidRecord->getIdentifier() );
  }

  DeviceList& System::getDevices()
  {
    return mDevices;
  }

  const bool System::isInitializing() const
  {
    return mInitializing;
  }

  int System::getNextMouseIndex()
  {
    return ++mMouseIndexPool;
  }

  int System::getNextKeyboardIndex()
  {
    return ++mKeyboardIndexPool;
  }

  int System::getNextControllerIndex()
  {
    return ++mControllerIndexPool;
  }

  void System::update()
  {
    // Run PnP & raw events if there are any
    mMonitor->update();

    // Make sure that we disconnect failed devices,
    // and update the rest
    for ( Device* device : mDevices )
      if ( device->isDisconnectFlagged() )
        deviceDisconnect( device );
      else
        device->update();

    // Run queued G-key events if using the SDK
    if ( mLogitechGKeys->isInitialized() )
      mLogitechGKeys->update();
  }

  Logitech::GKeySDK* System::getLogitechGKeys()
  {
    return mLogitechGKeys;
  }

  Logitech::LedSDK* System::getLogitechLEDs()
  {
    return mLogitechLEDs;
  }

  XInput* System::getXInput()
  {
    return mXInput;
  }

  System::~System()
  {
    for ( Device* device : mDevices )
    {
      device->disable();
      delete device;
    }

    SAFE_DELETE( mHIDManager );
    SAFE_DELETE( mMonitor );
    SAFE_RELEASE( mDirectInput );
    SAFE_DELETE( mLogitechLEDs );
    SAFE_DELETE( mLogitechGKeys );
    SAFE_DELETE( mXInput );
    
    // Restore accessiblity features
    mInternals.restore();
  }

}