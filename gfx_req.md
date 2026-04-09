# Comprehensive Computer Graphics Project Requirements

## Assignment Requirements

* **Camera & Viewing Controls:**
    * Implement a Bird's Eye View viewing transformation.
    * Implement camera rotations: Pitch (Key `X`), Yaw (Key `Y`), and Roll (Key `Z`).
    * Implement flying simulator movement: Forward (`W`), Backward (`S`), Left (`A`), Right (`D`), Up (`E`), and Down (`R`).
    * Rotate the camera around a look-at point (Key `F`).
    * Divide the viewport into 4 equal parts. Control the scene of each viewport with user input (e.g., toggling Combine Lighting, Ambient Only, Diffuse Only, Directional Only alongside Isometric, Top, Front, and Inside views).
* **Lighting Implementation:**
    * Include multiple light sources: Point lights, Directional light, Spot light (with a single cut-off angle), and Emissive light.
    * Lighting toggles:
        * Directional light (Key `1`), Point lights (Key `2`), Spot light (Key `3`).
        * Ambient light (Key `5`), Diffuse light (Key `6`), Specular light (Key `7`).
        * General Light On/Off (Key `L`).
* **Textures & Surfaces:**
    * Implement multiple textured objects:
        * Simple texture without surface color.
        * Blended texture with surface color (color computed on both the vertex and the fragment).
    * Include textured curvy surfaced objects (must include at least a sphere and a cone).
    * Incorporate curvy objects using Bezier curves, Spline Curves, and Ruled surfaces.
* **Scene Dynamics & Interaction:**
    * Implement a rotating fan (Key `G`).
    * Include doors and windows that can be opened and closed.
    * Simulate the function of dynamic equipment.
    * Add keyboard interactions to toggle features.
    * Print all controls and key actions to the console.
* **Custom Math Function:**
    * Implement your own custom version of the `glm::rotate` function.

## Special Requirements

* **Complex Objects & Hierarchical Movement:**
    * Include scene objects with complex, hierarchical movements.
        * *Example:* A robot arm/hand where the arm and fingers have individual pivot points, movement restrictions, and follow kinematics equations.
* **Custom Tool Integration:**
    * Create at least one object using the provided "wine glass making program" by exporting the points, and use that object in the final scene.
* **Advanced Environment & Animations:**
    * Generate tree leaves using fractals.
    * Implement random motion algorithms for birds.
    * Implement motion for both the camera and light sources.
    * Tie dynamic lighting projections to the position of the sun.
* **Physics:**
    * Implement basic physics interactions, specifically collision detection (as emphasized by Masud sir).
* **Specific Object Request:**
    * Add a clock to the scene (as requested by Taj sir).