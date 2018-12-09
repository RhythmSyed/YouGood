import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.support.annotation.Nullable;
import android.util.Log;
import android.widget.Toast;
import com.facebook.react.bridge.ReactApplicationContext;
import com.facebook.react.bridge.ReactContext; 
import com.facebook.react.bridge.ReactContextBaseJavaModule;
import com.facebook.react.bridge.ReactMethod;
import com.facebook.react.bridge.ReadableMap;
import com.facebook.react.modules.core.DeviceEventManagerModule;
import com.twilio.client.Connection;
import com.twilio.client.ConnectionListener;
import com.twilio.client.Device;
import com.twilio.client.DeviceListener;
import com.twilio.client.PresenceEvent;
import com.twilio.client.Twilio;
import java.io.BufferedReader;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.net.URL;
import java.net.URLConnection;
import java.util.HashMap;
import java.util.Map;

public class TwilioModule extends ReactContextBaseJavaModule implements    ConnectionListener, DeviceListener {

 private ReactContext rContext;
 private Device twilioDevice;
 private Connection connection;
 private Connection pendingConnection;
 private IntentReceiver _receiver;
 private TwilioModule self;
 private String TAG = "CDMS_TWILIO";

 public class IntentReceiver extends BroadcastReceiver {

              private ConnectionListener _cl;

 public IntentReceiver(ConnectionListener connectionListener) {
  this._cl = connectionListener;
}

public void onReceive(Context context, Intent intent) {
  Log.d(TAG,"onReceive method called");
  pendingConnection =        
 (Connection)intent.getParcelableExtra("com.twilio.client.Connection");
  pendingConnection.setConnectionListener(this._cl);
  pendingConnection.accept();
  connection = pendingConnection;
  pendingConnection = null;
  sendEvent("deviceDidReceiveIncoming", null);
}
 }

public TwilioModule(ReactApplicationContext reactContext) {
super(reactContext);
Log.d(TAG,"TwilioModule constructor called");
rContext = reactContext;
this.rContext = reactContext;
self = this;
this._receiver = new IntentReceiver(this);
IntentFilter intentFilter = new IntentFilter();
intentFilter.addAction("com.rogchap.react.modules.twilio.incoming");
this.rContext.registerReceiver(this._receiver, intentFilter);
}

private void sendEvent(String eventName, @Nullable Map<String, String> params) {

if (eventName.equals("connectionDidDisconnect")) {
  //Log.e("mytag", "not emitting an event, just dereferncing the DeviceEventEmitter");
  rContext.getJSModule(DeviceEventManagerModule.RCTDeviceEventEmitter.class).toString();
  //Log.e("mytag", "DONE");
}
else {
  rContext.getJSModule(DeviceEventManagerModule.RCTDeviceEventEmitter.class).emit(eventName, null);
}
}

@Override
public String getName() {
 return "Twilio";
}

@ReactMethod
public void initWithTokenUrl(String tokenUrl) {
Log.d(TAG,"TwilioModule initWithTokenUrl method called");
StringBuilder sb = new StringBuilder();
try {
  URLConnection conn = new URL(tokenUrl).openConnection();
  InputStream in = conn.getInputStream();
  BufferedReader reader = new BufferedReader(new InputStreamReader(in, "UTF-8"));
  String line = "";
  while ((line = reader.readLine()) != null) {
    sb.append(line);
  }
} catch (Exception e) {
}
initWithToken(sb.toString());
}

@ReactMethod
public void initWithToken(final String token) {
Log.d(TAG,"TwilioModule initWithToken method called, token = "+token);
if (!Twilio.isInitialized()) {
  Twilio.initialize(rContext, new Twilio.InitListener() {
    @Override
    public void onInitialized() {
      try {
        if (twilioDevice == null) {
          twilioDevice = Twilio.createDevice(token, self);
          if (twilioDevice!=null){
            Log.d(TAG,"twilioDevice is available");
          }
          else{
            Log.d(TAG,"twilioDevice is null");
          }
          Intent intent = new Intent();
          intent.setAction("com.rogchap.react.modules.twilio.incoming");
          PendingIntent pi = PendingIntent.getBroadcast(rContext, 0, intent, 0);
          twilioDevice.setIncomingIntent(pi);
        }
      } catch (Exception e) {
      }
    }

    @Override
    public void onError(Exception e) {
      Log.d(TAG, e.toString() + "Twilio initilization failed");
    }
  });
}
}

@ReactMethod
public void connect(ReadableMap par) {
Log.d(TAG,"twilioDevice connect");
String contact = "";
Map<String, String> params = new HashMap<String, String>();
contact = par.getString("To").trim();
params.put("To", contact);

// Create an outgoing connection
if (twilioDevice != null) {
  connection = twilioDevice.connect(params, self);
}
else {
  Log.d(TAG,"twilioDevice is null");
}
}

@ReactMethod
public void disconnect() {
Log.d(TAG,"disconnect method called");
if (connection != null) {
  connection.disconnect();
  connection = null;
}
}

@ReactMethod
public void accept() {
Log.d(TAG,"accept method called");
}

@ReactMethod
public void reject() {
  Log.d(TAG,"reject method called");
  pendingConnection.reject();
}

@ReactMethod
public void ignore() {
  Log.d(TAG,"ignore method called");
  pendingConnection.ignore();
}

@ReactMethod
public void setMuted(Boolean isMuted) {
Log.d(TAG,"setMuted method called");
if (connection != null && connection.getState() == Connection.State.CONNECTED) {
  connection.setMuted(isMuted);
}
}

/* ConnectionListener */

@Override
public void onConnecting(Connection connection) {
Log.d(TAG,"onConnecting method called");
sendEvent("connectionDidStartConnecting", null);
}

@Override
public void onConnected(Connection connection) {
Log.d(TAG,"onConnected method called");
sendEvent("connectionDidConnect", null);
}

@Override
public void onDisconnected(Connection connection) {
Log.d(TAG,"onDisconnected method called");
if (connection == connection) {
  connection = null;
}
if (connection == pendingConnection) {
  pendingConnection = null;
}
sendEvent("connectionDidDisconnect", null);
}

@Override
public void onDisconnected(Connection connection, int errorCode, String errorMessage) {
Log.d(TAG,"onDisconnected method with error called");
Map errors = new HashMap();
errors.put("err", errorMessage);
sendEvent("connectionDidFail", errors);
}

/* DeviceListener */
@Override
public void onStartListening(Device device) {
Log.d(TAG,"onStartListening method called");
this.sendEvent("deviceDidStartListening", null);
}

@Override
public void onStopListening(Device device) {
Log.d(TAG,"onStopListening method called");
}

@Override
public void onStopListening(Device inDevice, int inErrorCode, String inErrorMessage) {
Log.d(TAG,"onStopListening method with error code called");
}

@Override
public boolean receivePresenceEvents(Device device) {
Log.d(TAG,"receivePresenceEvents method called");
return false;
}

@Override
public void onPresenceChanged(Device inDevice, PresenceEvent  inPresenceEvent) {
Log.d(TAG,"onPresenceChanged method called");
}
}