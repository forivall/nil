#pragma once

#ifndef NTDDI_VERSION
# define NTDDI_VERSION NTDDI_VISTA
# define _WIN32_WINNT _WIN32_WINNT_VISTA
#endif
#include <sdkddkver.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dbt.h>
#include <objbase.h>
#include <stdlib.h>

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <xinput.h>

#include "nilTypes.h"

namespace nil {

  //! \struct Button
  //! Digital push button controller component.
  struct Button {
  public:
    bool pushed;
    Button();
  };

  //! \struct Axis
  //! Analog axis controller component.
  struct Axis {
  public:
    Real absolute;
    Axis();
  };

  //! \struct Slider
  //! Two-dimensional analog controller component.
  struct Slider {
  public:
    Vector2i absolute;
    Slider();
  };

  //! \struct POV
  //! Digital directional Point-of-View controller component.
  struct POV {
  public:
    static const POVDirection Centered   = 0x00000000;
    static const POVDirection North      = 0x00000001;
    static const POVDirection South      = 0x00000010;
    static const POVDirection East       = 0x00000100;
    static const POVDirection West       = 0x00001000;
    static const POVDirection NorthEast  = 0x00000101;
    static const POVDirection SouthEast  = 0x00000110;
    static const POVDirection NorthWest  = 0x00001001;
    static const POVDirection SouthWest  = 0x00001010;
    POVDirection direction;
    POV();
  };

  //! \class WinAPIError
  //! Container for a Windows API error description.
  struct WinAPIError {
  public:
    uint32_t code;
    String description;
  };

  //! \class Exception
  //! Main exception class. Descendant of std::exception.
  class Exception: public std::exception {
  public:
    enum Type: int {
      Generic = 0,
      WinAPI,
      DirectInput
    };
  private:
    Exception();
  protected:
    Type mType;
    String mDescription;
    String mSource;
    mutable String mFullDescription;
    mutable utf8String mUTF8Description;
    variant<WinAPIError> mAdditional;
    void handleAdditional( HRESULT hr = 0 );
  public:
    Exception( const String& description, Type type = Generic );
    Exception( const String& description, const String& source,
      Type type = Generic );
    Exception( const String& description, const String& source,
      HRESULT hr, Type type = Generic );
    virtual const String& getFullDescription() const;
    virtual const char* what() const throw();
  };

  class System;
  class DeviceInstance;

  //! \class Device
  //! Input device information entry.
  class Device {
  friend class System;
  friend class XInputController;
  public:
    enum Handler: int {
      Handler_DirectInput = 0, //!< Implemented by DirectInput
      Handler_XInput //!< Implemented by XInput
    };
    enum Type: int {
      Device_Keyboard = 0, //!< I'm a keyboard
      Device_Mouse, //!< I'm a mouse
      Device_Controller //!< No, I'm a controller
    };
    enum Status: int {
      Status_Disconnected = 0, //!< Disconnected but not forgotten
      Status_Pending, //!< Pending refresh
      Status_Connected //!< Up-to-date and available
    };
  protected:
    DeviceID mID; //!< Unique nil-specific identifier
    Status mStatus; //!< Current status
    Status mSavedStatus; //!< Status backup when updating
    String mName; //!< Device name
    Type mType; //!< Device type
    System* mSystem; //!< My owner
    DeviceInstance* mInstance; //!< My instance, if created
    bool mDisconnectFlagged; //!< Has there been a problem with me?
    Device( System* system, DeviceID id, Type type );
    virtual ~Device();
    virtual void create();
    virtual void update();
    virtual void destroy();
    virtual void setStatus( Status status );
    virtual void saveStatus();
    virtual const Status getSavedStatus();
    virtual void onDisconnect(); //!< On unplugged or otherwise disabled
    virtual void onConnect(); //!< On plugged or otherwise enabled
    virtual void flagDisconnected();
  public:
    virtual const DeviceID getID();
    virtual const Handler getHandler() = 0;
    virtual const Type getType();
    virtual const Status getStatus();
    virtual const String& getName();
    virtual System* getSystem();
    virtual const bool isDisconnectFlagged();
  };

  //! \class DirectInputDevice
  //! Device abstraction base class for DirectInput devices.
  class DirectInputDevice: public Device {
  friend class System;
  protected:
    GUID mProductID;
    GUID mInstanceID;
    DirectInputDevice( System* system, DeviceID id,
      LPCDIDEVICEINSTANCEW instance );
  public:
    virtual const Handler getHandler();
    virtual const GUID getProductID();
    virtual const GUID getInstanceID();
  };

  //! \class XInputDevice
  //! Device abstraction base class for XInput devices.
  class XInputDevice: public Device {
  friend class System;
  protected:
    int mXInputID;
    bool mIdentified;
    XINPUT_CAPABILITIES mCapabilities;
    XInputDevice( System* system, DeviceID id, int xinputID );
    virtual void identify();
    virtual void setStatus( Status status );
    virtual void onDisconnect();
    virtual void onConnect();
  public:
    virtual const Handler getHandler();
    virtual const int getXInputID();
    virtual const XINPUT_CAPABILITIES& getCapabilities();
  };

  typedef list<Device*> DeviceList;

  //! \class DeviceInstance
  //! Device instance base class.
  class DeviceInstance {
  protected:
    System* mSystem;
    Device* mDevice;
  public:
    DeviceInstance( System* system, Device* device );
    virtual void update() = 0;
    virtual ~DeviceInstance();
  };

