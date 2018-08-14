
#include "Aria.h"
#include "ArNetworking.h"
#include "ArClientDataReceiver.h"

class TestTarget {
public:
    void printInt(const int& i) { printf("Subscriber got %d\n", i); }
};

int main(int argc, char **argv)
{
  Aria::init();
  ArClientBase client;
  ArArgumentParser parser(&argc, argv);
  ArClientSimpleConnector clientConnector(&parser);

  /* Check command-line parameters: */
  parser.loadDefaultArguments();
  if (!Aria::parseArgs() || !parser.checkHelpAndWarnUnparsed())
  {
    Aria::logOptions();
    exit(0);
  }

  /* Connect to server: */
  if (!clientConnector.connectClient(&client))
  {
    if (client.wasRejected())
      printf("Server '%s' rejected connection, exiting\n", client.getHost());
    else
      printf("Could not connect to server '%s', exiting\n", client.getHost());
    exit(1);
  } 
  printf("Connected to server.\n");
  client.setRobotName(client.getHost()); // include server name in log messages


  /* Request "test_pub" data from the server.  This data will be
   * automatically requested every 1000ms, and when any response is received, then
   * the printInt() method of the 'test' object will be called. 
   * The template parameters <TestTarget, int> indicate the type of the target
   * object, and the type of the value to be obtained from the responses from
   * the server and passed to the target method printInt() as an argument (by
   * reference).  For this to work, the server must have "test_pub" data
   * available, and responses must include values of the correct type 
   * (integer).
   * See the definition of the TestTarget class above.
  */
  TestTarget test;
  ArClientDataReceiver<TestTarget, int> receiver(&client, "test_pub", &test, &TestTarget::printInt, 1000);

  /* Begin communicating with the server. run() will exit if disconnected for
   * any reason. */
  client.run();
  client.disconnect();
  Aria::exit(0);
}
