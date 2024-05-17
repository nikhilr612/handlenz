# HandLenz
A MediaPipe Application that performs real-time hand landmark detection, and uses the resulting landmarks to generate MIDI messages, by mapping landmark space to pitch space using a provided scale.

# HandLenz Application Usage

- See [LibMP - MediaPipe as a Shared Library](https://github.com/rajkundu/mediapipe) for more information on LibMP
- This application does the following in realtime:
  1. Streams video frames from an attached camera/webcam (OpenCV Device #0)
  2. Runs MediaPipe Hand Landmark on each frame
  3. Uses LibMP to get the XYZ coordinates of detected landmarks
  4. Draws a circle at each landmark's location on the input frame
  5. Displays the result using OpenCV/HighGUI
- Compilation is managed using CMake - see [`CMakeLists.txt`](/CMakeLists.txt) and [`libprotobuf.cmake`](/libprotobuf.cmake)
    - Change `${MEDIAPIPE_DIR}` in `CMakeLists.txt` to match your installation of LibMP
- This application uses a custom [fork](https://github.com/nikhilr612/libmp) of the original LibMP project.
- Tested on Windows (MSVC 17.9.8)

## Prerequisites
1. Clone and build [LibMP](https://github.com/nikhilr612/libmp)

## Usage
1. Compile using CMake
2. `cd` into output directory containing `main` binary
3. Run `main` binary

## How does it work?
Some (most?) MediaPipe data types are based on [Protobuf](https://developers.google.com/protocol-buffers) messages. Thus, their data can be easily serialized and deserialized. LibMP provides functions which accomplish this, and this project imports both Protobuf & the Protobuf-based MediaPipe types - allowing for practically native use of MediaPipe data types in an external C++ application.