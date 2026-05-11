import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.12

ApplicationWindow {
    visible: true
    width: 360
    height: 280
    title: "STM32 Control Suite"

    Rectangle {
        anchors.fill: parent
        color: "#2b2b2b"

        // affichage simple, juste pour la demo
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 10

            Text {
                text: "STM32 Control Suite"
                color: "white"
                font.pointSize: 18
                Layout.alignment: Qt.AlignLeft
            }

            Text {
                text: "Connection: " + (deviceState.connected ? "online" : "offline")
                color: "white"
                font.pointSize: 12
            }

            Text {
                text: "Temperature: " + deviceState.temperature.toFixed(1) + " °C"
                color: "white"
                font.pointSize: 12
            }

            Text {
                text: "Voltage: " + deviceState.voltage.toFixed(2) + " V"
                color: "white"
                font.pointSize: 12
            }

            Text {
                text: "LED: " + (deviceState.ledState ? "on" : "off")
                color: "white"
                font.pointSize: 12
            }

            Item { Layout.fillHeight: true }
        }
    }
}
