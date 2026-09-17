[MINI_PROPOSAL.md](https://github.com/user-attachments/files/32349527/MINI_PROPOSAL.md)
# Group Mini Proposal

**Group:** Group 11  
**Team members:** Sara Malmros, Abit Veseli, Marwan Warsame, Edvard Cedell
**Repository:** (https://github.com/Piggelino/C-Project.git)
## Working title

The Sonic Square – Distance-to-Sound Synthesizer

## Interaction in one sentence

Moving a physical square marker closer to or further from a webcam dynamically changes the pitch of a generated synthesizer tone.

## Physical marker action

What will the user do with the marker?

- Change its distance relative to the camera to alter its visible pixel area.
- Hide or show the marker to toggle the sound interaction on or off.
  
## Application response

What will change in the application?

- A C++ application processes the video feed using OpenCV, calculates the marker's pixel area, and continuously maps this value to an audio frequency (Hz) for real-time sound generation.

## Minimum demonstrable version

A program that captures the webcam feed, isolates the square using thresholding, calculates its area in real-time, and outputs a continuous tone where a larger pixel area (closer distance) equals a higher pitch and a smaller area (further distance) equals a lower pitch.

## Optional extension

Map the marker's horizontal (X-axis) position within the camera frame to control audio volume or stereo panning (shifting the sound between the left and right speakers).

## Why this interaction?

It creates an intuitive, tactile connection between spatial distance and audio frequency, effectively transforming the physical space in front of the camera into a theremin-like digital instrument.
