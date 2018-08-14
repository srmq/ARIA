#ifndef ARNETMODETESTLOOP_H
#define ARNETMODETESTLOOP_H

#include "ariaTypedefs.h"
#include "Aria.h"
#include "ArNetworking.h"
#include "ArServerMode.h"

/** This mode uses the most simple robot control commands to drive back and
 * forth along a line repeatedly. It can be activated by an ArNetworking
 * "testloop" request, or by a user using custom text commands interface (if
 * command handler is passed in constructor or addCommands() is called).
 * Some options are added to the configuration if addToConfig() is called or 
 * an ArConfig object is given in the constructor.
 */
class ArServerModeTestLoop : public ArServerMode
{
public:
  AREXPORT ArServerModeTestLoop(ArServerBase *server, ArRobot *robot, ArConfig *config = NULL, ArServerHandlerCommands *cmds = NULL);
  AREXPORT virtual ~ArServerModeTestLoop();
  AREXPORT virtual void activate(void);
  AREXPORT virtual void deactivate(void);
  AREXPORT void addToConfig(ArConfig *config);
  AREXPORT void addCommands(ArServerHandlerCommands *cmds);
private:
  void netTestLoop(ArServerClient *client, ArNetPacket *packet);
  virtual void userTask(void);
  virtual void checkDefault(void) { activate(); }
protected:
  ArFunctor2C<ArServerModeTestLoop, ArServerClient *, ArNetPacket *> myNetTestLoopCB;
  int myPadding, myStopDist, mySpeed, myDriveDist, myRotSpeed;
  int myRotAccel, myRotDecel, myObsDecel;
  ArPose myStartPos;
  enum {Trans, Stop, Rot, Deactivated} myState;
  ArFunctorC<ArServerModeTestLoop> myActivateCB;
};

#endif // ARNETMODETESTLOOP_H
