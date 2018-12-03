import TimerCountdown from 'react-native-timer-countdown';
import React, { Component } from 'react';
import { StyleSheet, Text, View, Button } from 'react-native';

// Reference Attribution: https://www.npmjs.com/package/react-native-timer-countdown

class Countdown extends Component { 
    render() {
        return (
            <View>
                <TimerCountdown
                    initialSecondsRemaining={1000*30}
                    onTick={secondsRemaining => console.log('tick', secondsRemaining)}
                    onTimeElapsed={() => {this.props.navigation.navigate('Locator')}}
                    allowFontScaling={true}
                    style={{ fontSize: 20 }}
                />
                <Button title="I'm Good" onPress={()=>this.props.navigation.navigate('Splash')}/>
            </View>
        )
    }
}

export default Countdown;
