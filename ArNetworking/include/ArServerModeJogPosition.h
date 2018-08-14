
#ifndef ARSERVERMODEJOGPOSITION_H
#define ARSERVERMODEJOGPOSITION_H

#include "ariaTypedefs.h"
#include "Aria.h"
#include "ArServerMode.h"

class ArServerHandlerCommands;

/** Mode that allows clients to request simple discrete motions of the robot.

    From a client, send the "moveDist" request for forward/backward motion by the given amount (double, mm).
    For Example:
@code
    client.requestOnceWithDouble("moveDist", 500.0);
@endcode

    From a client, send the "turnByAngle" request to rotate by the given angle (double, deg).
    For Example:
@code
    client.requestOnceWithDouble("turnByAngle", -90.0);
@endcode

    From a client, send the "turnToHeading" request to rotate to a given absolute heading angle (double, deg). 
    For Example:
@code
    client.requestOnceWithDouble("turnToHeading", 25.5);
@endcode

    This mode will become active if any of the above requests are received.  any
active robot motion will be stopped once the mode becomes active, and also upon
deactivation before switching to a new mode.

    Each moveDist request interrupts any previous moveDist request if
still active.  Each turnByAngle or turnToHeading interrupts any previous turnByAngle
or turnToHeading if still active.

    For continuous velocity control, see ArServerModeRatioDrive instead.

    Motion will be limited based on obstacles sensed. Clearences may be
    configured in configuration (ArConfig) if addToConfig() is called to
    associate with an ArConfig object.  (E.g.  <code>motionMode.addToConfig(Aria::getConfig())</code>).
*/

class ArServerModeJogPosition : public ArServerMode
{
public:
  AREXPORT ArServerModeJogPosition(ArServerBase *server, ArRobot *robot, const char *name = "jogPositionMode", ArServerHandlerCommands *customCommands = NULL);
  AREXPORT virtual ~ArServerModeJogPosition();
  AREXPORT virtual void activate(void);
  AREXPORT virtual void deactivate(void);


  /// Adds to a config in a section
  AREXPORT void addToConfig(ArConfig *config, const char *section = "Jog Robot Position");
  /// Add string commands for users to do movements in MobileEyes or other clients
  AREXPORT void addCommands(ArServerHandlerCommands *commands);
 // AREXPORT virtual void userTask(void);
  AREXPORT virtual ArActionGroup *getActionGroup(void) { return &myActionGroup;}
  ArActionInput* getTurnAction() { return myTurnAction; }
  ArActionDriveDistance* getDriveAction() { return myDriveAction; }

  /// Adds a callback when trying to back up
//  void addDrivingBackwardsCallback(ArFunctor *functor, int position = 50)
//    { myDrivingBackwardsCallbacks.addCallback(functor, position); }
  /// Removes a callback for trying to back up
//  void remDrivingBackwardsCallback(ArFunctor *functor)
 //   { myDrivingBackwardsCallbacks.remCallback(functor); }

  /// Request a turn. Mode must be active.
  AREXPORT void turn(double angle);
  /// Request a movement. Mode must be active.
  AREXPORT void move(double distance);
  /// Request a movement. Mode must be active.
  AREXPORT void heading(double angle);
      
protected:
  void serverMove(ArServerClient *client, ArNetPacket *packet);
  void serverTurn(ArServerClient *client, ArNetPacket *packet);
  void serverHeading(ArServerClient *client, ArNetPacket *packet);
  bool myPrinting;

  ArActionDeceleratingLimiter *myLimiterForward;
  ArActionDeceleratingLimiter *myLimiterBackward;
  ArActionDeceleratingLimiter *myLimiterLateralLeft;
  ArActionDeceleratingLimiter *myLimiterLateralRight;
  ArActionLimiterRot *myLimiterRot;
  ArActionInput *myTurnAction;
  ArActionDriveDistance *myDriveAction;
  ArActionMovementParameters *myMovementParameters;
  ArActionGroup myActionGroup;
  ArTime myLastCommand;
  ArFunctor2C<ArServerModeJogPosition, ArServerClient *, ArNetPacket *> myServerMoveCB;
  ArFunctor2C<ArServerModeJogPosition, ArServerClient *, ArNetPacket *> myServerTurnCB;
  ArFunctor2C<ArServerModeJogPosition, ArServerClient *, ArNetPacket *> myServerHeadingCB;

  ArServerHandlerCommands *myCustomCommandServer;
  ArFunctor1C<ArServerModeJogPosition, ArArgumentBuilder*> myStringCommandMoveCB;
  ArFunctor1C<ArServerModeJogPosition, ArArgumentBuilder*> myStringCommandTurnCB;
  ArFunctor1C<ArServerModeJogPosition, ArArgumentBuilder*> myStringCommandHeadingCB;
  void stringCmdMove(ArArgumentBuilder* args);
  void stringCmdTurn(ArArgumentBuilder* args);
  void stringCmdHeading(ArArgumentBuilder* args);
};


#endif
