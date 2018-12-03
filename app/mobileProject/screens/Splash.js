import React, { Component } from 'react';
import { StyleSheet, Text, View, Button } from 'react-native';

class Splash extends Component {
    render() {
        return (
            <View style={styles.container}>
                <Text style={styles.title}>YouGood? Fall Detector</Text>
                <View style = {styles.button}>
                    <Button title="Timer" onPress={()=>this.props.navigation.navigate('Countdown')}/>
                </View>
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