# SpeakerView


## Message format

SpatGRIS communicates with SpeakerView using the following messages:

### Initial configuration

```json
{
  "killSV": false,
  "spkStpName": "default_speaker_setup",
  "SGHasFocus": false,
  "KeepSVOnTop": false,
  "SVGrabFocus": false,
  "showHall": false,
  "spatMode": 0,
  "showSourceNumber": false,
  "showSpeakerNumber": false,
  "showSpeakers": true,
  "showSpeakerTriplets": false,
  "showSourceActivity": false,
  "showSpeakerLevel": false,
  "showSphereOrCube": false,
  "genMute": false,
  "spkTriplets": [ [ 1, 2, 3], [4, 2, 7], ... ]
}
```

### Speakers

```json
[
  "speakers",
  [
    // id
    1, 
    
    // pos
    [
      -0.37,
      0.92,
      0.0
    ],

    // is selected:
    false,

    // is direct out only
    false,

    // alpha
    0.75
  ],
  ...
]
```


### Sources

```json
[
  "sources",
  [
    // id
    1,

    // pos
    [
      0.507,
      0.857,
      0.0943
    ],

    // color
    [
      1,
      0,
      0,
      0.8
    ],

    // spat mode (0: dome ; 1: cube)
    0,

    // azimuth span
    0,

    // zenith span
    0
  ],
  ...
```


