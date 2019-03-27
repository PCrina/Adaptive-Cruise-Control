# Adaptive-Cruise-Control
Build the prototype of an autonomous car, based on a system that works with a video camera. The system automatically adjusts the vehicle’s speed, accelerating or slowing down in order to maintain a safe distance from the vehicle ahead. If the action of slowing down is not enough to avoid collision, the car will change its direction.

The application architecture is based on a TCP socket based communication protocol. The client, running on the UDOO board, captures the images through the camera and sends them to the server. The server receives the image, processes it and sends back to the client a number representing the distance to the front car. When the client receives the distance he will make a decision about the speed sent to the Arduino module, which controls the engines.

## How to start the application
1) Plug in the power sources for the car.
2) Power up the udoo board.
3) Login using VNC to upload the arduino code.
4) Connect via ssh or VNC and run sender and receiver.
5) The car will look for a specific object(circle, license plate) to adapt its speed, otherwise it will keep moving at the speed that has been set.
