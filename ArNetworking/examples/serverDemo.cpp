
#include "Aria.h"
#include "ArNetworking.h"
#include "ArSonarMTX.h"
#include "ArServerModeJogPosition.h"
#include "ArServerAdvertiser.h"
#include "ArServerModeTestLoop.h"
#include "ArGPSConnector.h"
#include "ArGPS.h"
#include "ArSeekurIMU.h"
#include "ArSimUtil.h"
#include "ArServerInfoTrailDrawing.h"

/** @example serverDemo.cpp Example ArNetworking server providing teleoperation,
 * sonar data, control the camera, etc.
 *
 * This is a basic ArNetworking server. It connects to a robot or simulator,
 * including, if available, IRs, gyro, and bumpers.  Give the option 
 * "-connectLaser" on the command line to enable the laser rangefinder,
 * if available.
 *
 * Run "./serverDemo -help" for a full list of command line options.
 *
 * Once running, connect to this server with a a client such as 
 * MobileEyes.
 *
 * This server provides the following services:
 *  - User login (optional)
 *  - Basic robot telemetry information 
 *  - Range sensor data values (not used by MobileEyes)
 *  - Graphics representing range sensor reading positions
 *  - Teleoperation modes (including safe/unsafe drive modes)
 *  - Wander mode
 *  - Various advanced "custom" commands to control logging, debugging, etc.
 *
 * Note that this program requires a terminal to run -- i.e. you can't run
 * it in the background in Linux.  To modify it to allow that, remove the key
 * handler code in main().
 */


