#include "nil.h"
#include "nilWindows.h"

namespace Nil {

  ExternalModule::ExternalModule(): mModule( NULL ), mInitialized( false )
  {
  }

  bool ExternalModule::isInitialized() const
  {
    return mInitialized;
  }

}