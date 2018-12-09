import React, { Component } from 'react';
import { StyleSheet, Text, View, Button, Alert } from 'react-native';
import awsIot from 'aws-iot-device-sdk';

const device = awsIot.device({
    host: "a2pv8pigeo9wbh-ats.iot.us-east-2.amazonaws.com",
    clientId: Math.random(),
    protocol: "wss",
    accessKeyId: "AKIAJBDBZHG3VXU5TLQQ",
    secretKey: "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"
});

class Splash extends Component {
    render() {
        device.on('connect', () => {
            device.subscribe('upload_topic')
        })

        device.on('message', (topic, message) => {
            console.log(topic, message.toString())
            this.props.navigation.navigate('Countdown')
        })
          
        device.on('error', error => {
            console.log(error)
        })

        return (
            <View style={styles.container}>
                <Text style={styles.title}>YouGood? Fall Detector</Text>
            </View>
        );
    }
}

export default Splash;


const styles = StyleSheet.create({
    container: {
        backgroundColor: 'white',
        flex: 1,
        alignItems: 'center',
        justifyContent: 'center',
    },
    title: {
        fontWeight: 'bold',
        fontSize: 18,
        color: 'black',
        marginBottom: 20
    },
    button: {
        marginBottom: 10
    }
});