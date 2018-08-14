
/** @example robotUpdateExample.cpp Client that monitors status, mode and other information from a server.
 * This example shows how to use ArClientHandlerRobotUpdate to receive updates
 * on essential robot state data: position, speed, current server mode and
 * status strings, etc.   You can use this to detect when the server switches
 * modes, or its status changes such as arnlServer arriving at a goal (or
 * failing), etc.   See @ref RemoteRequestAPI in the ArNetworking API reference
 * documentation (or docs/RemoteRequestAPI.html) 
 * for a list of mode and status
 * names, as well as ARNL or MOGS documentation if you are using ARNL or MOGS.
 */

#include "Aria.h"
#include "ArNetworking.h"
#include "ArClientHandlerRobotUpdate.h"


void statusChanged(const char *mode, const char *status)
{
  printf("                                                                                    \r");
  printf("status changed! mode=%s, status=%s\n", mode, status);
}


int main(int argc, char **argv)
{
  Aria::init();
  ArClientBase client;
  ArArgumentParser parser(&argc, argv);

  /* This will be used to connect our client to the server. 
   * It will get the hostname from the -host command line argument: */
  ArClientSimpleConnector clientConnector(&parser);

  parser.loadDefaultArguments();

  /* Check for -host, -help, ARIA arguments, and unhandled arguments: */
  if (!Aria::parseArgs() || !parser.checkHelpAndWarnUnparsed())
  {
    Aria::logOptions();
    Aria::exit(0);
  }

  
  /* Connect our client object to the remote server: */
  if (!clientConnector.connectClient(&client))
  {
    if (client.wasRejected())
      printf("Server '%s' rejected connection, exiting\n", client.getHost());
    else
      printf("Could not connect to server '%s', exiting\n", client.getHost());
    Aria::exit(1);
  } 

  printf("Connected to server.\n");

  client.setRobotName(client.getHost()); // include server name in log messages

  client.runAsync();

 
  ArClientHandlerRobotUpdate updates(&client);
  updates.addStatusChangedCB(new ArGlobalFunctor2<const char*, const char*>(&statusChanged));

  updates.requestUpdates();

  while (client.getRunningWithLock())
  {
    updates.lock();
    printf("Mode:%s  Status:%s  Pos:%.0f,%.0f,%.0f  Vel:%.0f,%.0f,%.0f  Bat:%.1f  \r",
      updates.getMode(),
      updates.getStatus(),
      updates.getX(), updates.getY(), updates.getTh(),
      updates.getVel(), updates.getLatVel(), updates.getRotVel(),
      updates.getVoltage()
    );
    updates.unlock();
    fflush(stdout);
    ArUtil::sleep(1000);
  }

  /* The client stopped running, due to disconnection from the server, general
   * Aria shutdown, or some other reason. */
  client.disconnect();
  Aria::exit(0);
  return 0;
}