  //! \class Mouse
  //! Mouse device instance base class.
  class Mouse: public DeviceInstance {
  protected:
  public:
    Mouse( System* system, Device* device );
    virtual void update() = 0;
    virtual ~Mouse();
  };

  //! \class Keyboard
  //! Keyboard device instance base class.
  class Keyboard: public DeviceInstance {
  protected:
  public:
    Keyboard( System* system, Device* device );
    virtual void update() = 0;
    virtual ~Keyboard();
  };

  //! \struct ControllerState
  //! Game controller state structure.
  struct ControllerState {
  public:
    void clear();
    ControllerState();
    vector<Button> mButtons;
    vector<Axis> mAxes;
    vector<Slider> mSliders;
    vector<POV> mPOVs;
  };

  //! \class ControllerListener
  //! Game controller event listener base class.
  //! Derive your own listener from this class.
  class ControllerListener {
  public:
    virtual void onButtonPressed(
      const ControllerState& state, size_t button ) = 0;
    virtual void onButtonReleased(
      const ControllerState& state, size_t button ) = 0;
    virtual void onAxisMoved(
      const ControllerState& state, size_t axis ) = 0;
    virtual void onSliderMoved(
      const ControllerState& state, size_t slider ) = 0;
    virtual void onPOVMoved(
      const ControllerState& state, size_t pov ) = 0;
  };

  typedef list<ControllerListener*> ControllerListenerList;

  //! \class Controller
  //! Game controller device instance base class.
  class Controller: public DeviceInstance {
  public:
    enum Type: int {
      Controller_Unknown = 0,
      Controller_Joystick,
      Controller_Gamepad,
      Controller_Firstperson,
      Controller_Driving,
      Controller_Flight,
      Controller_DancePad,
      Controller_Guitar,
      Controller_Bass,
      Controller_Drumkit,
      Controller_ArcadePad
    };
  protected:
    Type mType;
    ControllerState mState;
    ControllerListenerList mListeners;
    virtual void fireChanges( const ControllerState& lastState );
  public:
    Controller( System* system, Device* device );
    virtual void update() = 0;
    virtual ~Controller();
    virtual const Type getType();
    virtual const ControllerState& getState() const;
  };

  //! \class DirectInputMouse
  //! Mouse implemented by DirectInput.
  class DirectInputMouse: public Mouse {
  protected:
  public:
    DirectInputMouse( DirectInputDevice* device );
    virtual void update();
    virtual ~DirectInputMouse();
  };

  //! \class DirectInputKeyboard
  //! Keyboard implemented by DirectInput.
  class DirectInputKeyboard: public Keyboard {
  protected:
  public:
    DirectInputKeyboard( DirectInputDevice* device );
    virtual void update();
    virtual ~DirectInputKeyboard();
  };

  //! \class DirectInputController
  //! Game controller implemented by DirectInput.
  class DirectInputController: public Controller {
  protected:
  public:
    DirectInputController( DirectInputDevice* device );
    virtual void update();
    virtual ~DirectInputController();
  };

  //! \class XInputController
  //! Game controller implemented by XInput.
  class XInputController: public Controller {
  protected:
    DWORD mLastPacket;
    XINPUT_STATE mXInputState;
    inline Real filterLeftThumbAxis( int val );
    inline Real filterRightThumbAxis( int val );
    inline Real filterTrigger( int val );
  public:
    XInputController( XInputDevice* device );
    virtual void update();
    virtual ~XInputController();
  };

  //! \class PnPListener
  //! Plug-n-Play event listener.
  class PnPListener {
  public:
    virtual void onPlug( const GUID& deviceClass,
      const String& devicePath ) = 0;
    virtual void onUnplug( const GUID& deviceClass,
      const String& devicePath ) = 0;
  };

  //! \class PnPMonitor
  //! Monitors for Plug-n-Play (USB) device events.
  class PnPMonitor {
  protected:
    HINSTANCE mInstance; //!< Host application instance handle
    ATOM mClass; //!< Class registration handle
    HWND mWindow; //!< Window handle
    HDEVNOTIFY mNotifications; //!< Device notifications registration
    PnPListener* mListener; //!< Our listener
  protected:
    void registerNotifications();
    void unregisterNotifications();
    static LRESULT CALLBACK wndProc( HWND window, UINT message,
      WPARAM wParam, LPARAM lParam );
  public:
    PnPMonitor( HINSTANCE instance, PnPListener* listener );
    void update();
    ~PnPMonitor();
  };

  //! \class System
  //! The input system root.
  class System: public PnPListener {
  protected:
    DeviceID mIDPool; //!< Device indexing pool
    vector<DeviceID> mXInputIDs; //!< XInput device ID mapping
    vector<uint32_t> mXInputDeviceIDs; //!< Tracked list of XInput VIDs & PIDs
    IDirectInput8W* mDirectInput; //!< Our DirectInput instance
    HINSTANCE mInstance; //!< Host application instance handle
    HWND mWindow; //!< Host application window handle
    PnPMonitor* mMonitor; //!< Our Plug-n-Play event monitor
    DeviceList mDevices; //!< List of known devices
    bool mInitializing; //!< Are we initializing?
    void initializeDevices();
    void refreshDevices();
    void identifyXInputDevices();
    DeviceID getNextID();
    virtual void onPlug( const GUID& deviceClass, const String& devicePath );
    virtual void onUnplug( const GUID& deviceClass, const String& devicePath );
    static BOOL CALLBACK diEnumCallback(
      LPCDIDEVICEINSTANCEW instance, LPVOID referer );
  public:
    System( HINSTANCE instance, HWND window );
    void update();
    const bool isInitializing();
    ~System();
  };

}