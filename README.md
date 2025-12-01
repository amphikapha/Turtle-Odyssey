# 🐢 Turtle Odyssey

**A Chill 3D Endless Runner Game**

![Game Features](https://img.shields.io/badge/3D-OpenGL%203.3-green) ![Models](https://img.shields.io/badge/Models-FBX-blue) ![Language](https://img.shields.io/badge/Language-C++17-orange) ![Platform](https://img.shields.io/badge/Platform-Windows-lightgrey)

---

## 🎮 About The Game

**Turtle Odyssey** is a 3D Endless Runner game where you play as a little adventurous turtle! Dodge retro cars on the road, cross bridges, jump over rivers, collect power-up items, and try to run as far as you can!

This game is developed using **OpenGL 3.3** and **C++17**.

### ✨ Game Features

- 🐢 **3D Turtle Character** - Move freely in all directions
- 🚗 **Retro-Style Cars** - Vehicles moving at random speeds
- 🌉 **Bridges and Tunnels** - Diverse terrain and environments
- 🌊 **Rivers** - Jump across without falling into the water!
- 👹 **Goblin Enemies** - Watch out and dodge them!
- 💊 **Speed Boost Potion** - Temporary speed increase
- ❤️ **Heart Pickups** - Collect to gain extra lives
- 🎵 **Music and Sound Effects** - Immersive audio experience
- 🏆 **High Score System** - Compete with yourself!
- 🌅 **Beautiful Skybox** - Pure sky atmosphere
- 📷 **Third-Person Camera** - Camera follows the player

---

## 🖼️ Screenshots

<img width="1282" height="759" alt="Screenshot 2025-12-01 211440" src="https://github.com/user-attachments/assets/bce48bbd-be7a-4b84-a828-eea7fa21baa8" />

<img width="1282" height="759" alt="Screenshot 2025-12-01 211457" src="https://github.com/user-attachments/assets/057fed6c-ba7c-47f1-926c-fbfe4ee59a3a" />

<img width="1282" height="759" alt="Screenshot 2025-12-01 211633" src="https://github.com/user-attachments/assets/a5bccb39-946c-4a8e-b99b-72d17773a122" />

<img width="1282" height="759" alt="Screenshot 2025-12-01 211602" src="https://github.com/user-attachments/assets/41ee3c23-23a0-49f4-b46b-cf2ca0f9cfac" />

---

## 🎬 Gameplay Video

https://github.com/user-attachments/assets/e23ae2ce-50a7-46ec-a645-17db11fb5b34

---

## 🕹️ Play Now!

🎮 **Play the game at:** [**https://amphikapha.itch.io/turtle-odyssey**](https://amphikapha.itch.io/turtle-odyssey)

---

## 🎮 Controls

| Key | Action |
|-----|--------|
| **W** | Move Forward |
| **S** | Move Backward |
| **A** | Move Left |
| **D** | Move Right |
| **Space** | Jump |
| **[** | Decrease Volume |
| **]** | Increase Volume |
| **ESC** | Exit Game |

---

## 🚀 Quick Start (For Developers)

### Prerequisites:
- **Visual Studio 2019/2022** with C++ workload
- **CMake 3.20+**
- **vcpkg**

### Step 1: Install vcpkg and Dependencies

```powershell
# Clone vcpkg (if not installed)
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat

# Install required libraries
.\vcpkg install glfw3:x64-windows glm:x64-windows assimp:x64-windows openal-soft:x64-windows libsndfile:x64-windows freetype:x64-windows

# Integrate vcpkg with Visual Studio
.\vcpkg integrate install
```

### Step 2: Create `.env` File

Create a `.env` file in the project root directory with your vcpkg toolchain path:

```
VCPKG_TOOLCHAIN=C:/path/to/your/vcpkg/scripts/buildsystems/vcpkg.cmake
```

> **Example:** If vcpkg is installed at `C:\vcpkg`, then:
> ```
> VCPKG_TOOLCHAIN=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
> ```

You can also copy from `.env.example` and modify the path.

### Step 3: Build and Run

```powershell
# Build the game
.\build.bat

# Run the game
.\run.bat
```

---

## 📁 Project Structure

```
Turtle-Odyssey/
├── CMakeLists.txt          # Build configuration
├── README.md               # This file
├── src/
│   ├── main.cpp            # Main game loop
│   └── AudioManager.cpp    # Audio system
├── include/
│   ├── Shader.h            # Shader management
│   ├── Camera.h            # Third-person camera
│   ├── GameObject.h        # Base game object class
│   ├── Player.h            # Player (turtle) class
│   ├── Car.h               # Car obstacle class
│   ├── Model.h             # 3D model loader
│   ├── AudioManager.h      # Audio management
│   ├── HUD.h               # UI elements
│   └── TextRenderer.h      # Text rendering
├── shaders/
│   ├── vertex_shader.glsl
│   ├── fragment_shader.glsl
│   ├── skybox_vertex.glsl
│   ├── skybox_fragment.glsl
│   ├── ui_vertex.glsl
│   ├── ui_fragment.glsl
│   ├── text_vertex.glsl
│   └── text_fragment.glsl
├── assets/
│   ├── models/             # 3D models
│   ├── textures/           # Textures
│   ├── sound/              # Audio files
│   ├── cubemap/            # Skybox textures
│   └── ui/                 # UI elements
└── external/               # External libraries
```

---

## 🙏 Credits & Attribution

### 3D Models

| Asset | Author | License | Source |
|-------|--------|---------|--------|
| **Retro American Car** | Elbolillansen | CC Attribution | [Sketchfab](https://sketchfab.com/3d-models/free-retro-american-car-cartoon-low-poly-920afc941ac44b6599e6191631e8979b) |
| **Goblin** | thedmitrii77 | CC Attribution | [Sketchfab](https://sketchfab.com/3d-models/goblin-3d-model-free-fac5c9e785054b3790b7ade36121945a) |
| **Heart Emoji** | JChanvfx | CC Attribution | [Sketchfab](https://sketchfab.com/3d-models/heart-emoji-ee06846511314b96981313971236e188) |
| **Bridge** | shedmon | CC Attribution | [Sketchfab](https://sketchfab.com/3d-models/bridge-9328bbfc04a84202a6a97bd59408473a) |
| **Low Poly Potion** | mareksson | CC Attribution | [Sketchfab](https://sketchfab.com/3d-models/low-poly-potion-5e4e8d9708f34032bd07a661f6742d2a) |

### Music & Sound Effects

| Asset | Author | Source |
|-------|--------|--------|
| **Beautiful Day** (Background Music) | Zambolino | [FreeTouse.com](https://freetouse.com/music/zambolino/beautiful-day) |
| **Retro Coin** (Collect Sound) | Pixabay | [Pixabay](https://pixabay.com/sound-effects/retro-coin-4-236671/) |
| **Car Crash** | Pixabay | [Pixabay](https://pixabay.com/sound-effects/car-crash-sound-376882/) |
| **Water Splash** | Pixabay | [Pixabay](https://pixabay.com/sound-effects/water-splash-199583/) |
| **Running Sound** | Pixabay | [Pixabay](https://pixabay.com/sound-effects/running-on-the-floor-359909/) |
| **Walking Sound** | Pixabay | [Pixabay](https://pixabay.com/sound-effects/walking-soundscape-200112/) |
| **Energy Drink Effect** | Pixabay | [Pixabay](https://pixabay.com/sound-effects/energy-drink-effect-230559/) |

### Environment & Skybox

| Asset | Author | Source |
|-------|--------|--------|
| **Syferfontein Pure Sky HDRI** | Poly Haven | [Poly Haven](https://polyhaven.com/a/syferfontein_1d_clear_puresky) |
| **Panorama to Cubemap Converter** | jaxry | [GitHub](https://jaxry.github.io/panorama-to-cubemap/) |

### Libraries & Frameworks

- **OpenGL 3.3** - Graphics rendering
- **GLFW** - Window management and input
- **GLM** - Mathematics library
- **Assimp** - 3D model loading
- **OpenAL Soft** - Audio playback
- **FreeType** - Text rendering
- **stb_image** - Image loading

---

## ⚠️ Troubleshooting

### Error: "Cannot find GLFW"
- Make sure GLFW is installed via vcpkg
- Run `vcpkg integrate install`

### Error: "glad/glad.h not found"
- Make sure the file is in `external/glad/include/`

### Black screen / Nothing displays
- Check that shaders are in the `shaders/` folder correctly
- Check the console for any error messages

### No sound
- Make sure audio files are in `assets/sound/`
- Verify that OpenAL is installed correctly

---

## 📝 License

This project was created for educational purposes in the Game Engine with OpenGL course [2110584.68 (2025/1)].

3D models, music, and sound effects are used under their respective Creative Commons licenses. See the Attribution section above for details.

---

**Happy Gaming! 🐢💚**
