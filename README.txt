Neely Hopkins / nhopkins@mines.edu
GPU Killas / Neely Hopkins, Gray St. Amant, Christine Hwang
FP / The Grey Havens

HOW TO PLAY:
Try to collect as many coins as possible while avoiding the enemies!
The blue spheres send the enemies away to their starts so you have more time to collect coins. Save these until they get close!
Avoid falling off the edges, as this will force you to start from the starting position.
If you collect all the coins, you win the game, and all enemies are sent to the center of the starting platform. Run some victory laps!

SECTION A:
-movements-
W: move forward
A: move left
S: move backward
D: move right
SPACE: jump

-cameras-
1: first person camera
2: arcball camera (SHIFT = zoom out, SHIFT+SPACE = zoom in)
3: third person camera

SECTION B:
Vehicle moves along bezier curve when it jumps.

SECTION C:
The ground is textured.
The coins are a particle system, which also implements a texture.

SECTION D:
Dynamic light: when the vehicle becomes too close to an object it might crash into, a spotlight will be shown on the object.
Point light: we have lamp posts located around the track

SECTION E:
We have our regular lighting shader implemented, as well as a texture shader, each in a unique way.

SECTION X: gray maybe change this idk. make us fall out of sky or somethuing cool
In the center of the starting platform, you will notice a bunch of trees enclosing the middle of the disk shape.
Normally, collision detection prevents one from entering this center. However, if you keep colliding with the trees for
6 seconds straight, then you will be able to pass through and collect even more coins.

To compile, click build and run.

Bugs:

CONTRIBUTIONS:
Neely Hopkins: created all the platforms and textures, and used my A4 assignment as the base for gameplay (inlcudes enemies and coins)
implemented blue spheres, and all other game logic.
Gray St. Amant: created easter egg and bezier curve.
Christine Hwang: made coins into a particle system, made trees and lamps.

This took us a very long time. Maybe close to 25 hours.

8. The bezier curve lab was super helpful.

10. My favorite and the best way to show off everything we've learned!