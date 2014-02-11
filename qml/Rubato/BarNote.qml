import QtQuick 2.0

Rectangle {
    id: note
    width: 15
    height: note.parent.height / 88.0
    color: selected ? "yellow" : "lightgrey"
    property int midiNumber
    property int initialMidiNumber
    property bool selected: false

    Component.onCompleted: {
        midiNumber = y * 88 / parent.height
        model.noteOn(midiNumber)
        initialMidiNumber = midiNumber
        noteOffTimer.start()
    }

    Timer {
        id: noteOffTimer
        interval: 250; running: false; repeat: false
        onTriggered: {
            model.noteOff(initialMidiNumber)
        }
    }

    MouseArea {
        anchors.fill: parent
        onPressed: model.noteOn(midiNumber)
        onReleased: model.noteOff(midiNumber)
        onClicked: selected = !selected
        drag.target: parent
        drag.axis: Drag.XAxis
        drag.threshold: 1
        onMouseYChanged: {
            if (pressed) {
                // Quantize the y value to the nearest half note
                var yValue = mapToItem(note.parent, mouse.x, mouse.y).y
                note.y = Math.floor(88 * yValue / note.parent.height) * note.parent.height / 88
                if (note.y > note.parent.height - height)
                    note.y = note.parent.height - height
                if (note.y < 0)
                    note.y = 0
                var newMidiNumber = Math.floor(88 * yValue / note.parent.height)
                if (newMidiNumber > 88) newMidiNumber = 88
                else if (midiNumber < 0) newMidiNumber = 0
                if (newMidiNumber !== midiNumber) {
                    model.noteOff(midiNumber)
                    midiNumber = newMidiNumber
                    model.noteOn(midiNumber)
                }
            }
        }
    }
}
