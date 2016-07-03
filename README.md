# Thing

Final projects for Robotics exam.

# Objectives

The goal of this project is designing a board to control 5 servomotors,
one for each finger of a [robotic hand](http://inmoov.fr/hand-and-forarm/).
The heart of the project is an atXmega microcontoller which should be able
to set both the current, the angle and the speed of each servo individually.
The hand is also capable of holding things with a user-specified force.

Everything is battery powered and commands can be sent via a WiFi link or
through an USB port. A [ROS](http://www.ros.org/) module will be provided for 
integration with existing or new robotics projects.

A programmer is included within the board for easy firmware upgrade.
