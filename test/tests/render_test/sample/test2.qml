import QtQuick 2.0

Item {
    Rectangle {
        id: rectangle
        x:0
        y: 0
        width: 155
        height: 160
        color: "#b01818"

        NumberAnimation on x {
            from: 0
            to: 500
            duration: 1000
        }
    }



}
