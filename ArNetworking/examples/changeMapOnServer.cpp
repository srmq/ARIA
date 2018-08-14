#include "Aria.h"
#include "ArNetworking.h"

ArClientBase *client;
ArClientHandlerConfig *configHandler;
bool done = false;

std::string newmapname;

void gotConfig(void)
{
  configHandler->lock();
  ArConfig *newConfig = configHandler->getConfig();
  printf("Got configuration from server. Changing map to %s\n", newmapname.c_str());
  ArConfigSection *s = newConfig->findSection("Files");
  if(!s) return;
  ArConfigArg *arg = s->findParam("Map");
  if(!arg) return;
  arg->setString(newmapname.c_str());
  //char error[512];
  //newConfig->callProcessFileCallbacks(false, error, 512);
  configHandler->unlock();
  puts("Saving...");
  configHandler->saveConfigToServer();
  ArUtil::sleep(10000); // hack todo wait for map changed notification or error.
  done = true;
}

int main(int argc, char **argv)
{
  Aria::init();
  ArGlobalFunctor gotConfigCB(&gotConfig);
  std::string hostname;

  client = new ArClientBase;
  configHandler = new ArClientHandlerConfig(client);

  configHandler->addGotConfigCB(&gotConfigCB);

  ArArgumentParser parser(&argc, argv);
	
  ArClientSimpleConnector clientConnector(&parser);

  parser.loadDefaultArguments();

  /* Check for -help, and unhandled arguments: */
  if (!Aria::parseArgs() || !parser.checkHelpAndWarnUnparsed(1))
  {
    Aria::logOptions();
    exit(0);
  }

  char *val = parser.checkParameterArgument("-map");
  if(val)
  {
    newmapname = val;
    printf("Will set new map name to %s on server.\n", newmapname.c_str());
  }
  else
  {
    puts("Error: must specify new map file name with -map argument.\nExample:\n\t./changeMapOnServer -host 10.0.126.32 -map columbia.map");
    Aria::exit(-5);
    return -5;
  }


  /* Connect our client object to the remote server: */
  if (!clientConnector.connectClient(client))
  {
    puts("Error conncting to server. Use -host argument to specify host address.");
    Aria::exit(-4);
    return -4;
  } 

  puts("Connected to server.");

  client->setRobotName(client->getHost()); // include server hostname in log messages


  puts("Requesting current configuration...");
  configHandler->requestConfigFromServer();
  client->runAsync();

  while (!done)
    ArUtil::sleep(100);
  
  Aria::exit(0);
}
