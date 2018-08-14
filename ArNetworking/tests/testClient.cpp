#include "Aria.h"
#include "ArNetworking.h"

ArClientBase *ClientP = 0;

void test(ArNetPacket *packet)
{
  printf("got packet %d (%s)\n", packet->getCommand(), ClientP->getName(packet->getCommand()));
}

int main(int argc, char **argv)
{
  ArClientBase client;
  ClientP = &client;
  ArGlobalFunctor1<ArNetPacket *> testCB(&test);

  Aria::init();
  //ArLog::init(ArLog::StdOut, ArLog::Verbose);
  ArTime startTime;
  startTime.setToNow();
  if (!client.blockingConnect("localhost", 7273))
  {
    printf("Could not connect to server, exiting\n");
    exit(1);
  }    
  printf("Took %ld msec to connect\n", startTime.mSecSince());
  
  client.runAsync();
  
  client.lock();
  client.addHandler("test", &testCB);
  client.addHandler("test2", &testCB);
  client.addHandler("test3", &testCB);
  client.logDataList();
  
  puts("requestOnce for test");
  client.requestOnce("test");
  puts("request for test2 interval 100ms");
  client.request("test2", 100);
  puts("request for test3 no interval, push");
  client.request("test3", -1);
  client.unlock();
  ArUtil::sleep(1000);
  printf("Changing speed\n");
  client.lock();
  puts("request for test2 interval 300ms");
  client.request("test2", 300);
  client.unlock();
  ArUtil::sleep(1000);
  client.lock();
  puts("requestStop test2");
  client.requestStop("test2");
  client.unlock();
  
  ArUtil::sleep(1000);
  puts("disconnecting");
  client.lock();
  client.disconnect();
  client.unlock();
  ArUtil::sleep(50);
  exit(0);
}
