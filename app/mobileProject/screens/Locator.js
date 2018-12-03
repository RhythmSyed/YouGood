import React, { Component } from 'react';
import { PermissionsAndroid, View, Text, StyleSheet, TouchableOpacity, TouchableHighlight, Button, Alert, Animated } from 'react-native';
import MapView, { PROVIDER_GOOGLE } from 'react-native-maps';
import awsIot from 'aws-iot-device-sdk';
var AWS = require('aws-sdk');

const device = awsIot.device({
    host: "a2pv8pigeo9wbh-ats.iot.us-east-2.amazonaws.com",
    clientId: Math.random(),
    protocol: "wss",
    accessKeyId: "AKIAJBDBZHG3VXU5TLQQ",
    secretKey: "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"
});

device.on('connect', () => {
  device.subscribe('user_location')
})

device.on('message', (topic, message) => {
  console.log(topic, message.toString())  
  Alert.alert(
    'Alert Title',
    'My Alert Msg',
    [
      {text: 'Ask me later', onPress: () => console.log('Ask me later pressed')},
      {text: 'Cancel', onPress: () => console.log('Cancel Pressed'), style: 'cancel'},
      {text: 'OK', onPress: () => console.log('OK Pressed')},
    ],
    { cancelable: false }
  )
})

device.on('error', error => {
  console.log(error)
})

async function requestUserLocation(){
    try
    {
      const granted = await PermissionsAndroid.request(PermissionsAndroid.PERMISSIONS.ACCESS_FINE_LOCATION,
      {
        'title' : 'Get Current Location',
        'message': 'Allow app to access location services to get your current location'
      })
      
      if (granted === PermissionsAndroid.RESULTS.GRANTED){
        console.log("You can access Location now")
        return true;
      }
      else {
        console.log("Location Access Denied")
        return false;
      }
    } 
    catch(err) {
      console.warn(err)
      return false;
    }
}

class Locator extends Component { 
  state = {
    location_marker: []
  }

  getUserLocation() {
    navigator.geolocation.getCurrentPosition(
      (position) => {
        this.state.location_marker.push({
          title: "location",
          coordinates: {
            latitude: position.coords.latitude,
            longitude: position.coords.longitude
          },
          key: "location",
          description: "(" + position.coords.latitude.toString() + "," + position.coords.longitude.toString() + ")"
        });
        this.setState(this.state)
      }
    )
  }

  render() {
    return (
      <View style={styles.mapContainer} key={'initial'}>
        <Button title="Get User Location" 
          onPress={ () => 
            {
              if(requestUserLocation()){
                  this.getUserLocation();
              }
              else{
                Alert("No")
              }
            }
          }
        />
        
        <MapView
          initialRegion={{
            latitude: 40.42877019,
            longitude: -86.91209748,
            latitudeDelta: 0,
            longitudeDelta: 0
          }}
          style={styles.map}
          onPress={
            (event) => {}
          }
          showsBuildings={true}
          provider={PROVIDER_GOOGLE}
          mapType = "standard"
          showsIndoors = {true}
          loadingEnabled = {true}
        >

        {this.state.location_marker.map(marker => (
          <MapView.Marker 
            coordinate={marker.coordinates}
            title={marker.title}
            key={marker.key}
            description={marker.description}
          />
        ))}
        
        </MapView>          
      </View>
    )
  }

}

export default Locator;



// Style Sheets //

const styles = StyleSheet.create({
  mapContainer: {
      width: '100%',
      height: 550,
      marginTop: 20
  },
  map: {
      width: '100%',
      height: '100%',
      justifyContent: 'center'
      
  },
  mapInstruction: {
    textAlign: 'center',
    color: "#9400d3",
    fontWeight: 'bold',
    fontSize: 18
  }
});