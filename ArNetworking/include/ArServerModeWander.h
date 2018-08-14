#ifndef ARNETMODEWANDER_H
#define ARNETMODEWANDER_H

#include "ariaTypedefs.h"
#include "Aria.h"
#include "ArNetworking.h"
#include "ArServerMode.h"

class ArServerModeWander : public ArServerMode
{
public:
  AREXPORT ArServerModeWander(ArServerBase *server, ArRobot *robot, int forwardVel = 400, int avoidFrontDist = 450, int avoidVel = 200, int turnAmt = 15);
  AREXPORT virtual ~ArServerModeWander();
  AREXPORT virtual void activate(void);
  AREXPORT virtual void deactivate(void);
  AREXPORT void wander(void);
  AREXPORT void netWander(ArServerClient *client, ArNetPacket *packet);
  AREXPORT virtual void userTask(void);
  virtual void checkDefault(void) { activate(); }
  virtual ArActionGroup *getActionGroup(void) {return &myWanderGroup;}
protected:
  ArActionGroupWander myWanderGroup;
  ArFunctor2C<ArServerModeWander, ArServerClient *, ArNetPacket *> myNetWanderCB;
  
  class StopIfNoRangeDevicesAction : public virtual ArAction
  {
  public:
    StopIfNoRangeDevicesAction() : ArAction("StopIfNoRangeDevices", "Prevents robot motion if there are no range device objects attached to the ArRobot object") 
    {}
  private:
    ArActionDesired myDesired;
    virtual ArActionDesired* fire(ArActionDesired  )
    {
      if(getRobot()->getRangeDeviceList()->size() == 0)
      {
        myDesired.reset();
        myDesired.setMaxVel(0);
        myDesired.setMaxNegVel(0);
        myDesired.setMaxLeftLatVel(0);
        myDesired.setMaxRightLatVel(0);
        myDesired.setMaxRotVel(0);
        return &myDesired;
      }
      return NULL;
    }
  };

  StopIfNoRangeDevicesAction myStopIfNoRangeDevicesAction; 
};

#endif // ARNETMODEWANDER_H
