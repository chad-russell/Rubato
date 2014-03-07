import QtQuick 2.0
import QtQuick.Controls 1.1

Rectangle {
    width: pitchSlider.x + pitchSlider.width + 20;
    height: 360
    id: window

    property var notes: []

    Connections {
        target: model
        onFreqDataChanged: {
            freqImage.source = ""
            freqImage.source = "image://frequency/fuck"
            blur.source = null
            blur.source = freqImage
        }
    }


    ScrollView {
        id: scrollView
        width: window.width; height: window.height - 45
        MouseArea {
            width: freqImage.implicitWidth; height: window.height - 65

            Rectangle {
                width: freqImage.width
                height: playhead.height
                x: playhead.width/2
                color: "lightgrey"
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        model.seekTo( mouse.x / width )
                        model.emitPercentDoneChanged()
                    }
                }
            }

            Image {
                id: freqImage
                width: parent.width - playhead.width
                height: parent.height - playhead.height
                x: playhead.width/2
                y: playhead.height
                source: model.percentLoaded < 99 ? "" : "image://frequency/fuck"
                fillMode: Image.Stretch

            }
            GaussianBlur {
                id: blur
                anchors.fill: freqImage
                source: freqImage
                radius: 3
                samples: 12
                deviation: 6
            }
            Rectangle {
                id: playhead
                width: 20; height: 20
                color: "steelblue"
                radius: 4
                x: model.percentLoaded < 99 ? -50 : model.percentDone * freqImage.width
                onXChanged: {
                    if(dragArea.pressed)
                        model.seekTo(x / freqImage.width)
                    for(var i = 0; i < notes.length; i++) {
                        if (x > notes[i].x && x < notes[i].x + notes[i].width && !notes[i].playing) {
                            model.noteOn(notes[i].midiNumber)
                            notes[i].playing = true
                        }
                        else if ((x < notes[i].x || x > notes[i].x + notes[i].width) && notes[i].playing) {
                            model.noteOff(notes[i].midiNumber)
                            notes[i].playing = false
                        }
                    }
                }
                MouseArea {
                    id: dragArea
                    anchors.fill: parent
                    drag.target: playhead
                    drag.axis: Drag.XAxis
                    drag.minimumX: 0
                    drag.maximumX: freqImage.width
                }
            }
            Rectangle {
                width: 1; height: freqImage.height
                anchors.top: playhead.bottom
                anchors.left: playhead.horizontalCenter
                color: "pink"
            }

            MouseArea {
                anchors.fill: freqImage
                onPressed: {
                    var component = Qt.createComponent("BarNote.qml")
                    var barNote = component.createObject(this, { "x": mouse.x - 7.5, "y": mouse.y - (parent.height / 88.0)/2 })
                    if (barNote === null) console.log("error creating barNote")
                    window.notes.push(barNote)
                }
            }
        }
    }

    // Only visible when loading
    ProgressBar {
        id: progressBar
        x: 0; y: 0
        width: parent.width; height: (model.percentLoaded > 99 || model.percentLoaded < 1) ? 0 : 4
        minimumValue: 0; maximumValue: 100
        value: model.percentLoaded
    }

    DropArea {
        anchors.fill: parent
        onEntered: {
        }
        onDropped: {
            model.file = drop.urls[0]
        }
    }

    Slider {
        id: tempoSlider
        anchors.left: parent.left; anchors.leftMargin: 20
        width: 300
        minimumValue: 0.2
        anchors.verticalCenter: playPauseButton.verticalCenter
        maximumValue: 5
        value: 1
        onValueChanged: {
            model.setTimestretchRatio(value)
        }
    }
    Button {
        id: playPauseButton
        width: 70; height: 30
        anchors.bottom: window.bottom
        anchors.left: tempoSlider.right; anchors.leftMargin: 20
        anchors.margins: 3
        text: model.playing ? "pause" : "play"
        onClicked: model.play()
    }
    Slider {
        id: pitchSlider
        anchors.left: playPauseButton.right; anchors.leftMargin: 20
        width: 300
        minimumValue: 0.5
        anchors.verticalCenter: playPauseButton.verticalCenter
        maximumValue: 2
        value: 1
        onValueChanged: {
            model.setSampleRateRatio(value)
        }

    }
}
