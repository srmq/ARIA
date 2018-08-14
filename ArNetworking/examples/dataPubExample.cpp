#include "Aria.h"
#include "ArNetworking.h"
#include "ArServerDataProvider.h"

class TestProvider
{
  int i;
  ArMutex mutex;
public:
  TestProvider() : i(0) {}
  void lock() { mutex.lock(); }
  void unlock() { mutex.unlock(); }
  int get() { return i; }
  void inc() { ++i; }
};

int main(int argc, char **argv)
{
  Aria::init();
  ArServerBase server;
  if (!server.open(7272))
  {
    printf("Could not open server port\n");
    exit(1);
  }
  server.runAsync();
  TestProvider test;
  ArServerDataProvider<TestProvider, int> testProvider(&server, "test_pub", "test proovider", &test, &TestProvider::get, &TestProvider::lock, &TestProvider::unlock);
  while(server.getRunningWithLock())
  {
    ArUtil::sleep(1000);
    test.lock();
    test.inc();
    test.unlock();
  }
  Aria::exit(0);
  return 0;
}
