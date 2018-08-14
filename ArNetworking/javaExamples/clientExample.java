
import com.mobilerobots.Aria.*;
import com.mobilerobots.ArNetworking.*;


class MapUpdatedHandler  extends ArFunctor_NetPacket
{
  public void invoke(ArNetPacket p)
  {
    System.out.println("Map was changed on server!");
  }
}

public class clientExample {

  /* This loads all the ArNetworking classes (they will be in the global
   * namespace) when this class is loaded: */
  static {
    try {
        System.loadLibrary("AriaJava");
        System.loadLibrary("ArNetworkingJava");
    } catch (UnsatisfiedLinkError e) {
      System.err.println("Native code libraries (AriaJava and ArNetworkingJava .so or .DLL) failed to load. See the chapter on Dynamic Linking Problems in the SWIG Java documentation for help.\n" + e);
      System.exit(1);
    }
  }

  /* Main program: */
  public static void main(String argv[]) {
    System.out.println("Starting Java ArNetworking Test");

    /* Global Aria class inititalizaton */
    Aria.init(Aria.SigHandleMethod.SIGHANDLE_THREAD, true);


    ArClientBase client = new ArClientBase();

    if (!client.blockingConnect("localhost", 7272)) // change hostname or ip address here to connect to a remote host
    {
      System.err.println("Error: Could not connect to server on localhost, exiting.");
      System.exit(1);
    }    

    
    client.runAsync();
    System.out.println("Running...");
    
    // Example server request:
    System.out.println("Requesting wander...");
    client.requestOnce("wander");

    // How to create a packet handler to receive a notification if the map is
    // updated:
    client.addHandler("mapUpdated", new MapUpdatedHandler());
    client.requestOnce("checkMap");

    while(client.isConnected())
    {
      ArUtil.sleep(2000);
    }

    Aria.shutdown();
  }
}

