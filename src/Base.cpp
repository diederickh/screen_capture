#include <screencapture/Base.h>

namespace sc {

  Base::Base()
    :state(SC_NONE)
    ,callback(NULL)
    ,user(NULL)
  {
  }

  Base::~Base() {

    state = SC_NONE;
    callback = NULL;
    user = NULL;
  }
  
} /* namespace sc */
