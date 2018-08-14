#include "Aria.h"
#include "ArExport.h"
#include "ArServerModeTestLoop.h"

AREXPORT ArServerModeTestLoop::ArServerModeTestLoop(ArServerBase *server, ArRobot *robot, ArConfig *config, ArServerHandlerCommands *cmds) : 
  ArServerMode(robot, server, "test loop"),
  myNetTestLoopCB(this, &ArServerModeTestLoop::netTestLoop),
  myPadding(300), myStopDist(1500), mySpeed(300), myDriveDist(5000), myRotSpeed(50),
  myRotAccel(0), myRotDecel(0), myObsDecel(0),
  myState(Deactivated),
  myActivateCB(this, &ArServerModeTestLoop::activate)
{
  myMode = "TestLoop";
  if (myServer != NULL)
  {
    addModeData("testloop", "makes the robot perform a test loop consisting of only velocity and 180 deg heaoding commands", &myNetTestLoopCB,
		"none", "none", "Movement", "RETURN_NONE");
  }
  if(config != NULL)
    addToConfig(config);
  if(cmds != NULL)
    addCommands(cmds);
}

AREXPORT void ArServerModeTestLoop::addToConfig(ArConfig *config)
{
  config->addParam(ArConfigArg("Speed", &mySpeed, "Speed to drive forward (mm/s)", 1, 1000), "Test Loop Mode");
  config->addParam(ArConfigArg("RotationSpeed", &myRotSpeed, "Rotational speed when turning at end of line. (deg/s). If 0, don't change.", 0, 400), "Test Loop Mode");
  config->addParam(ArConfigArg("RotationAccel", &myRotAccel, "Rotational acceleration when turning at end of line. (deg/s). If 0, don't change.", 0, 400), "Test Loop Mode");
  config->addParam(ArConfigArg("RotationDecel", &myRotAccel, "Rotational deceleration when turning at end of line. (deg/s). If 0, don't change.", 0, 400), "Test Loop Mode");
  config->addParam(ArConfigArg("Distance", &myDriveDist, "Distance to drive forward (mm)", 0, 30000), "Test Loop Mode");
  config->addParam(ArConfigArg("ObstacleStopDist", &myStopDist, "Stop if a sensor returns at or less than this distance"), "Test Loop Mode");
  config->addParam(ArConfigArg("ObstacleCheckPadding", &myPadding, "Check for sensors within robot width plus this padding on each side, and within twice robot length."), "Test Loop Mode");
  config->addParam(ArConfigArg("ObstacleDecel", &myObsDecel, "If not 0, set this deceleration when stopping due to obstacle. If 0, don't change.", 0, 400), "Test Loop Mode");
}

AREXPORT ArServerModeTestLoop::~ArServerModeTestLoop()
{

}

AREXPORT void ArServerModeTestLoop::activate(void)
{
  if (!baseActivate())
    return;

  setActivityTimeToNow();
  myRobot->clearDirectMotion();
  myStatus = "Test Loop";
  myStartPos = myRobot->getPose();
  myState = Trans;
}

AREXPORT void ArServerModeTestLoop::deactivate(void)
{
  myState = Deactivated;
  myRobot->stop();
  baseDeactivate();
}

void ArServerModeTestLoop::netTestLoop(ArServerClient *client, 
				     ArNetPacket *packet)
{
  myRobot->lock();
  ArLog::log(ArLog::Verbose, "Activating");
  activate();
  myRobot->unlock();
  setActivityTimeToNow();
}

void ArServerModeTestLoop::userTask(void)
{
  if(!isActive()) return;
  setActivityTimeToNow();
  if(myRobot->getRangeDeviceList()->size() == 0)
  {
    myRobot->stop();
    myStatus = "No range sensing devices! Will not run. (See program log.)";
    return;
  }
  const double rw = myRobot->getRobotWidth();
  const double rl = myRobot->getRobotLength();
  if(myRobot->checkRangeDevicesCurrentBox((-rw/2)-myPadding, 0, (rw/2)+myPadding, rl*2) <= myStopDist)
  {
    if(myObsDecel != 0) myRobot->setTransDecel(myObsDecel);
    myRobot->stop();
    myStatus = "Stopping due to sensing";
    ArLog::log(ArLog::Terse, "ArServerModeTestLoop: Stopping due to obstacle within %.2fm", myStopDist/1000.0);
    return;
  }
  if(myState == Trans)
  {
    if(myRobot->getPose().findDistanceTo(myStartPos) > myDriveDist)
    {
      myState = Stop;
      myStatus = "Stopping";
      myRobot->stop();
      return;
    }
    else
    {
      myStatus = "Driving forward";
      myRobot->setVel(mySpeed);
      return;
    }
  }
  else if(myState == Stop)
  {
    if(myRobot->isStopped())
    {
      myState = Rot;
      myStatus = "Turning around";
      if(myRotAccel != 0) myRobot->setRotAccel(myRotAccel);
      if(myRotDecel != 0) myRobot->setRotDecel(myRotDecel);
      if(myRotSpeed != 0) myRobot->setRotVelMax(myRotSpeed);
      myRobot->setDeltaHeading(180);
      return;
    }
  }
  else if(myState == Rot)
  {
    if(myRobot->isHeadingDone())
    {
      myStatus = "Driving forward";
      myState = Trans;
      myStartPos = myRobot->getPose();
      return;
    }
  }
}

AREXPORT void ArServerModeTestLoop::addCommands(ArServerHandlerCommands *cmds)
{
   cmds->addCommand("TestLoop:Activate", "Start simple test loop mode", &myActivateCB);
}

