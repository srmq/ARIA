
#include "Aria.h"
#include "ArNetworking.h"
#include <string>
#include <assert.h>

// Test object providing values via accessor methods
class Test
{
public:
  int getInt()
  {
    return 23;
  }

  float getFloat()
  {
    return 3.14159;
  }

  double getDouble()
  {
    return 3.14159;
  }

  bool getBool()
  {
    return false;
  }

  char getChar()
  {
    return 255;
  }

  unsigned char getUChar()
  {
    return 42;
  }

  unsigned int getUInt()
  {
    return 23;
  }

  short getShort()
  {
    return 23;
  }

  unsigned short getUShort()
  {
    return 23;
  }

  std::string getString()
  {
    return "Hello world";
  }
};


int packetcounter = 0;

void handleIntPacket(ArNetPacket *pkt)
{
  printf("Got test_int packet with: %d\n", (int) pkt->bufToByte4());
  ++packetcounter;
}


void handleFloatPacket(ArNetPacket *pkt)
{
  printf("Got test_float packet with: %f\n", (float) pkt->bufToByte4() / 1000.0);
  ++packetcounter;
}


int main(int argc, char **argv)
{
  Aria::init();
  ArServerBase server;

  //server.addData("test", "some wierd test", &testCB, "none", "none");
  //server.addData("test2", "another wierd test", &testCB, "none", "none");
  //server.addData("test3", "yet another wierd test", &testCB, "none", "none");

  Test test;

  int i = 23;
  bool r = false;

  r = server.addDataRequest<int>("test_int_1", "test int", &i);
  assert(r);
  r = server.addDataRequest<int>("test_int_2", "test int", new ArRetFunctorC<int, Test>(&test, &Test::getInt));
  assert(r);

  float f = 3.14159;
  r = server.addDataRequest<float>("test_float_1", "test float", &f);
  assert(r);
  r = server.addDataRequest<float>("test_float_2", "test float from func", new ArRetFunctorC<float, Test>(&test, &Test::getFloat));
  assert(r);

  if (!server.open(7279))
  {
    printf("Could not open server on port 7279\n");
    Aria::exit(1);
  }
  printf("Starting server at localhost:7279\n");
  server.runAsync();

  ArClientBase client;
  if(!client.blockingConnect("localhost", 7279))
  {
    printf("could not connect to our test server (localhost:7279)\n");
    Aria::exit(1);
  }

  client.addHandler("test_int_1", new ArGlobalFunctor1<ArNetPacket*>(&handleIntPacket));
  client.request("test_int_1", 1000);
  client.addHandler("test_int_2", new ArGlobalFunctor1<ArNetPacket*>(&handleIntPacket));
  client.request("test_int_2", 1000);
  client.addHandler("test_float_1", new ArGlobalFunctor1<ArNetPacket*>(&handleFloatPacket));
  client.request("test_float_1", 1000);
  client.addHandler("test_float_2", new ArGlobalFunctor1<ArNetPacket*>(&handleFloatPacket));
  client.request("test_float_2", 1000);

  client.runAsync();
  
//  while (server.getRunningWithLock())
//  {
//    ArUtil::sleep(1000);
//    //server.broadcastPacketTcp(&packet, "test3");
//  }

  ArUtil::sleep(30000);

  assert(packetcounter >= 4);

  Aria::exit(0);
}
