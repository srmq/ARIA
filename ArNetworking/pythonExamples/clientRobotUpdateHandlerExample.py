from AriaPy import *
from ArNetworkingPy import *
import sys

def modeCallback(mode):
  print 'Mode changed'
  print mode

def statusCallback(status):
  print 'Status changed: ', status

Aria.init()
client = ArClientBase()

# You can change the hostname or ip address below to connect to a removve # server:
if not client.blockingConnect("localhost", 7272):
  print "Could not connect to server at localhost port 7272, exiting"
  Aria.exit(1);
client.runAsync()

updates = ArClientHandlerRobotUpdate(client)
updates.addStatusChangedCB(statusCallback)
updates.addModeChangedCB(modeCallback)
updates.requestUpdates()

while True:
  updates.lock()
  print "Mode: %s Status: %s Pos: (%d,%d,%d)" % (updates.getMode(), updates.getStatus(), updates.getX(), updates.getY(), updates.getTh())
  updates.unlock();
  ArUtil.sleep(2000)

Aria.exit(0)