int main(int argc, char **argv)
{
  Aria::init();
  //ArLog::init(ArLog::StdOut, ArLog::Verbose);
  ArArgumentParser parser(&argc, argv);
  parser.loadDefaultArguments();
  ArRobot robot;
  ArRobotConnector robotConnector(&parser, &robot);
  ArAnalogGyro gyro(&robot);
  ArGPSConnector gpsConnector(&parser);
  ArSeekurIMU imu(&robot);

  if (!robotConnector.connectRobot())
  {
    printf("Could not connect to robot... exiting\n");
    Aria::exit(1);
  }

  ArDataLogger dataLogger(&robot, "dataLog.tsv");
  
  ArServerBase server;

  ArLaserConnector laserConnector(&parser, &robot, &robotConnector);
  ArServerSimpleOpener simpleOpener(&parser);


  if (!Aria::parseArgs() || !parser.checkHelpAndWarnUnparsed())
  {    
    Aria::logOptions();
    Aria::exit(1);
  }

  // Set up where we'll look for files such as user/password 
  char fileDir[1024];
  ArUtil::addDirectories(fileDir, sizeof(fileDir), Aria::getDirectory(), 
			 "ArNetworking/examples");

  if (!simpleOpener.open(&server, fileDir, 240))
  {
    if (simpleOpener.wasUserFileBad())
      printf("Bad user/password/permissions file\n");
    else
      printf("Could not open server port\n");
    exit(1);
  }

 
 
  ArSonarDevice sonarDev;
  robot.addRangeDevice(&sonarDev);

  ArIRs irs;
  robot.addRangeDevice(&irs);

  ArBumpers bumpers;
  robot.addRangeDevice(&bumpers);

  // attach services to the server
  ArServerInfoRobot serverInfoRobot(&server, &robot);
  ArServerInfoSensor serverInfoSensor(&server, &robot);
  ArServerInfoDrawings drawings(&server);

  // modes for controlling robot movement
  ArServerModeStop modeStop(&server, &robot);
  ArServerModeRatioDrive modeRatioDrive(&server, &robot);  
  ArServerModeWander modeWander(&server, &robot);
  ArServerModeJogPosition modeJog(&server, &robot);
  modeJog.addToConfig(Aria::getConfig());
  modeStop.addAsDefaultMode();
  modeStop.activate();

  // set up the simple commands
  ArServerHandlerCommands commands(&server);
  ArServerSimpleComUC uCCommands(&commands, &robot);  // send commands directly to microcontroller
  ArServerSimpleComMovementLogging loggingCommands(&commands, &robot); // control debug logging
  ArServerSimpleComGyro gyroCommands(&commands, &robot, &gyro); // configure gyro
  ArServerSimpleComLogRobotConfig configCommands(&commands, &robot); // control more debug logging
  ArServerSimpleServerCommands serverCommands(&commands, &server); // control ArNetworking debug logging
  ArServerSimpleLogRobotDebugPackets logRobotDebugPackets(&commands, &robot, ".");  // debugging tool

  modeJog.addCommands(&commands);

  // ArServerModeDrive is an older drive mode. ArServerModeRatioDrive is newer and generally performs better,
  // but you can use this for old clients if neccesary.
  //ArServerModeDrive modeDrive(&server, &robot);
  //modeDrive.addControlCommands(&commands); // configure the drive modes (e.g. enable/disable safe drive)

  ArServerHandlerConfig serverHandlerConfig(&server, Aria::getConfig()); // make a config handler
  ArLog::addToConfig(Aria::getConfig()); // let people configure logging

  modeRatioDrive.addToConfig(Aria::getConfig(), "Teleop settings"); // able to configure teleop settings
  modeRatioDrive.addControlCommands(&commands);

  commands.addCommand("DataLogger:StopLogging", "Stop logging to data log file.  See configuration options.", dataLogger.getStopLogFunctor());
  commands.addCommand("DataLogger:StartLogging", "Start or resume logging to data log file. See configuration.", dataLogger.getStartLogFunctor());
  commands.addStringCommand("DataLogger:InsertComment", "Write a message to the data log as a comment. Data Logging must be enabled in config.", dataLogger.getWriteCommentFunctor());
  commands.addCommand("DataLogger:ClearLog", "Reset log file, deleting all existing logged data", dataLogger.getClearLogFunctor());
  commands.addCommand("DataLogger:SaveCopy", "Save a copy of the log file with timestamp.", dataLogger.getSaveCopyFunctor()); 
  commands.addStringCommand("DataLogger:SaveCopyAs", "Save a copy of the log file with given name.", dataLogger.getSaveCopyWithArgsFunctor()); 

  // You can use this class to send a set of arbitrary strings 
  // for MobileEyes to display, this is just a small example
  ArServerInfoStrings stringInfo(&server);
  Aria::getInfoGroup()->addAddStringCallback(stringInfo.getAddStringFunctor());
  Aria::getInfoGroup()->addStringInt(
	  "Motor Packet Count", 10, 
	  new ArConstRetFunctorC<int, ArRobot>(&robot, 
					       &ArRobot::getMotorPacCount));
  /*
  Aria::getInfoGroup()->addStringInt(
	  "Laser Packet Count", 10, 
	  new ArRetFunctorC<int, ArSick>(&sick, 
					 &ArSick::getSickPacCount));
  */

  ArServerInfoTrailDrawing trail("RobotPose", new ArConstRetFunctorC<ArPose, ArRobot>(&robot, &ArRobot::getPose), 200,
      ArDrawingData("polyDots", ArColor(100,255,100), 80, 64), 
      drawings, 5, 50);
  robot.addSensorInterpTask("trail", 10, trail.getPullFunc());

  ArGPS *gps = gpsConnector.createGPS(&robot);
  ArGPSCoordConverter gpsconverter(gps);
  if(gps && gps->connect())
  {
    ArLog::log(ArLog::Normal, "serverDemo: Connected to GPS");
    dataLogger.addData<double>("Latitude", new ArConstRetFunctorC<double, ArGPS>(gps, &ArGPS::getLatitude), "%0.4f", 12);
    dataLogger.addData<double>("Longitude", new ArConstRetFunctorC<double, ArGPS>(gps, &ArGPS::getLongitude), "%0.4f", 12);
    dataLogger.addData<const char*>("GPS Fix", new ArConstRetFunctorC<const char*, ArGPS>(gps, &ArGPS::getFixTypeName), "%s", 12);
    dataLogger.addData<double>("GPS HDOP", new ArConstRetFunctorC<double, ArGPS>(gps, &ArGPS::getHDOP), "%0.4f", 12);
    dataLogger.addData<Ar3DPoint>("Pos. Converted From GPS", gpsconverter.getCurrentFunc(), "(%0.4f,%0.4f,%0.4f)", 32);
    dataLogger.addData<ArTime>("GPS Pos Timestamp", new ArConstRetFunctorC<ArTime, ArGPS>(gps, &ArGPS::getGPSPositionTimestamp), "%lu.%lu", 16);

    ArServerInfoTrailDrawing *gpsTrail = new ArServerInfoTrailDrawing("gpspoints", gpsconverter.getCurrent2DPoseFunc(), 300,
      ArDrawingData("polyDots", ArColor(100,100,255), 100, 63), 
      drawings, 5, 50);
    robot.addSensorInterpTask("gpspoints", 20, gpsTrail->getPullFunc());

    Aria::getInfoGroup()->addStringDouble("GPS Latitude", 10, new ArConstRetFunctorC<double, ArGPS>(gps, &ArGPS::getLatitude));
    Aria::getInfoGroup()->addStringDouble("GPS Longitude", 10, new ArConstRetFunctorC<double, ArGPS>(gps, &ArGPS::getLongitude));
    Aria::getInfoGroup()->addStringString("GPS Fix", 10, new ArConstRetFunctorC<const char*, ArGPS>(gps, &ArGPS::getFixTypeName));
    Aria::getInfoGroup()->addStringDouble("GPS HDOP", 10, new ArConstRetFunctorC<double, ArGPS>(gps, &ArGPS::getHDOP));
  }



  if(robotConnector.getRemoteIsSim())
  {
    ArLog::log(ArLog::Normal, "Connected to simulator, simulator info will be available for display and logging.");
    ArSimUtil *sim = new ArSimUtil(&robot);
    Aria::getInfoGroup()->addStringDouble("Sim True X", 8, 
      new ArRetFunctorC<double, ArSimUtil>(sim, &ArSimUtil::getSimTrueX));
    Aria::getInfoGroup()->addStringDouble("Sim True Y", 8, 
      new ArRetFunctorC<double, ArSimUtil>(sim, &ArSimUtil::getSimTrueY)); 
    Aria::getInfoGroup()->addStringDouble("Sim True Th", 6, 
      new ArRetFunctorC<double, ArSimUtil>(sim, &ArSimUtil::getSimTrueTh));

    ArServerInfoTrailDrawing *trail  = new ArServerInfoTrailDrawing(
      "SimTruePose", sim->getSimTruePoseFunc(), 100,
      ArDrawingData("polyDots", ArColor(255, 178, 63), 180, 60), // type, color, size, layer -- orange dots bigger and lower than MOGS trails
      drawings,
      5 // poll position every 5 robot cycles (500ms)
    );
    robot.addSensorInterpTask("SimTruePoseDrawing", 20, trail->getPullFunc());
  }


  Aria::getInfoGroup()->addString("Data Log Status", 24, new ArFunctor2C<ArDataLogger, char*, ArTypes::UByte2>(&dataLogger, &ArDataLogger::getStatus));
  Aria::getInfoGroup()->addStringUnsignedLong("Data Log Disk Free Space", 10, new ArRetFunctorC<unsigned long, ArDataLogger>(&dataLogger, &ArDataLogger::getAvailableDiskSpaceMB), "%lu MB");

  Aria::getInfoGroup()->addStringDouble("IMU Yaw Speed", 10, new ArConstRetFunctorC<double, ArSeekurIMU>(&imu, &ArSeekurIMU::getYawSpeed));
  Aria::getInfoGroup()->addStringDouble("IMU Yaw Pos", 10, new ArConstRetFunctorC<double, ArSeekurIMU>(&imu, &ArSeekurIMU::getYawPos));
  Aria::getInfoGroup()->addStringDouble("IMU Roll Speed", 10, new ArConstRetFunctorC<double, ArSeekurIMU>(&imu, &ArSeekurIMU::getRollSpeed));
  Aria::getInfoGroup()->addStringDouble("IMU Roll Pos", 10, new ArConstRetFunctorC<double, ArSeekurIMU>(&imu, &ArSeekurIMU::getRollPos));
  Aria::getInfoGroup()->addStringDouble("IMU Pitch Speed", 10, new ArConstRetFunctorC<double, ArSeekurIMU>(&imu, &ArSeekurIMU::getPitchSpeed));
  Aria::getInfoGroup()->addStringDouble("IMU Pitch Pos", 10, new ArConstRetFunctorC<double, ArSeekurIMU>(&imu, &ArSeekurIMU::getPitchPos));

  Aria::getInfoGroup()->addStringDouble("IMU Accel X", 10, new ArConstRetFunctorC<double, ArSeekurIMU>(&imu, &ArSeekurIMU::getXAccel));
  Aria::getInfoGroup()->addStringDouble("IMU Accel Y", 10, new ArConstRetFunctorC<double, ArSeekurIMU>(&imu, &ArSeekurIMU::getYAccel));
  Aria::getInfoGroup()->addStringDouble("IMU Accel Z", 10, new ArConstRetFunctorC<double, ArSeekurIMU>(&imu, &ArSeekurIMU::getZAccel));

  Aria::getInfoGroup()->addStringFloat("IMU Avg Temp", 10, new ArConstRetFunctorC<float, ArSeekurIMU>(&imu, &ArSeekurIMU::getTemperature));

  dataLogger.addData<double>("IMU Yaw Speed (deg/s)", new ArConstRetFunctorC<double, ArSeekurIMU>(&imu, &ArSeekurIMU::getYawSpeed), "%.2f");
  dataLogger.addData<double>("IMU Yaw Pos (deg)", new ArConstRetFunctorC<double, ArSeekurIMU>(&imu, &ArSeekurIMU::getYawPos), "%.2f");
  dataLogger.addData<double>("IMU Roll Speed (deg/s)", new ArConstRetFunctorC<double, ArSeekurIMU>(&imu, &ArSeekurIMU::getRollSpeed), "%.2f");
  dataLogger.addData<double>("IMU Roll Pos (deg)", new ArConstRetFunctorC<double, ArSeekurIMU>(&imu, &ArSeekurIMU::getRollPos), "%.2f");
  dataLogger.addData<double>("IMU Pitch Speed (deg/s)", new ArConstRetFunctorC<double, ArSeekurIMU>(&imu, &ArSeekurIMU::getPitchSpeed), "%.2f");
  dataLogger.addData<double>("IMU Pitch Pos (deg)", new ArConstRetFunctorC<double, ArSeekurIMU>(&imu, &ArSeekurIMU::getPitchPos), "%.2f");
  dataLogger.addData<double>("IMU Accel X", new ArConstRetFunctorC<double, ArSeekurIMU>(&imu, &ArSeekurIMU::getXAccel), "%.2f");
  dataLogger.addData<double>("IMU Accel Y", new ArConstRetFunctorC<double, ArSeekurIMU>(&imu, &ArSeekurIMU::getYAccel), "%.2f");
  dataLogger.addData<double>("IMU Accel Z", new ArConstRetFunctorC<double, ArSeekurIMU>(&imu, &ArSeekurIMU::getZAccel), "%.2f");
  dataLogger.addData<float>("IMU Avg Temp (deg C)",  new ArConstRetFunctorC<float, ArSeekurIMU>(&imu, &ArSeekurIMU::getTemperature), "%.2f");

  Aria::getInfoGroup()->addStringUnsignedLong("Log Disk Free Space", 10, new ArGlobalRetFunctor<unsigned long>(&ArLog::getAvailableDiskSpaceMB), "%lu MB");

  dataLogger.addToConfig(Aria::getConfig());

  
  ArServerModeTestLoop testLoop(&server, &robot, Aria::getConfig(), &commands);

  
  // start the robot running, true means that if we lose connection the run thread stops
  robot.runAsync(true);


  // connect the laser(s) if it was requested
  if (!laserConnector.connectLasers())
  {
    ArLog::log(ArLog::Terse, "serverDemo: Warning: Could not connect to lasers.  Will continue but without laser sensing!");
    //Aria::exit(2);
  }
  

  drawings.addRobotsRangeDevices(&robot);

  // log whatever we wanted to before the runAsync
  simpleOpener.checkAndLog();


  ArServerAdv adv(&server, "serverDemo", "Simple ArNetworking Server Example");

  // now let it spin off in its own thread
  server.runAsync();


  // Uncomment code below to add a key handler so that you can exit by pressing
  // escape. Note that a key handler prevents you from running
  // a program in the background on Linux, since it expects an 
  // active terminal to read keys from; remove this if you want
  // to run it in the background.
/*
  ArKeyHandler *keyHandler;
  if ((keyHandler = Aria::getKeyHandler()) == NULL)
  {
    keyHandler = new ArKeyHandler;
    Aria::setKeyHandler(keyHandler);
    robot.lock();
    robot.attachKeyHandler(keyHandler);
    robot.unlock();
    printf("To exit, press escape.\n");
  }
*/

  // Read in parameter files.

  std::string configFile = "serverDemoConfig.txt";
  Aria::getConfig()->setBaseDirectory("./");

  ArLog::log(ArLog::Normal, "Loading config file %s%s...", Aria::getConfig()->getBaseDirectory(), configFile.c_str());
  if (Aria::getConfig()->parseFile(configFile.c_str(), true, true))
  {
    ArLog::log(ArLog::Normal, "Sucessfully loaded config file %s", configFile.c_str());
  }
  else
  {
    if (ArUtil::findFile(configFile.c_str()))
    {
      ArLog::log(ArLog::Normal, 
		 "Warning: Errors loading or parsing configuration file %s%s, continuing anyway",
     Aria::getConfig()->getBaseDirectory(),
		 configFile.c_str());
    }
    else
    {
      ArLog::log(ArLog::Normal, "No configuration file %s%s, creating.", Aria::getConfig()->getBaseDirectory(), configFile.c_str());
      Aria::getConfig()->writeFile(configFile.c_str());
    }
  }


  robot.lock();
  robot.enableMotors();
  robot.unlock();

  ArLog::log(ArLog::Normal, "serverDemo: Server is now running. Connect to %s with MobileEyes, clientDemo or another client.", server.getOpenOnIP()?server.getOpenOnIP():"this host");

  robot.waitForRunExit();
  Aria::exit(0);
}


