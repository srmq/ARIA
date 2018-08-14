
#include "Aria.h"
#include "ArNetworking.h"


/** @example simpleServerExample.cpp This is a simple example of an ArNetworking server.
 * This is the simplest possible ArNetworking server.  It creates the server but
 * does not try to connect to the robot or any other
 * devices, and provides no services.
 *
 * For more complete servers that connect to the robot, see
 * simpleRobotServerExample.cpp and  serverDemo.cpp.
 */

int main(int argc, char **argv)
{
  Aria::init();
  ArArgumentParser parser(&argc, argv);

  // The base server object, manages all connections to clients.
  ArServerBase server;

  // This object simplifies configuration and opening of the ArServerBase
  // object.
  ArServerSimpleOpener simpleOpener(&parser);

  // parse the command line. fail and print the help if the parsing fails
  // or if the help was requested with -help
  parser.loadDefaultArguments();
  if (!simpleOpener.parseArgs() || !parser.checkHelpAndWarnUnparsed())
  {    
    simpleOpener.logOptions();
    Aria::exit(1);
  }

  // Use the ArSimpleOpener to open the server port
  if (!simpleOpener.open(&server))
  {
    ArLog::log(ArLog::Terse, "Error: Could not open server on port %d", simpleOpener.getPort());
    exit(1);
  }



  // The simple opener might have information to display right before starting 
  // the server thread:
  simpleOpener.checkAndLog();

  // now let the server base run in a new thread, accepting client connections.
  server.runAsync();

  ArLog::log(ArLog::Normal, "Server is now running... Press Ctrl-C to exit.");


  while(true)
    ArUtil::sleep(500);
  Aria::exit(0);
}


