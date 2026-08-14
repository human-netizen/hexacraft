#pragma once
// =====================================================
// Helper: convert world XZ to grid col/row
// =====================================================
void worldToColRow(float wx, float wz, int &col, int &row) {
    float zSpacing = HEX_RADIUS * 1.5f;
    float xSpacing = HEX_RADIUS * 2.0f * 0.866f;
    row = (int)roundf(wz / zSpacing);
    float xOff = (abs(row) % 2 == 1) ? (xSpacing * 0.5f) : 0.0f;
    col = (int)roundf((wx - xOff) / xSpacing);
}

// Y-aware ground: find highest solid block BELOW the player's feet
float getGroundYAtHeight(int col, int row, float currentY) {
    int playerH = (int)floorf(currentY / HEX_HEIGHT);
    for (int h = playerH; h >= 0; h--) {
        if (getBlock(col, row, h) != BLOCK_AIR) {
            return (float)(h + 1) * HEX_HEIGHT;
        }
    }
    return 0.0f;
}

// Check if a block at (col, row, h) is solid for collision
bool isSolid(int col, int row, int h) {
    int bt = getBlock(col, row, h);
    if (bt == BLOCK_AIR || bt == BLOCK_WATER) return false;
    BlockProperties props = getBlockProps(bt);
    if (!props.isSolid) return false;
    // Open interactive blocks (doors, trapdoors, fence gates) are passable
    if (props.isInteractive) {
        uint16_t state = getBlockState(col, row, h);
        if ((state >> 2) & 1) return false; // open = not solid
    }
    return true;
}

// =====================================================
// Third-person camera collision (19E-5)
// =====================================================
// March from the player's eye toward where the third-person camera wants to sit
// and stop short of the first solid block. Without this the camera sinks into
// whatever the player backs up against and you end up looking at the inside of a
// wall — or through it, since nothing here culls back faces.
//
// Returns the (possibly shortened) offset from `pivot`, never a longer one.
glm::vec3 collideCamera(glm::vec3 pivot, glm::vec3 desiredOffset) {
    float wanted = glm::length(desiredOffset);
    if (wanted < 1e-4f) return desiredOffset;
    glm::vec3 dir = desiredOffset / wanted;

    // Keep the near plane out of the wall it stopped against.
    const float pad = 0.35f;
    const float step = 0.15f;

    for (float d = step; d < wanted; d += step) {
        glm::vec3 p = pivot + dir * d;
        int col, row, h;
        worldToGrid(p, col, row, h);
        if (isSolid(col, row, h)) {
            float safe = d - pad;
            if (safe < 0.25f) safe = 0.25f;   // never collapse fully into the head
            return dir * safe;
        }
    }
    return desiredOffset;
}

// =====================================================
// Car as a solid body
// =====================================================
// Half-extents of the car in its own frame, read off drawCar (objects.h): the
// front bumper reaches x = 1.125 and the wheels reach z = 0.65.
const float CAR_HALF_LEN = 1.05f;
const float CAR_HALF_WID = 0.60f;
// Tallest curb the car will ride up. One block plus epsilon — the same allowance
// tryMoveWithStep gives the player, and for the same reason: terrain here is
// built from whole blocks, and a car that stopped dead at every one-block rise
// could not get out of the courtyard.
const float CAR_STEP_MAX = 1.1f;

// True when (wx, wz) at feet height feetY is inside the parked car.
//
// This is the one idea worth taking from their Collider.h — that a vehicle is a
// body other things collide with. Not the data structure: theirs is a list of
// hand-registered AABBs, and nobody is going to register 1.28 M of those for the
// voxel grid this world is actually made of.
//
// Skipped entirely while driving. The B key hands control to the car without
// moving the player, so the two are normally far apart, but if the car is driven
// back over the player the block would otherwise trap them — see the push-out in
// processInput, which is what actually resolves that case.
// World -> car-local. Forward is (cos, sin) — taken from the motion integrator
// in processInput rather than from drawCar's matrix, because where the car
// actually goes is what has to be blocked. The footprint is symmetric across its
// long axis, so the sign convention for "right" does not matter to the test;
// the push-out does care, and reads `outLz` back.
bool carOverlaps(float wx, float wz, float feetY, float pad,
                 float* outLx = nullptr, float* outLz = nullptr) {
    // Vertical band: the body sits roughly 1.4 units above its base. Outside
    // that the mover is under it or on a ledge above it, neither of which the
    // flat XZ test can speak to.
    if (feetY > carPos.y + 1.4f || feetY < carPos.y - 1.4f) return false;

    float c = cosf(carYaw), s = sinf(carYaw);
    float dx = wx - carPos.x, dz = wz - carPos.z;
    float lx =  dx * c + dz * s;
    float lz = -dx * s + dz * c;
    if (outLx) *outLx = lx;
    if (outLz) *outLz = lz;
    return fabsf(lx) < CAR_HALF_LEN + pad && fabsf(lz) < CAR_HALF_WID + pad;
}

// The movement gate. Skipped entirely while driving: the B key hands control to
// the car without moving the player, so blocking would trap whoever is standing
// in it. Running the player over is handled by the push-out in processInput,
// which calls carOverlaps directly and therefore still fires while driving.
bool carBlocks(float wx, float wz, float feetY, float pad) {
    if (controlCar) return false;
    return carOverlaps(wx, wz, feetY, pad);
}

// Check if player can stand at world position (wx, wz) at current height.
// Samples 9 points around the player's horizontal radius so blocks that
// are adjacent (but not exactly under the center) are also tested.
bool canMoveTo(float wx, float wz, float currentY) {
    int feetH = (int)floorf(currentY / HEX_HEIGHT);
    int headH = feetH + 1;

    // Player physical half-width — keep slightly under hex half-width (0.43)
    // so a 1-hex-wide corridor is still passable.
    static const float PR = 0.28f;
    static const float SX[9] = { 0,  PR, -PR,  0,    0,  PR*0.707f, -PR*0.707f, -PR*0.707f,  PR*0.707f };
    static const float SZ[9] = { 0,   0,    0, PR, -PR,  PR*0.707f,  PR*0.707f, -PR*0.707f, -PR*0.707f };

    int seenC[9], seenR[9];
    int nSeen = 0;
    for (int i = 0; i < 9; i++) {
        int col, row;
        worldToColRow(wx + SX[i], wz + SZ[i], col, row);
        // skip if we already checked this cell
        bool dup = false;
        for (int j = 0; j < nSeen; j++) {
            if (seenC[j] == col && seenR[j] == row) { dup = true; break; }
        }
        if (dup) continue;
        seenC[nSeen] = col; seenR[nSeen] = row; nSeen++;

        if (isSolid(col, row, feetH)) return false;
        if (isSolid(col, row, headH)) return false;
    }

    // The parked car is solid too. This runs for mobs as well as the player,
    // since they share this function — which is right: a pig should walk around
    // the car, not through it.
    if (carBlocks(wx, wz, currentY, PR)) return false;

    return true;
}

// Try to move with auto-step (climb 1-block ledges)
bool tryMoveWithStep(float& px, float& py, float& pz, float nx, float nz) {
    if (canMoveTo(nx, nz, py)) {
        px = nx; pz = nz;
        return true;
    }
    float steppedY = py + HEX_HEIGHT;
    if (canMoveTo(nx, nz, steppedY)) {
        int col, row;
        worldToColRow(nx, nz, col, row);
        float groundY = getGroundYAtHeight(col, row, steppedY);
        if (groundY > py && groundY <= py + HEX_HEIGHT + 0.1f) {
            px = nx; pz = nz;
            py = groundY;
            return true;
        }
    }
    return false;
}

// True when the car's whole footprint at (wx, wz) heading `yaw` clears the terrain.
//
// Six probes: the four corners, plus the middle of the front and rear bumpers so
// a lone pillar cannot slip between the corners of a 2.1 x 1.2 body.
//
// The test is a HEIGHT comparison, not the isSolid() test canMoveTo uses. That
// matters: carPos.y trails the ground it is driving onto (the terrain-follow
// below eases toward it), so at the moment the car meets a one-block rise the
// block ahead genuinely is solid at the car's current level. An isSolid() test
// would refuse every hill. Comparing ground heights instead asks the question
// that was actually meant — is this a curb or a wall.
//
// Limitation, inherited rather than introduced: getGroundYWorld reports the top
// of the highest block in the column, so driving under an overhang reads as
// blocked. The pre-existing terrain-follow has the same blind spot — it would
// lift the car onto the bridge deck — so this is no worse than before.
// (definition below — it needs getGroundYWorld)

// Legacy: get ground Y at a world XZ (uses highest block — for objects, not player)
float getGroundYWorld(float wx, float wz) {
    int col, row;
    worldToColRow(wx, wz, col, row);
    return getGroundY(col, row);
}

bool carCanBeAt(float wx, float wz, float yaw, float carY) {
    static const float LX[6] = { 1.0f,  1.0f, -1.0f, -1.0f,  1.0f, -1.0f };
    static const float LZ[6] = { 1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f };
    float c = cosf(yaw), s = sinf(yaw);
    for (int i = 0; i < 6; i++) {
        float lx = LX[i] * CAR_HALF_LEN, lz = LZ[i] * CAR_HALF_WID;
        float px = wx + lx * c - lz * s;
        float pz = wz + lx * s + lz * c;
        if (getGroundYWorld(px, pz) > carY + CAR_STEP_MAX) return false;
    }
    return true;
}

// =====================================================
// Block break / place actions
// =====================================================
void spawnItemDrop(int col, int row, int h, BlockType bt) {
    glm::vec3 pos = hexGridPos(col, row, 0.0f) + glm::vec3(0, h * HEX_HEIGHT + 0.5f, 0);
    ItemDrop drop;
    drop.pos = pos;
    drop.vel = glm::vec3(
        (float)((gridSeed(col, row) % 100) - 50) * 0.02f,
        3.0f,
        (float)((gridSeed(row, col) % 100) - 50) * 0.02f
    );
    drop.type = bt;
    drop.lifetime = 0.0f;
    drop.bobPhase = (float)(gridSeed(col + h, row) % 100) * 0.06f;
    itemDrops.push_back(drop);
}

void breakBlock() {
    if (!hasTarget) return;
    int bt = getBlock(targetCol, targetRow, targetHeight);
    if (bt == BLOCK_AIR) return;
    setBlock(targetCol, targetRow, targetHeight, BLOCK_AIR);
    int dropType = getBlockDrop(bt);
    if (dropType != BLOCK_AIR) {
        addToInventory(dropType, 1);
        printf("[Decompose] %s -> +1 %s added to inventory\n",
               getBlockName(bt), getBlockName(dropType));
    } else {
        printf("[Decompose] %s decomposed (no drop)\n", getBlockName(bt));
    }

    // Degrade tool durability
    InventorySlot& held = playerInventory[hotbarSlot];
    if (held.durability > 0) {
        held.durability--;
        if (held.durability <= 0) {
            printf("[Tool] %s broke!\n", getBlockName(held.type));
            held = {BLOCK_AIR, 0, -1};
        }
    }
}

// Convert player yaw to 4-direction facing (0=+Z, 1=+X, 2=-Z, 3=-X)
int yawToFacing(float yaw) {
    // yaw is atan2(z, x); normalize to [0, 2PI)
    float a = fmodf(yaw + 2.0f * PI, 2.0f * PI);
    // split into 4 quadrants
    if (a < PI * 0.25f || a >= PI * 1.75f) return 0;
    if (a < PI * 0.75f) return 1;
    if (a < PI * 1.25f) return 2;
    return 3;
}

void placeBlock() {
    if (!hasTarget) return;
    if (!gridInBounds(placeCol, placeRow, placeHeight)) return;
    if (getBlock(placeCol, placeRow, placeHeight) != BLOCK_AIR) return;
    int bt = playerInventory[hotbarSlot].type;
    if (bt == BLOCK_AIR || playerInventory[hotbarSlot].count <= 0) return;

    // Items (tools etc.) can't be placed in the world
    if (getBlockProps(bt).isItem) return;

    setBlock(placeCol, placeRow, placeHeight, bt);
    playerInventory[hotbarSlot].count--;
    if (playerInventory[hotbarSlot].count <= 0) playerInventory[hotbarSlot].type = BLOCK_AIR;

    // Set initial facing state for directional blocks
    BlockProperties props = getBlockProps(bt);
    if (props.shape == SHAPE_DOOR || props.shape == SHAPE_TRAPDOOR
        || props.shape == SHAPE_STAIR || props.shape == SHAPE_FENCE
        || props.shape == SHAPE_FLAT_PANEL) {
        uint16_t state = yawToFacing(playerYaw) & 3;
        setBlockState(placeCol, placeRow, placeHeight, state);
    }
    printf("[Block] Placed %d at (%d, %d, %d)\n", bt, placeCol, placeRow, placeHeight);
}

// =====================================================
// Grab / carry — K
// =====================================================
// Adapted from ../Tasfia-007-OpenGL-3.3--Medieval-European-Countryside-/Project1/
// main.cpp:2966-2977 and :3438-3441, where G near a barrel sets g_heldObjIdx and
// the barrel is then drawn at cam.pos + cam.front * 2.5 for as long as it is held.
//
// Theirs picks from a fixed list of scenery props. hexacraft has no such list —
// everything in the world is a grid cell — so the port lifts a BLOCK out of the
// grid instead. The mechanic survives intact and the state stays just as small:
// what was one index is one block type plus the cell it came from.
//
// The point of having this next to breakBlock() is that it moves a block WITHOUT
// laundering it through the inventory. Break a mossy cobblestone and getBlockDrop
// hands back plain cobblestone; break a door and its facing is gone. Carry it and
// both survive, because the type and the state word travel with the block.

// How far in front of the eye the block floats, and how big it is drawn. Full
// size at 2.2 units fills most of the screen, which hides whatever the player is
// trying to aim at — and aiming is how the block gets set down again.
const float CARRY_DIST  = 2.2f;
const float CARRY_SCALE = 0.45f;
// Lateral / vertical offset from the aiming ray, in world units at CARRY_DIST.
const float CARRY_OFF_X = 0.62f;
const float CARRY_OFF_Y = -0.42f;

// The block hangs on the aiming ray, not in front of the camera. These differ in
// third person, where camPos sits 3.5 units behind the player: camPos + front*2.2
// would put the block *behind* the player's head, between them and the camera.
// Same expression updateBlockTarget() casts from (objects.h:195), so the carried
// block always sits on the line the crosshair is testing.
static glm::vec3 carryOrigin() {
    return (cameraMode == 2) ? camPos
                             : playerWorldPos + glm::vec3(0.0f, 1.62f, 0.0f);
}

// Put the carried block back in the cell it was lifted from.
static void returnCarriedToOrigin() {
    if (heldBlockType == BLOCK_AIR) return;
    if (gridInBounds(heldOriginCol, heldOriginRow, heldOriginH)
        && getBlock(heldOriginCol, heldOriginRow, heldOriginH) == BLOCK_AIR) {
        setBlock(heldOriginCol, heldOriginRow, heldOriginH, heldBlockType);
        setBlockState(heldOriginCol, heldOriginRow, heldOriginH, heldBlockState);
    } else {
        // The hole got filled in while the block was being carried. Dropping it
        // as an item is the only outcome that neither destroys the block nor
        // overwrites whatever is standing there now.
        spawnItemDrop(heldOriginCol, heldOriginRow, heldOriginH, (BlockType)heldBlockType);
    }
    heldBlockType = BLOCK_AIR;
}

// K: lift the targeted block, or set the carried one down at the placement cell.
void toggleCarry() {
    if (playerDead) return;

    if (heldBlockType != BLOCK_AIR) {
        // --- set down ---
        if (!hasTarget || !gridInBounds(placeCol, placeRow, placeHeight)
            || getBlock(placeCol, placeRow, placeHeight) != BLOCK_AIR) {
            // Deliberately NOT "send it back to its origin", which is what the
            // plan called for. By the time a player is looking at open sky they
            // have usually walked some distance, and teleporting the block back
            // across the map is a stranger outcome than simply not letting go.
            // Origin restore is kept for death, where there is no other choice.
            printf("[Carry] Nowhere to set the %s down — aim at a block face\n",
                   getBlockName(heldBlockType));
            return;
        }
        setBlock(placeCol, placeRow, placeHeight, heldBlockType);
        setBlockState(placeCol, placeRow, placeHeight, heldBlockState);
        printf("[Carry] Set %s down at (%d, %d, %d)\n",
               getBlockName(heldBlockType), placeCol, placeRow, placeHeight);
        heldBlockType = BLOCK_AIR;
        return;
    }

    // --- pick up ---
    if (!hasTarget) return;
    int bt = getBlock(targetCol, targetRow, targetHeight);
    if (bt == BLOCK_AIR) return;
    // Same rule the break path enforces: bedrock is the floor of the world and
    // carrying it away would open a hole straight out the bottom.
    if (bt == BLOCK_BEDROCK) {
        printf("[Carry] Bedrock will not budge\n");
        return;
    }
    heldBlockType  = bt;
    heldBlockState = getBlockState(targetCol, targetRow, targetHeight);
    heldOriginCol  = targetCol;
    heldOriginRow  = targetRow;
    heldOriginH    = targetHeight;
    setBlock(targetCol, targetRow, targetHeight, BLOCK_AIR);
    printf("[Carry] Picked up %s from (%d, %d, %d)\n",
           getBlockName(bt), targetCol, targetRow, targetHeight);
}

// Once per frame, before the draw passes. Only job is death: a block held at the
// moment of death would otherwise sit in a global that nothing ever clears, and
// respawning would leave the player holding a block that no longer exists
// anywhere in the world.
void updateCarry() {
    if (heldBlockType == BLOCK_AIR || !playerDead) return;
    printf("[Carry] Died holding %s — returned it to (%d, %d, %d)\n",
           getBlockName(heldBlockType), heldOriginCol, heldOriginRow, heldOriginH);
    returnCarriedToOrigin();
}

// Draw pass. Called per viewport, so it must not touch any state.
void drawCarriedBlock(float time) {
    if (heldBlockType == BLOCK_AIR) return;

    // Offset down and to the right rather than dead ahead. Centred, the block
    // sits exactly behind the crosshair and hides it — and the crosshair is what
    // picks the cell the block gets set down in, so a centred carry makes the
    // block impossible to put back down accurately.
    glm::vec3 right = glm::normalize(glm::cross(camFront, camUp));
    glm::vec3 pos = carryOrigin() + camFront * CARRY_DIST
                  + right * CARRY_OFF_X + camUp * CARRY_OFF_Y;
    // A slow bob and spin. Without them the block is welded to the view and
    // reads as a HUD element painted on the screen rather than as an object
    // being carried through the world.
    pos.y += sinf(time * 1.6f) * 0.04f;

    glm::mat4 model = glm::translate(glm::mat4(1.0f), pos);
    model = myRotate(model, time * 0.8f, glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::scale(model, glm::vec3(CARRY_SCALE));

    bindBlockTexture(heldBlockType);
    drawHexModel(model, getBlockColor(heldBlockType));

    // bindBlockTexture() leaves textureMode, the sway amplitude and the specular
    // response set for THIS block type. Everything drawn afterwards in the pass
    // shares the shader, so leaving them set would tint the next object — and a
    // leaf block would have handed it a wind sway as well.
    setInt(shaderProgram, "textureMode", 0);
    setSway(0.0f);
    resetBlockSpecular();
}

// =====================================================
// Mob system — Phase 15: Mobs & Combat
// =====================================================

// --- 15D: A* Pathfinding on hex grid ---
struct AStarNode {
    int col, row, h;
    float g, f;
    int parentIdx;
};

float hexDistance(int c1, int r1, int c2, int r2) {
    // Axial hex distance
    float dx = (float)(c2 - c1);
    float dz = (float)(r2 - r1);
    return sqrtf(dx * dx + dz * dz);
}

// Get 6 hex neighbors for a given col/row
void getHexNeighbors(int col, int row, int out[][2], int& count) {
    count = 0;
    bool odd = (abs(row) % 2 == 1);
    // 6 hex directions (flat-top): depends on odd/even row
    int dirs[6][2];
    if (odd) {
        dirs[0][0]= 1; dirs[0][1]= 0;
        dirs[1][0]=-1; dirs[1][1]= 0;
        dirs[2][0]= 0; dirs[2][1]= 1;
        dirs[3][0]= 1; dirs[3][1]= 1;
        dirs[4][0]= 0; dirs[4][1]=-1;
        dirs[5][0]= 1; dirs[5][1]=-1;
    } else {
        dirs[0][0]= 1; dirs[0][1]= 0;
        dirs[1][0]=-1; dirs[1][1]= 0;
        dirs[2][0]=-1; dirs[2][1]= 1;
        dirs[3][0]= 0; dirs[3][1]= 1;
        dirs[4][0]=-1; dirs[4][1]=-1;
        dirs[5][0]= 0; dirs[5][1]=-1;
    }
    for (int i = 0; i < 6; i++) {
        out[count][0] = col + dirs[i][0];
        out[count][1] = row + dirs[i][1];
        count++;
    }
}

// =====================================================
// Terrain sculpt tools — HILL / POND (plan_2 Step 3)
// =====================================================
// Ported from gfx_b3 main.cpp:1827-1890, which reshapes an area of terrain in
// one click instead of a block at a time. hexacraft has had single-block
// break/place only.
//
// gfx_b3 works in axial (q,r) coordinates and tests ring membership with
// axialHexDist. hexacraft stores offset (col,row), and its hexDistance() above
// is plain Euclidean on those offsets — correct enough as an A* heuristic, but
// wrong as a ring test: in offset coordinates two cells the same number of hops
// apart have different Euclidean distances depending on row parity, so a disc
// cut that way comes out lopsided. Flood-filling with getHexNeighbors() gives
// exact hop distance and needs no coordinate conversion.
//
// Nothing here has to invalidate a mesh: hexacraft rebuilds terrain geometry
// from blockGrid every frame, and setBlock() already maintains columnMaxH — so
// findGroundH, mob pathing, rain landing and ground scatter all pick the new
// heights up on their own.

// Largest disc used below is radius 3 = 1 + 6 + 12 + 18 = 37 columns.
const int SCULPT_MAX_CELLS = 37;

// Columns of a hex disc, centre first, each tagged with its ring index.
// Returns the count written.
int hexDisc(int cCol, int cRow, int radius,
            int outCol[], int outRow[], int outRing[], int maxOut) {
    if (maxOut <= 0) return 0;
    outCol[0] = cCol; outRow[0] = cRow; outRing[0] = 0;
    int n = 1;
    int ringStart = 0, ringEnd = 1;   // [start,end) = the ring just completed
    for (int r = 1; r <= radius; r++) {
        int nextStart = n;
        for (int i = ringStart; i < ringEnd; i++) {
            int nb[6][2], nc = 0;
            getHexNeighbors(outCol[i], outRow[i], nb, nc);
            for (int k = 0; k < nc; k++) {
                bool seen = false;
                for (int j = 0; j < n; j++) {
                    if (outCol[j] == nb[k][0] && outRow[j] == nb[k][1]) { seen = true; break; }
                }
                if (seen) continue;
                if (n >= maxOut) return n;
                outCol[n] = nb[k][0]; outRow[n] = nb[k][1]; outRing[n] = r;
                n++;
            }
        }
        ringStart = nextStart; ringEnd = n;
    }
    return n;
}

// Highest naturally-occurring ground block in a column.
//
// findGroundH() is the wrong probe here. It uses blocksSight(), and wood blocks
// sight — so on a column with a tree it returns the top of the trunk, and a hill
// raised there would start in the canopy. This skips anything built or grown and
// finds the actual land surface.
bool isTerrainMaterial(int t) {
    switch (t) {
        case BLOCK_GRASS: case BLOCK_DIRT:   case BLOCK_SAND:
        case BLOCK_STONE: case BLOCK_STONE_LIGHT:
        case BLOCK_GRAVEL: case BLOCK_CLAY:  case BLOCK_SNOW:
        case BLOCK_SANDSTONE: case BLOCK_BEDROCK: case BLOCK_COBBLESTONE:
        case BLOCK_COAL_ORE: case BLOCK_ORE_GOLD: case BLOCK_ORE_DIAMOND:
            return true;
        default:
            return false;
    }
}

int findTerrainH(int col, int row) {
    int gi = col + GRID_OFF_X, gj = row + GRID_OFF_Z;
    if (gi < 0 || gi >= GRID_W || gj < 0 || gj >= GRID_D) return -1;
    for (int h = columnMaxH[gi][gj]; h >= 0; h--) {
        if (isTerrainMaterial(getBlock(col, row, h))) return h;
    }
    return -1;
}

// HILL — raise a radius-2 disc: centre +5, ring 1 +3, ring 2 +1.
const int HILL_RAISE[3] = { 5, 3, 1 };

void sculptHill(int col, int row) {
    int dc[SCULPT_MAX_CELLS], dr[SCULPT_MAX_CELLS], dring[SCULPT_MAX_CELLS];
    int n = hexDisc(col, row, 2, dc, dr, dring, SCULPT_MAX_CELLS);
    int raised = 0;

    for (int i = 0; i < n; i++) {
        if (!gridInBounds(dc[i], dr[i], 0)) continue;
        int base = findTerrainH(dc[i], dr[i]);
        if (base < 0) continue;

        int top = base + HILL_RAISE[dring[i]];
        if (top > GRID_H - 1) top = GRID_H - 1;   // a hill on a mountain must not
        if (top <= base) continue;                // write past the grid ceiling

        // Stone core, one dirt layer, grass cap. The layering is the whole
        // reason this reads as terrain rather than a stack of one material.
        for (int h = base + 1; h <= top; h++) {
            int mat = BLOCK_STONE;
            if (h == top)          mat = BLOCK_GRASS;
            else if (h == top - 1) mat = BLOCK_DIRT;
            setBlock(dc[i], dr[i], h, mat);
        }
        // The old surface is now interior. Leaving grass buried in the hill
        // shows up the moment anyone mines into it.
        if (getBlock(dc[i], dr[i], base) == BLOCK_GRASS)
            setBlock(dc[i], dr[i], base, BLOCK_DIRT);
        raised++;
    }
    printf("[Sculpt] Hill at (%d,%d) — %d columns raised\n", col, row, raised);
}

// POND — dig 3 layers down over a radius-1..3 disc and fill with water to one
// below the rim, so the bank always reads above the surface.
void sculptPond(int col, int row) {
    int radius = 1 + (int)(gridSeed(col, row) % 3);   // 1..3, stable per column
    int dc[SCULPT_MAX_CELLS], dr[SCULPT_MAX_CELLS], dring[SCULPT_MAX_CELLS];
    int n = hexDisc(col, row, radius, dc, dr, dring, SCULPT_MAX_CELLS);

    // The rim is the terrain height at the centre. Using each column's own
    // height instead would step the water surface, and standing water is level.
    int rim = findTerrainH(col, row);
    if (rim < 0) { printf("[Sculpt] Pond at (%d,%d) — no ground\n", col, row); return; }

    const int DIG = 3;
    // Layer 0 is bedrock and the world floor; never cut into it.
    int floorH = rim - DIG;
    if (floorH < 1) floorH = 1;
    int waterTop = rim - 1;
    if (waterTop < floorH) waterTop = floorH;

    // Is this column part of the basin? Used to tell an inside wall (which gets
    // dug) from an outside one (which has to hold water in).
    auto inDisc = [&](int cc, int rr) {
        for (int i = 0; i < n; i++) if (dc[i] == cc && dr[i] == rr) return true;
        return false;
    };

    int dug = 0, sealed = 0;
    for (int i = 0; i < n; i++) {
        if (!gridInBounds(dc[i], dr[i], 0)) continue;
        int top = findTerrainH(dc[i], dr[i]);
        if (top < 0) continue;

        // Clear everything above the floor — including any bank that stood
        // higher than the centre, or the pond gets a wall through its middle.
        int clearTo = (top > rim) ? top : rim;
        for (int h = floorH; h <= clearTo; h++) setBlock(dc[i], dr[i], h, BLOCK_AIR);

        // Seal the basin before filling it.
        //
        // gfx_b3 skips this because its world is solid below the surface. This
        // one is not: the terrain is riddled with caves, and a pond dug over
        // one ends up as water hanging above an open void. So line the bottom
        // and any outward-facing wall that isn't already rock.
        if (getBlock(dc[i], dr[i], floorH - 1) == BLOCK_AIR) {
            setBlock(dc[i], dr[i], floorH - 1, BLOCK_STONE);
            sealed++;
        }
        int nb[6][2], nc = 0;
        getHexNeighbors(dc[i], dr[i], nb, nc);
        for (int k = 0; k < nc; k++) {
            if (inDisc(nb[k][0], nb[k][1])) continue;      // dug too — no wall needed
            if (!gridInBounds(nb[k][0], nb[k][1], 0)) continue;
            for (int h = floorH; h <= waterTop; h++) {
                if (getBlock(nb[k][0], nb[k][1], h) == BLOCK_AIR) {
                    setBlock(nb[k][0], nb[k][1], h, BLOCK_STONE);
                    sealed++;
                }
            }
        }

        for (int h = floorH; h <= waterTop; h++) setBlock(dc[i], dr[i], h, BLOCK_WATER);
        dug++;
    }
    printf("[Sculpt] Pond at (%d,%d) r=%d rim=%d — %d columns dug, %d cells sealed\n",
           col, row, radius, rim, dug, sealed);
}

// A* pathfinding: returns world-space waypoints from start to goal
std::vector<glm::vec3> findPath(glm::vec3 startWorld, glm::vec3 goalWorld) {
    std::vector<glm::vec3> result;
    int sc, sr, gc, gr;
    worldToColRow(startWorld.x, startWorld.z, sc, sr);
    worldToColRow(goalWorld.x, goalWorld.z, gc, gr);

    if (sc == gc && sr == gr) return result;

    const int MAX_NODES = 200;
    std::vector<AStarNode> open;
    std::vector<AStarNode> closed;

    // Key for visited set: pack col/row
    auto packKey = [](int c, int r) -> int { return (c + GRID_OFF_X) * 1000 + (r + GRID_OFF_Z); };
    std::vector<int> closedKeys;

    AStarNode start;
    start.col = sc; start.row = sr;
    start.h = (int)floorf(startWorld.y / HEX_HEIGHT);
    start.g = 0.0f;
    start.f = hexDistance(sc, sr, gc, gr);
    start.parentIdx = -1;
    open.push_back(start);

    int totalExpanded = 0;
    while (!open.empty() && totalExpanded < MAX_NODES) {
        // Find lowest f in open
        int bestI = 0;
        for (int i = 1; i < (int)open.size(); i++) {
            if (open[i].f < open[bestI].f) bestI = i;
        }
        AStarNode cur = open[bestI];
        open.erase(open.begin() + bestI);

        // Check if reached goal
        if (cur.col == gc && cur.row == gr) {
            // Reconstruct path
            std::vector<glm::vec3> path;
            AStarNode n = cur;
            while (n.parentIdx >= 0) {
                glm::vec3 wp = hexGridPos(n.col, n.row, 0.0f);
                wp.y = getGroundY(n.col, n.row);
                path.push_back(wp);
                n = closed[n.parentIdx];
            }
            // Reverse (goal->start -> start->goal)
            for (int i = (int)path.size() - 1; i >= 0; i--)
                result.push_back(path[i]);
            return result;
        }

        int closedIdx = (int)closed.size();
        closed.push_back(cur);
        closedKeys.push_back(packKey(cur.col, cur.row));
        totalExpanded++;

        // Expand neighbors
        int neighbors[6][2];
        int ncount;
        getHexNeighbors(cur.col, cur.row, neighbors, ncount);
        for (int i = 0; i < ncount; i++) {
            int nc = neighbors[i][0], nr = neighbors[i][1];
            if (!gridInBounds(nc, nr, 0)) continue;

            // Check walkable: need solid ground below and air at feet+head
            int groundH = -1;
            for (int h = cur.h + 1; h >= cur.h - 2 && h >= 0; h--) {
                if (h < GRID_H && getBlock(nc, nr, h) != BLOCK_AIR &&
                    getBlock(nc, nr, h) != BLOCK_WATER) {
                    groundH = h;
                    break;
                }
            }
            if (groundH < 0) continue;
            int feetH = groundH + 1;
            int headH = groundH + 2;
            if (feetH >= GRID_H) continue;
            if (getBlock(nc, nr, feetH) != BLOCK_AIR) continue;
            if (headH < GRID_H && getBlock(nc, nr, headH) != BLOCK_AIR) continue;

            // Skip if in closed
            int key = packKey(nc, nr);
            bool inClosed = false;
            for (auto& ck : closedKeys) if (ck == key) { inClosed = true; break; }
            if (inClosed) continue;

            float newG = cur.g + 1.0f;
            // Check if in open with better g
            bool inOpen = false;
            for (auto& o : open) {
                if (o.col == nc && o.row == nr) {
                    inOpen = true;
                    if (newG < o.g) {
                        o.g = newG;
                        o.f = newG + hexDistance(nc, nr, gc, gr);
                        o.parentIdx = closedIdx;
                        o.h = feetH;
                    }
                    break;
                }
            }
            if (!inOpen) {
                AStarNode nn;
                nn.col = nc; nn.row = nr; nn.h = feetH;
                nn.g = newG;
                nn.f = newG + hexDistance(nc, nr, gc, gr);
                nn.parentIdx = closedIdx;
                open.push_back(nn);
            }
        }
    }
    return result; // empty = no path found
}

// Move mob with collision and auto-stepping
void moveMobWithCollision(Mob& m, float dx, float dz) {
    float newX = m.pos.x + dx;
    float newZ = m.pos.z + dz;
    if (!tryMoveWithStep(m.pos.x, m.pos.y, m.pos.z, newX, newZ)) {
        if (!tryMoveWithStep(m.pos.x, m.pos.y, m.pos.z, newX, m.pos.z)) {
            tryMoveWithStep(m.pos.x, m.pos.y, m.pos.z, m.pos.x, newZ);
        }
    }
}

// Move mob along A* path, returns true if still following
bool followPath(Mob& m, float dt) {
    if (m.path.empty() || m.pathIndex >= (int)m.path.size()) return false;
    glm::vec3 target = m.path[m.pathIndex];
    glm::vec3 dir = target - m.pos;
    dir.y = 0.0f;
    float dist = glm::length(dir);
    if (dist < 0.5f) {
        m.pathIndex++;
        if (m.pathIndex >= (int)m.path.size()) return false;
        target = m.path[m.pathIndex];
        dir = target - m.pos;
        dir.y = 0.0f;
        dist = glm::length(dir);
    }
    if (dist > 0.01f) {
        dir = glm::normalize(dir);
        m.yaw = atan2f(dir.z, dir.x);
        bool isHostile = (m.type == MOB_ZOMBIE || m.type == MOB_SKELETON);
        float speed = (isHostile ? 2.5f : 1.2f) * dt;
        moveMobWithCollision(m, dir.x * speed, dir.z * speed);
        m.walkTime += dt * (isHostile ? 4.0f : 3.0f);
    }
    return true;
}

void spawnMob(MobType type, glm::vec3 pos) {
    if ((int)mobs.size() >= MAX_MOBS) return;
    Mob m;
    m.pos = pos;
    m.yaw = (float)(gridSeed((int)(pos.x * 7), (int)(pos.z * 13)) % 628) / 100.0f;
    m.type = type;
    m.state = MOB_IDLE;
    if (type == MOB_ZOMBIE || type == MOB_SKELETON)
        m.health = 20.0f;
    else if (type == MOB_SHEEP)
        m.health = 8.0f;
    else
        m.health = 8.0f; // chicken, pig
    m.maxHealth = m.health;
    m.stateTimer = 0.0f;
    m.attackCooldown = 0.0f;
    m.walkTime = 0.0f;
    m.alive = true;
    m.hitFlash = 0.0f;
    m.deathTimer = -1.0f; // alive
    m.pathIndex = 0;
    m.pathTimer = 0.0f;
    mobs.push_back(m);
}

void spawnInitialMobs() {
    // Passive mobs in grass zones: chickens, pigs, sheep
    for (int i = 0; i < 12; i++) {
        unsigned int s = gridSeed(i * 17, i * 31 + 5);
        int col = (int)(s % 40) - 20;
        int row = (int)((s >> 8) % 40) - 20;
        glm::vec3 pos = hexGridPos(col, row, 0.0f);
        pos.y = getGroundY(col, row);
        MobType t;
        int r = s % 3;
        if (r == 0) t = MOB_CHICKEN;
        else if (r == 1) t = MOB_PIG;
        else t = MOB_SHEEP;
        spawnMob(t, pos);
    }
}

// Helper: get mob name for printing
const char* getMobName(MobType t) {
    switch (t) {
        case MOB_CHICKEN: return "Chicken";
        case MOB_PIG: return "Pig";
        case MOB_SHEEP: return "Sheep";
        case MOB_ZOMBIE: return "Zombie";
        case MOB_SKELETON: return "Skeleton";
        default: return "Mob";
    }
}

// Draw one body part in mob-local space.
// `base` already carries the mob's position and yaw, so every offset below is
// local: +X is forward (the way the mob is facing), +Z is its left, +Y is up.
void drawPart(const glm::mat4& base, glm::vec3 local, glm::vec3 color, glm::vec3 scale) {
    glm::mat4 m = glm::translate(base, local);
    drawBoxModel(glm::scale(m, scale), color);
}

void drawMob(const Mob& m, float time) {
    // Mob-local frame: +X forward, +Z left, +Y up.
    // The AI walks the mob along (cos yaw, 0, sin yaw) — see the
    // moveMobWithCollision() calls in updateMobs() — and a Y rotation by -yaw
    // maps local +X onto exactly that vector, so -yaw is the angle that makes
    // the model face where it is going. (drawPlayer() uses PI/2 - yaw instead
    // because its model is built facing +Z rather than +X.)
    // Every part below is expressed relative to this frame; nothing may use
    // m.pos directly, or it will not turn with the mob.
    glm::mat4 base = glm::translate(glm::mat4(1.0f), m.pos);
    base = myRotate(base, -m.yaw, glm::vec3(0, 1, 0));

    // Death animation: mob is dying (falling over + fading)
    if (m.deathTimer >= 0.0f) {
        float t = m.deathTimer;
        float fadeAlpha = 1.0f - (t / 1.0f); // fade over 1 second
        if (fadeAlpha <= 0.0f) return;
        // Blending has to be enabled for the `alpha` uniform to do anything —
        // without it the corpse stayed fully opaque and then popped out of
        // existence when the timer expired. Same pattern as drawHexWater().
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        setFloat(shaderProgram, "alpha", fadeAlpha);
        // Tilt the mob to fall over (rotate around its local Z axis, so it
        // topples sideways relative to its own facing)
        float tiltAngle = t * 1.57f; // 90 degrees over ~1s
        if (tiltAngle > 1.57f) tiltAngle = 1.57f;
        // Draw a simplified falling body
        glm::vec3 col;
        if (m.type == MOB_ZOMBIE) col = glm::vec3(0.3f, 0.5f, 0.2f);
        else if (m.type == MOB_SKELETON) col = glm::vec3(0.85f, 0.85f, 0.8f);
        else if (m.type == MOB_PIG) col = glm::vec3(0.9f, 0.6f, 0.6f);
        else if (m.type == MOB_SHEEP) col = glm::vec3(0.95f, 0.95f, 0.9f);
        else col = glm::vec3(0.9f, 0.9f, 0.85f);
        glm::mat4 fall = glm::translate(base, glm::vec3(0, 0.5f, 0));
        fall = myRotate(fall, tiltAngle, glm::vec3(0, 0, 1));
        drawBoxModel(glm::scale(fall, glm::vec3(0.4f, 0.6f, 0.3f)), col);
        setFloat(shaderProgram, "alpha", 1.0f);
        glDisable(GL_BLEND);
        return;
    }

    if (!m.alive) return;

    // Set damage tint if hit
    if (m.hitFlash > 0.0f) {
        setVec3(shaderProgram, "colorTint", glm::vec3(1.0f, 0.15f, 0.1f));
        setFloat(shaderProgram, "colorTintStrength", m.hitFlash / 0.3f);
    }

    if (m.type == MOB_CHICKEN) {
        float bob = sinf(m.walkTime * 4.0f) * 0.05f;
        glm::vec3 feather(0.9f, 0.9f, 0.85f);
        glm::vec3 beak(0.9f, 0.7f, 0.1f);
        // Body
        drawPart(base, {0.0f, 0.32f + bob, 0.0f}, feather, {0.34f, 0.30f, 0.26f});
        // Head — forward of the body and overlapping its top, instead of
        // floating straight above it with a visible air gap
        // (docs/bug_evidence/06_chicken_head_detached.png)
        drawPart(base, {0.15f, 0.50f + bob, 0.0f}, feather, {0.20f, 0.20f, 0.18f});
        // Beak
        drawPart(base, {0.29f, 0.48f + bob, 0.0f}, beak, {0.10f, 0.06f, 0.06f});
        // Comb
        drawPart(base, {0.14f, 0.62f + bob, 0.0f}, glm::vec3(0.8f, 0.1f, 0.1f), {0.07f, 0.09f, 0.05f});
        // Wattle (under the beak)
        drawPart(base, {0.25f, 0.42f + bob, 0.0f}, glm::vec3(0.8f, 0.1f, 0.1f), {0.05f, 0.07f, 0.05f});
        // Tail
        drawPart(base, {-0.19f, 0.42f + bob, 0.0f}, feather, {0.12f, 0.16f, 0.10f});
        // Wings, tucked against the sides
        drawPart(base, {0.0f, 0.34f + bob,  0.15f}, feather, {0.24f, 0.18f, 0.05f});
        drawPart(base, {0.0f, 0.34f + bob, -0.15f}, feather, {0.24f, 0.18f, 0.05f});
        // Legs — hip rotation, the two half a cycle apart. A chicken is a biped,
        // so unlike the pig and sheep there is no diagonal pairing to preserve.
        Gait g = makeGait(m.walkTime * (GAIT_RATE / 3.0f));
        drawLimb(base, {0.0f, 0.20f,  0.07f}, {0, 0, 1}, g.legL, g.kneeL, 0.11f, 0.09f, 0.05f, beak, beak);
        drawLimb(base, {0.0f, 0.20f, -0.07f}, {0, 0, 1}, g.legR, g.kneeR, 0.11f, 0.09f, 0.05f, beak, beak);
    } else if (m.type == MOB_PIG) {
        float bob = sinf(m.walkTime * 3.0f) * 0.03f;
        glm::vec3 pink(0.9f, 0.6f, 0.6f);
        glm::vec3 darkPink(0.8f, 0.5f, 0.5f);
        // Body — long along the forward axis, the way a quadruped is shaped
        drawPart(base, {0.0f, 0.42f + bob, 0.0f}, pink, {0.70f, 0.38f, 0.42f});
        // Head
        drawPart(base, {0.44f, 0.42f + bob, 0.0f}, pink, {0.30f, 0.34f, 0.34f});
        // Snout
        drawPart(base, {0.61f, 0.38f + bob, 0.0f}, darkPink, {0.10f, 0.12f, 0.14f});
        // Ears
        drawPart(base, {0.40f, 0.60f + bob,  0.11f}, darkPink, {0.09f, 0.08f, 0.06f});
        drawPart(base, {0.40f, 0.60f + bob, -0.11f}, darkPink, {0.09f, 0.08f, 0.06f});
        // Curly tail
        drawPart(base, {-0.37f, 0.50f + bob, 0.0f}, darkPink, {0.07f, 0.07f, 0.07f});
        // Legs — diagonal pairs move together, as a quadruped's gait does.
        // Rotating at the hip rather than sliding the whole leg sideways: the
        // foot now scribes an arc and stays under the body, instead of the leg
        // shuttling back and forth as a rigid post.
        // Passive mobs tick walkTime at 3.0/s, hence the different divisor to
        // the humanoids above.
        Gait g = makeGait(m.walkTime * (GAIT_RATE / 3.0f));
        // One diagonal pair takes g.legL, the other g.legR — they are already
        // half a cycle apart, which is exactly the pairing the old code built
        // by hand out of +legSwing and -legSwing.
        drawLimb(base, { 0.22f, 0.26f,  0.15f}, {0, 0, 1}, g.legL, g.kneeL, 0.14f, 0.12f, 0.11f, darkPink, darkPink);
        drawLimb(base, { 0.22f, 0.26f, -0.15f}, {0, 0, 1}, g.legR, g.kneeR, 0.14f, 0.12f, 0.11f, darkPink, darkPink);
        drawLimb(base, {-0.22f, 0.26f,  0.15f}, {0, 0, 1}, g.legR, g.kneeR, 0.14f, 0.12f, 0.11f, darkPink, darkPink);
        drawLimb(base, {-0.22f, 0.26f, -0.15f}, {0, 0, 1}, g.legL, g.kneeL, 0.14f, 0.12f, 0.11f, darkPink, darkPink);
    } else if (m.type == MOB_SHEEP) {
        // White woolly body, dark face and legs
        float bob = sinf(m.walkTime * 3.0f) * 0.03f;
        glm::vec3 wool(0.95f, 0.95f, 0.9f);
        glm::vec3 face(0.35f, 0.3f, 0.25f);
        // Body (big woolly block)
        drawPart(base, {0.0f, 0.55f + bob, 0.0f}, wool, {0.72f, 0.44f, 0.46f});
        // Neck ruff, then the head clear of the body. The head used to sit at
        // x=0.35 y=0.5 with the body 0.45 wide and 0.4 tall, which put it
        // inside the body's silhouette so only the ears showed
        // (docs/bug_evidence/05_sheep_head_inside_body.png).
        drawPart(base, {0.40f, 0.62f + bob, 0.0f}, wool, {0.20f, 0.22f, 0.24f});
        drawPart(base, {0.57f, 0.55f + bob, 0.0f}, face, {0.22f, 0.26f, 0.22f});
        // Ears
        drawPart(base, {0.52f, 0.68f + bob,  0.13f}, face, {0.07f, 0.05f, 0.09f});
        drawPart(base, {0.52f, 0.68f + bob, -0.13f}, face, {0.07f, 0.05f, 0.09f});
        // Legs (dark) — hip rotation, diagonal pairs together. See the pig.
        Gait g = makeGait(m.walkTime * (GAIT_RATE / 3.0f));
        drawLimb(base, { 0.22f, 0.34f,  0.16f}, {0, 0, 1}, g.legL, g.kneeL, 0.18f, 0.16f, 0.10f, face, face);
        drawLimb(base, { 0.22f, 0.34f, -0.16f}, {0, 0, 1}, g.legR, g.kneeR, 0.18f, 0.16f, 0.10f, face, face);
        drawLimb(base, {-0.22f, 0.34f,  0.16f}, {0, 0, 1}, g.legR, g.kneeR, 0.18f, 0.16f, 0.10f, face, face);
        drawLimb(base, {-0.22f, 0.34f, -0.16f}, {0, 0, 1}, g.legL, g.kneeL, 0.18f, 0.16f, 0.10f, face, face);
    } else if (m.type == MOB_ZOMBIE) {
        // walkTime ticks at 4.0/s for a hostile mob (see moveMobTowards above),
        // so this is the conversion into the shared GAIT_RATE cadence.
        Gait g = makeGait(m.walkTime * (GAIT_RATE / 4.0f));
        // Bob and sway ride on a copy of the frame, so the limbs below inherit
        // them. Roll is about the forward axis (+X in the mob frame), which
        // rocks the torso over whichever leg is carrying the weight.
        glm::mat4 body = glm::translate(base, glm::vec3(0.0f, g.bob, 0.0f));
        body = myRotate(body, g.sway, glm::vec3(1, 0, 0));

        glm::vec3 zGreen(0.3f, 0.5f, 0.2f);
        glm::vec3 zDark(0.2f, 0.35f, 0.15f);
        glm::vec3 zShirt(0.25f, 0.4f, 0.3f);
        // Torso, split into shoulders / chest / belt instead of one slab. The
        // step out across the shoulders is what gives a blocky humanoid a
        // readable silhouette; the single box read as a fridge.
        drawPart(body, {0.0f, 1.19f, 0.0f}, zShirt, {0.28f, 0.16f, 0.54f});
        drawPart(body, {0.0f, 0.94f, 0.0f}, zShirt, {0.26f, 0.36f, 0.46f});
        drawPart(body, {0.0f, 0.70f, 0.0f}, zDark,  {0.26f, 0.14f, 0.44f});
        // Neck, so the head is not balanced flat on the shoulders
        drawPart(body, {0.0f, 1.31f, 0.0f}, zDark, {0.16f, 0.10f, 0.18f});
        // Head
        drawPart(body, {0.0f, 1.52f, 0.0f}, zDark, {0.42f, 0.45f, 0.42f});
        // Eyes on the front face, so its facing reads at a glance
        drawPart(body, {0.21f, 1.57f,  0.11f}, glm::vec3(0.55f, 0.1f, 0.1f), {0.03f, 0.07f, 0.09f});
        drawPart(body, {0.21f, 1.57f, -0.11f}, glm::vec3(0.55f, 0.1f, 0.1f), {0.03f, 0.07f, 0.09f});

        // Arms. The zombie pose is both arms held out front, which in a real
        // hierarchy is just a large constant forward rotation at the shoulder:
        // +1.45 rad lifts a limb that hangs straight down to near horizontal.
        // The walk swing rides on top at a third amplitude — at the full 0.40
        // the outstretched arms flap rather than sway.
        const float ZOMBIE_REACH = 1.45f;
        // Positive bend folds the forearm backward, which from a horizontal arm
        // means downward, so the hands droop instead of the arm being one bar.
        drawLimb(body, {0.0f, 1.19f,  0.32f}, {0, 0, 1},
                 ZOMBIE_REACH + g.armL * 0.35f, 0.15f,
                 0.30f, 0.26f, 0.16f, zShirt, zGreen);
        drawLimb(body, {0.0f, 1.19f, -0.32f}, {0, 0, 1},
                 ZOMBIE_REACH + g.armR * 0.35f, 0.15f,
                 0.30f, 0.26f, 0.16f, zShirt, zGreen);

        // Legs. Hip sits at the bottom of the belt, and thigh + shin together
        // span the same 0.65 the old single leg box did, so the feet still meet
        // the ground at y = 0 when the knee is straight.
        drawLimb(body, {0.0f, 0.65f,  0.11f}, {0, 0, 1}, g.legL, g.kneeL,
                 0.34f, 0.31f, 0.16f, zDark, zDark);
        drawLimb(body, {0.0f, 0.65f, -0.11f}, {0, 0, 1}, g.legR, g.kneeR,
                 0.34f, 0.31f, 0.16f, zDark, zDark);
    } else if (m.type == MOB_SKELETON) {
        // Gray/white bony humanoid with bow
        Gait g = makeGait(m.walkTime * (GAIT_RATE / 4.0f));
        glm::mat4 body = glm::translate(base, glm::vec3(0.0f, g.bob, 0.0f));
        body = myRotate(body, g.sway, glm::vec3(1, 0, 0));
        glm::vec3 bone(0.85f, 0.85f, 0.8f);
        glm::vec3 dark(0.15f, 0.15f, 0.15f); // eye sockets
        glm::vec3 bowCol(0.5f, 0.3f, 0.15f);
        // Body (ribcage — thinner than zombie)
        drawPart(body, {0.0f, 0.975f, 0.0f}, bone, {0.24f, 0.62f, 0.40f});
        // Neck vertebra
        drawPart(body, {0.0f, 1.28f, 0.0f}, bone, {0.12f, 0.10f, 0.14f});
        // Head (skull)
        drawPart(body, {0.0f, 1.50f, 0.0f}, bone, {0.40f, 0.42f, 0.40f});
        // Eye sockets, sunk into the front of the skull
        drawPart(body, {0.19f, 1.54f,  0.10f}, dark, {0.04f, 0.09f, 0.10f});
        drawPart(body, {0.19f, 1.54f, -0.10f}, dark, {0.04f, 0.09f, 0.10f});

        // Arms held forward at the ready, less raised than the zombie's — a
        // skeleton is aiming a bow, not lunging.
        const float SKELE_REACH = 1.30f;
        drawLimb(body, {0.0f, 1.22f,  0.24f}, {0, 0, 1},
                 SKELE_REACH + g.armL * 0.35f, 0.12f,
                 0.25f, 0.21f, 0.12f, bone, bone);
        // The right arm's wrist frame comes back out, so the bow inherits both
        // the shoulder and the elbow and stays in the hand while the arm moves.
        // Hanging it off `base` at a fixed offset — which is what the old code
        // did — left it floating beside the arm as soon as the arm swung.
        glm::mat4 wrist = drawLimb(body, {0.0f, 1.22f, -0.24f}, {0, 0, 1},
                                   SKELE_REACH + g.armR * 0.35f, 0.12f,
                                   0.25f, 0.21f, 0.12f, bone, bone);
        // Bow, stood upright in the hand. Rotated back out of the arm's frame:
        // the arm points forward, so the limb's local down is world forward, and
        // without this the bow would lie flat along the direction of aim.
        glm::mat4 mBow = myRotate(wrist, -SKELE_REACH, glm::vec3(0, 0, 1));
        drawBoxModel(glm::scale(mBow, glm::vec3(0.05f, 0.50f, 0.05f)), bowCol);

        // Legs (thin). Thigh + shin span the 0.68 the old leg box did.
        drawLimb(body, {0.0f, 0.68f,  0.09f}, {0, 0, 1}, g.legL, g.kneeL,
                 0.36f, 0.32f, 0.13f, bone, bone);
        drawLimb(body, {0.0f, 0.68f, -0.09f}, {0, 0, 1}, g.legR, g.kneeR,
                 0.36f, 0.32f, 0.13f, bone, bone);
    }

    // Health bar above head if damaged (all mobs). Drawn in world axes, not
    // mob-local — a bar that swings around with the mob's facing is harder to
    // read than one that always hangs straight above it.
    if (m.health < m.maxHealth && m.alive) {
        float hFrac = m.health / m.maxHealth;
        float headY = (m.type == MOB_ZOMBIE || m.type == MOB_SKELETON) ? 2.0f :
                       (m.type == MOB_SHEEP || m.type == MOB_PIG) ? 0.95f : 0.8f;
        drawHex(m.pos + glm::vec3(0, headY, 0), glm::vec3(0.8f, 0.1f, 0.1f), glm::vec3(0.3f * hFrac, 0.04f, 0.04f));
    }

    // Reset tint
    if (m.hitFlash > 0.0f) {
        setVec3(shaderProgram, "colorTint", glm::vec3(0.0f));
        setFloat(shaderProgram, "colorTintStrength", 0.0f);
    }
}

// --- Arrow projectiles (skeleton ranged attack) ---
void spawnArrow(glm::vec3 from, glm::vec3 target) {
    Arrow a;
    a.pos = from + glm::vec3(0, 1.2f, 0); // shoot from chest height
    glm::vec3 dir = target - a.pos;
    dir.y += 1.0f; // arc upward slightly
    if (glm::length(dir) > 0.01f) dir = glm::normalize(dir);
    a.vel = dir * 12.0f; // arrow speed
    a.lifetime = 0.0f;
    a.active = true;
    arrows.push_back(a);
}

void updateArrows(float dt) {
    for (int i = (int)arrows.size() - 1; i >= 0; i--) {
        Arrow& a = arrows[i];
        if (!a.active) { arrows.erase(arrows.begin() + i); continue; }
        a.lifetime += dt;
        a.vel.y -= 9.8f * dt; // gravity
        a.pos += a.vel * dt;

        // Hit player?
        float distToPlayer = glm::length(a.pos - (playerWorldPos + glm::vec3(0, 0.9f, 0)));
        if (distToPlayer < 1.0f) {
            playerHealth -= 3.0f;
            if (playerHealth < 0.0f) playerHealth = 0.0f;
            printf("[Combat] Arrow hit you! Health: %.0f\n", playerHealth);
            if (playerHealth <= 0.0f && !playerDead) {
                playerDead = true;
                printf("[Player] YOU DIED! Press P to respawn.\n");
            }
            a.active = false;
            continue;
        }

        // Hit ground or timeout?
        int ac, ar;
        worldToColRow(a.pos.x, a.pos.z, ac, ar);
        float groundY = getGroundYWorld(a.pos.x, a.pos.z);
        if (a.pos.y <= groundY || a.lifetime > 5.0f) {
            a.active = false;
        }
    }
}

void drawArrows() {
    for (auto& a : arrows) {
        if (!a.active) continue;
        // Arrow: thin brown stick, shaft along local +X so it uses the same
        // frame as the mobs. The old call rotated a Z-aligned shaft by +yaw and
        // never applied `pitch` at all, so arrows flew broadside-on and stayed
        // level while their actual path arced.
        glm::vec3 dir = glm::normalize(a.vel);
        float yaw = atan2f(dir.z, dir.x);
        float pitch = asinf(dir.y);
        glm::mat4 m = glm::translate(glm::mat4(1.0f), a.pos);
        m = myRotate(m, -yaw, glm::vec3(0, 1, 0));
        m = myRotate(m, pitch, glm::vec3(0, 0, 1));
        drawBoxModel(glm::scale(m, glm::vec3(0.5f, 0.035f, 0.035f)), glm::vec3(0.5f, 0.35f, 0.15f));
    }
}

void updateMobs(float dt, float time) {
    // Update player attack cooldown
    playerAttackCooldown -= dt;
    if (playerAttackCooldown < 0.0f) playerAttackCooldown = 0.0f;

    // Spawn hostile mobs at night
    mobSpawnTimer += dt;
    if (dayFactor < 0.3f && mobSpawnTimer > 5.0f) {
        mobSpawnTimer = 0.0f;
        int hostileCount = 0;
        for (auto& m : mobs) if ((m.type == MOB_ZOMBIE || m.type == MOB_SKELETON) && m.alive) hostileCount++;
        if (hostileCount < 10) {
            unsigned int s = gridSeed((int)(time * 100), (int)(playerWorldPos.x * 7));
            float angle = (float)(s % 628) / 100.0f;
            float dist = 15.0f + (float)(s % 10);
            glm::vec3 spawnPos = playerWorldPos + glm::vec3(cosf(angle) * dist, 0, sinf(angle) * dist);
            int sc, sr;
            worldToColRow(spawnPos.x, spawnPos.z, sc, sr);
            spawnPos.y = getGroundY(sc, sr);
            // Torch safe zone check
            bool nearTorch = false;
            for (auto& tp : torchPositions) {
                if (glm::length(tp.pos - spawnPos) < 5.0f) { nearTorch = true; break; }
            }
            if (!nearTorch && glm::length(spawnPos - playerWorldPos) > 8.0f) {
                // Alternate zombie and skeleton
                MobType spawnType = (s % 3 == 0) ? MOB_SKELETON : MOB_ZOMBIE;
                spawnMob(spawnType, spawnPos);
                printf("[Mob] %s spawned!\n", getMobName(spawnType));
            }
        }
    }

    // Despawn hostile mobs during day
    if (dayFactor > 0.5f) {
        for (int i = (int)mobs.size() - 1; i >= 0; i--) {
            if (mobs[i].type == MOB_ZOMBIE || mobs[i].type == MOB_SKELETON) {
                mobs.erase(mobs.begin() + i);
            }
        }
    }

    // Update death animations + remove finished deaths
    for (int i = (int)mobs.size() - 1; i >= 0; i--) {
        if (mobs[i].deathTimer >= 0.0f) {
            mobs[i].deathTimer += dt;
            if (mobs[i].deathTimer > 1.0f) {
                mobs.erase(mobs.begin() + i);
            }
            continue; // skip normal update for dying mobs
        }
    }

    for (auto& m : mobs) {
        if (!m.alive || m.deathTimer >= 0.0f) continue;
        m.stateTimer += dt;
        m.attackCooldown -= dt;
        if (m.attackCooldown < 0.0f) m.attackCooldown = 0.0f;
        m.hitFlash -= dt;
        if (m.hitFlash < 0.0f) m.hitFlash = 0.0f;
        m.pathTimer += dt;

        float distToPlayer = glm::length(m.pos - playerWorldPos);

        if (m.type == MOB_ZOMBIE) {
            // Hostile: chase player within 16 blocks, use A* pathfinding
            if (distToPlayer < 16.0f && !playerDead) {
                m.state = MOB_CHASE;
                // Recalculate path every 1.5 seconds
                if (m.pathTimer > 1.5f) {
                    m.pathTimer = 0.0f;
                    m.path = findPath(m.pos, playerWorldPos);
                    m.pathIndex = 0;
                }
                if (!followPath(m, dt)) {
                    // Direct approach if no path
                    glm::vec3 dir = playerWorldPos - m.pos;
                    dir.y = 0.0f;
                    if (glm::length(dir) > 0.5f) {
                        dir = glm::normalize(dir);
                        m.yaw = atan2f(dir.z, dir.x);
                        moveMobWithCollision(m, dir.x * 2.5f * dt, dir.z * 2.5f * dt);
                        m.walkTime += dt * 4.0f;
                    }
                }
                // Attack player when close
                if (distToPlayer < 1.5f && m.attackCooldown <= 0.0f) {
                    playerHealth -= 3.0f;
                    if (playerHealth < 0.0f) playerHealth = 0.0f;
                    m.attackCooldown = 1.0f;
                    printf("[Mob] Zombie hit you! Health: %.0f\n", playerHealth);
                    if (playerHealth <= 0.0f && !playerDead) {
                        playerDead = true;
                        printf("[Player] YOU DIED! Press P to respawn.\n");
                    }
                }
            } else {
                m.state = MOB_WANDER;
                m.path.clear();
                if (m.stateTimer > 3.0f) {
                    m.stateTimer = 0.0f;
                    unsigned int s = gridSeed((int)(m.pos.x * 10 + time * 100), (int)(m.pos.z * 10));
                    m.state = (s % 3 == 0) ? MOB_WANDER : MOB_IDLE;
                    if (m.state == MOB_WANDER) m.yaw = (float)(s % 628) / 100.0f;
                }
                if (m.state == MOB_WANDER) {
                    moveMobWithCollision(m, cosf(m.yaw) * 1.5f * dt, sinf(m.yaw) * 1.5f * dt);
                    m.walkTime += dt * 3.0f;
                }
            }
        } else if (m.type == MOB_SKELETON) {
            // Ranged hostile: approach to 8-15 range, shoot arrows
            if (distToPlayer < 20.0f && !playerDead) {
                m.state = MOB_CHASE;
                // Recalculate path every 2 seconds
                if (m.pathTimer > 2.0f) {
                    m.pathTimer = 0.0f;
                    // If too far, path toward player; if in range, stay put
                    if (distToPlayer > 15.0f) {
                        m.path = findPath(m.pos, playerWorldPos);
                        m.pathIndex = 0;
                    } else {
                        m.path.clear(); // in range, stop moving
                    }
                }

                if (distToPlayer > 15.0f) {
                    if (!followPath(m, dt)) {
                        glm::vec3 dir = playerWorldPos - m.pos;
                        dir.y = 0.0f;
                        if (glm::length(dir) > 0.5f) {
                            dir = glm::normalize(dir);
                            m.yaw = atan2f(dir.z, dir.x);
                            moveMobWithCollision(m, dir.x * 2.0f * dt, dir.z * 2.0f * dt);
                            m.walkTime += dt * 3.5f;
                        }
                    }
                } else if (distToPlayer < 6.0f) {
                    // Too close — back away
                    glm::vec3 dir = m.pos - playerWorldPos;
                    dir.y = 0.0f;
                    if (glm::length(dir) > 0.1f) {
                        dir = glm::normalize(dir);
                        m.yaw = atan2f(-dir.z, -dir.x); // face player while retreating
                        moveMobWithCollision(m, dir.x * 1.5f * dt, dir.z * 1.5f * dt);
                        m.walkTime += dt * 3.0f;
                    }
                } else {
                    // In range: face player
                    glm::vec3 dir = playerWorldPos - m.pos;
                    dir.y = 0.0f;
                    if (glm::length(dir) > 0.1f) m.yaw = atan2f(dir.z, dir.x);
                }

                // Shoot arrows at 8-15 block range
                if (distToPlayer >= 5.0f && distToPlayer <= 18.0f && m.attackCooldown <= 0.0f) {
                    spawnArrow(m.pos, playerWorldPos + glm::vec3(0, 0.9f, 0));
                    m.attackCooldown = 2.0f; // slower than zombie melee
                    printf("[Mob] Skeleton shoots an arrow!\n");
                }
            } else {
                m.state = MOB_WANDER;
                m.path.clear();
                if (m.stateTimer > 3.0f) {
                    m.stateTimer = 0.0f;
                    unsigned int s = gridSeed((int)(m.pos.x * 10 + time * 100), (int)(m.pos.z * 10));
                    m.state = (s % 3 == 0) ? MOB_WANDER : MOB_IDLE;
                    if (m.state == MOB_WANDER) m.yaw = (float)(s % 628) / 100.0f;
                }
                if (m.state == MOB_WANDER) {
                    moveMobWithCollision(m, cosf(m.yaw) * 1.5f * dt, sinf(m.yaw) * 1.5f * dt);
                    m.walkTime += dt * 3.0f;
                }
            }
        } else {
            // Passive mobs: wander randomly, flee when attacked
            if (m.state == MOB_FLEE) {
                // Run away from player for 3 seconds
                if (m.stateTimer > 3.0f) {
                    m.state = MOB_IDLE;
                    m.stateTimer = 0.0f;
                } else {
                    glm::vec3 dir = m.pos - playerWorldPos;
                    dir.y = 0.0f;
                    if (glm::length(dir) > 0.1f) {
                        dir = glm::normalize(dir);
                        m.yaw = atan2f(dir.z, dir.x);
                        float fleeSpeed = 3.0f * dt;
                        moveMobWithCollision(m, dir.x * fleeSpeed, dir.z * fleeSpeed);
                        m.walkTime += dt * 5.0f;
                    }
                }
            } else {
                if (m.stateTimer > 3.0f) {
                    m.stateTimer = 0.0f;
                    unsigned int s = gridSeed((int)(m.pos.x * 10 + time * 100), (int)(m.pos.z * 10));
                    m.state = (s % 3 == 0) ? MOB_WANDER : MOB_IDLE;
                    if (m.state == MOB_WANDER) {
                        m.yaw = (float)(s % 628) / 100.0f;
                    }
                }
                if (m.state == MOB_WANDER) {
                    float speed = 1.2f * dt;
                    moveMobWithCollision(m, cosf(m.yaw) * speed, sinf(m.yaw) * speed);
                    m.walkTime += dt * 3.0f;
                }
            }
        }

        // Snap to ground with basic gravity
        int mc, mr;
        worldToColRow(m.pos.x, m.pos.z, mc, mr);
        float groundY = getGroundYAtHeight(mc, mr, m.pos.y + 0.6f);
        if (m.pos.y > groundY) {
            m.pos.y -= 12.0f * dt;
            if (m.pos.y < groundY) m.pos.y = groundY;
        } else {
            m.pos.y = groundY;
        }
    }

    // Update arrows
    updateArrows(dt);

    // Handle dying mobs (deathTimer started but not yet removed — handled above)
    // Drop loot for newly killed mobs (alive=false, deathTimer not started)
    for (auto& m : mobs) {
        if (!m.alive && m.deathTimer < 0.0f) {
            m.deathTimer = 0.0f; // start death animation
            // Drop loot
            if (m.type == MOB_PIG) {
                playerHunger += 4.0f;
                if (playerHunger > playerMaxHunger) playerHunger = playerMaxHunger;
                printf("[Mob] Pig dropped food! Hunger restored.\n");
            } else if (m.type == MOB_CHICKEN) {
                playerHunger += 2.0f;
                if (playerHunger > playerMaxHunger) playerHunger = playerMaxHunger;
                printf("[Mob] Chicken dropped food! Hunger restored.\n");
            } else if (m.type == MOB_SHEEP) {
                // Drop wool (white wool block)
                int sc, sr;
                worldToColRow(m.pos.x, m.pos.z, sc, sr);
                int h = (int)(m.pos.y / HEX_HEIGHT);
                spawnItemDrop(sc, sr, h, BLOCK_WOOL_WHITE);
                printf("[Mob] Sheep dropped wool!\n");
            } else if (m.type == MOB_ZOMBIE) {
                // Drop rotten flesh (represented as dirt for now)
                int sc, sr;
                worldToColRow(m.pos.x, m.pos.z, sc, sr);
                int h = (int)(m.pos.y / HEX_HEIGHT);
                spawnItemDrop(sc, sr, h, BLOCK_DIRT);
                printf("[Mob] Zombie dropped rotten flesh!\n");
            } else if (m.type == MOB_SKELETON) {
                // Drop bones (sticks) and arrows (sticks)
                int sc, sr;
                worldToColRow(m.pos.x, m.pos.z, sc, sr);
                int h = (int)(m.pos.y / HEX_HEIGHT);
                spawnItemDrop(sc, sr, h, BLOCK_STONE); // bones proxy
                printf("[Mob] Skeleton dropped bones!\n");
            }
        }
    }
}

// 15C: Combat — left-click on mob to attack (with cooldown)
void attackMob(glm::vec3 camOrigin, glm::vec3 camDir) {
    // Check player attack cooldown
    if (playerAttackCooldown > 0.0f) return;

    float bestDist = 4.0f; // melee range
    int bestIdx = -1;
    for (int i = 0; i < (int)mobs.size(); i++) {
        if (!mobs[i].alive || mobs[i].deathTimer >= 0.0f) continue;
        glm::vec3 toMob = mobs[i].pos + glm::vec3(0, 0.5f, 0) - camOrigin;
        float d = glm::length(toMob);
        if (d > bestDist) continue;
        float dot = glm::dot(glm::normalize(toMob), camDir);
        if (dot > 0.85f && d < bestDist) {
            bestDist = d;
            bestIdx = i;
        }
    }
    if (bestIdx >= 0) {
        int heldType = playerInventory[hotbarSlot].type;
        float damage = getToolDamage(heldType);
        mobs[bestIdx].health -= damage;
        mobs[bestIdx].hitFlash = 0.3f; // red tint for 0.3s

        // Player attack cooldown (swords: 0.625s, axes: 1.0s, others: 0.4s)
        if (isSword(heldType)) playerAttackCooldown = 0.625f;
        else if (isAxe(heldType)) playerAttackCooldown = 1.0f;
        else playerAttackCooldown = 0.4f;

        // Knockback
        float kbStr = isSword(heldType) ? 2.0f : 1.5f;
        glm::vec3 kb = glm::normalize(mobs[bestIdx].pos - camOrigin);
        kb.y = 0.2f;
        mobs[bestIdx].pos += kb * kbStr;

        printf("[Combat] Hit %s with %s for %.0f damage! Health: %.0f\n",
            getMobName(mobs[bestIdx].type),
            getBlockName(heldType),
            damage,
            mobs[bestIdx].health);

        // Degrade tool durability on hit
        InventorySlot& held = playerInventory[hotbarSlot];
        if (held.durability > 0) {
            held.durability--;
            if (held.durability <= 0) {
                printf("[Tool] %s broke!\n", getBlockName(held.type));
                held = {BLOCK_AIR, 0, -1};
            }
        }

        // Passive mobs flee when attacked
        if (mobs[bestIdx].type == MOB_CHICKEN || mobs[bestIdx].type == MOB_PIG ||
            mobs[bestIdx].type == MOB_SHEEP) {
            mobs[bestIdx].state = MOB_FLEE;
            mobs[bestIdx].stateTimer = 0.0f;
            // Alert nearby passive mobs too
            for (auto& m2 : mobs) {
                if (&m2 == &mobs[bestIdx]) continue;
                if (m2.type == MOB_CHICKEN || m2.type == MOB_PIG || m2.type == MOB_SHEEP) {
                    if (glm::length(m2.pos - mobs[bestIdx].pos) < 8.0f) {
                        m2.state = MOB_FLEE;
                        m2.stateTimer = 0.0f;
                    }
                }
            }
        }

        if (mobs[bestIdx].health <= 0.0f) {
            mobs[bestIdx].alive = false;
            printf("[Combat] Killed!\n");
        }
    }
}

// =====================================================
// Birds — init, update, draw
// =====================================================
void initBirds() {
    birds.clear();
    srand(12345);
    for (int i = 0; i < 6; i++) {
        Bird b;
        b.pos = glm::vec3(
            (rand() % 100 - 50) * 0.5f,
            15.0f + (rand() % 10),
            (rand() % 100 - 50) * 0.5f
        );
        // Generate random waypoints
        for (int w = 0; w < 6; w++) {
            b.waypoints[w] = glm::vec3(
                (rand() % 120 - 60) * 0.5f,
                12.0f + (rand() % 8),
                (rand() % 120 - 60) * 0.5f
            );
        }
        b.currentWP = 0;
        b.t = 0.0f;
        b.wingPhase = (float)(rand() % 1000) / 1000.0f * 6.28f;
        b.speed = 3.0f + (rand() % 20) * 0.1f;
        b.color = glm::vec3(0.15f + (rand()%30)*0.01f, 0.12f + (rand()%20)*0.01f, 0.1f + (rand()%15)*0.01f);
        b.yaw = 0.0f; // set on the first updateBirds() tick from actual motion
        birds.push_back(b);
    }
    printf("[Birds] Spawned %d birds\n", (int)birds.size());
}

void updateBirds(float dt, float time) {
    for (auto& b : birds) {
        b.t += b.speed * dt * 0.3f;
        if (b.t >= 1.0f) {
            b.t -= 1.0f;
            b.currentWP = (b.currentWP + 1) % 4; // cycle through waypoints
            // Regenerate far waypoints for variety
            int futureWP = (b.currentWP + 3) % 6;
            b.waypoints[futureWP] = glm::vec3(
                (rand() % 120 - 60) * 0.5f,
                12.0f + (rand() % 8),
                (rand() % 120 - 60) * 0.5f
            );
        }
        // Catmull-Rom spline interpolation between waypoints
        int i0 = b.currentWP % 6;
        int i1 = (b.currentWP + 1) % 6;
        int i2 = (b.currentWP + 2) % 6;
        int i3 = (b.currentWP + 3) % 6;
        glm::vec3 prev = b.pos;
        b.pos = catmullRom(b.waypoints[i0], b.waypoints[i1], b.waypoints[i2], b.waypoints[i3], b.t);
        // Heading from actual motion along the spline, so the bird points the
        // way it is flying instead of always facing -Z. Same convention as the
        // mobs: yaw = atan2(dz, dx), forward is (cos yaw, 0, sin yaw).
        glm::vec3 step = b.pos - prev;
        if (step.x * step.x + step.z * step.z > 1e-8f) b.yaw = atan2f(step.z, step.x);
        b.wingPhase += dt * 12.0f; // wing flap speed
    }
}

void drawBird(const Bird& b, float time) {
    // Same local frame as the mobs: +X forward, +Z left, +Y up. The wings used
    // to hinge around Z with the body's long axis on Z, so a bird flying along
    // X flew sideways with its wings edge-on to the direction of travel.
    glm::mat4 base = glm::translate(glm::mat4(1.0f), b.pos);
    base = myRotate(base, -b.yaw, glm::vec3(0, 1, 0));
    // Body
    drawPart(base, {0.0f, 0.0f, 0.0f}, b.color, {0.34f, 0.12f, 0.14f});
    // Head and beak
    drawPart(base, {0.21f, 0.05f, 0.0f}, b.color * 1.1f, {0.12f, 0.12f, 0.12f});
    drawPart(base, {0.31f, 0.04f, 0.0f}, glm::vec3(0.9f, 0.7f, 0.1f), {0.09f, 0.04f, 0.04f});
    // Tail
    drawPart(base, {-0.23f, 0.02f, 0.0f}, b.color, {0.14f, 0.025f, 0.12f});
    // Wings — flap using sin(wingPhase), hinged at the shoulder with myRotate
    float wingAngle = sinf(b.wingPhase) * 0.7f; // radians
    glm::vec3 wingColor = b.color + glm::vec3(0.1f, 0.1f, 0.15f);
    // Left wing (+Z)
    glm::mat4 wingL = glm::translate(base, glm::vec3(0.0f, 0.03f, 0.06f));
    wingL = myRotate(wingL, wingAngle, glm::vec3(1, 0, 0));
    wingL = glm::translate(wingL, glm::vec3(0, 0, 0.16f));
    drawBoxModel(glm::scale(wingL, glm::vec3(0.22f, 0.025f, 0.30f)), wingColor);
    // Right wing (-Z), mirrored so both wings rise and fall together
    glm::mat4 wingR = glm::translate(base, glm::vec3(0.0f, 0.03f, -0.06f));
    wingR = myRotate(wingR, -wingAngle, glm::vec3(1, 0, 0));
    wingR = glm::translate(wingR, glm::vec3(0, 0, -0.16f));
    drawBoxModel(glm::scale(wingR, glm::vec3(0.22f, 0.025f, 0.30f)), wingColor);
}

// =====================================================
// Window — frame + two panes that swing open
// =====================================================
void drawWindow(glm::vec3 pos, float angle, float facing = 0.0f) {
    glm::vec3 frame(0.35f, 0.3f, 0.2f); // brown wood
    glm::vec3 glass(0.5f, 0.7f, 0.9f);  // light blue glass

    // Frame (4 sides)
    drawHex(pos + glm::vec3(0, 0.6f, 0), frame, glm::vec3(0.7f, 0.05f, 0.05f));  // top
    drawHex(pos + glm::vec3(0, -0.6f, 0), frame, glm::vec3(0.7f, 0.05f, 0.05f)); // bottom
    drawHex(pos + glm::vec3(-0.35f, 0, 0), frame, glm::vec3(0.05f, 1.2f, 0.05f)); // left
    drawHex(pos + glm::vec3(0.35f, 0, 0), frame, glm::vec3(0.05f, 1.2f, 0.05f));  // right
    drawHex(pos, frame, glm::vec3(0.05f, 1.2f, 0.05f)); // center divider

    // Left pane (pivots on left frame edge using myRotate)
    float rad = glm::radians(angle);
    glm::mat4 leftPane = glm::translate(glm::mat4(1.0f), pos + glm::vec3(-0.17f, 0, 0));
    leftPane = myRotate(leftPane, glm::radians(facing), glm::vec3(0, 1, 0));
    leftPane = myRotate(leftPane, -rad, glm::vec3(0, 1, 0)); // swing open
    leftPane = glm::translate(leftPane, glm::vec3(-0.085f, 0, 0));
    leftPane = glm::scale(leftPane, glm::vec3(0.15f, 1.0f, 0.02f));
    // Semi-transparent glass
    setFloat(shaderProgram, "alpha", 0.5f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    drawHexModel(leftPane, glass);

    // Right pane (pivots on right frame edge)
    glm::mat4 rightPane = glm::translate(glm::mat4(1.0f), pos + glm::vec3(0.17f, 0, 0));
    rightPane = myRotate(rightPane, glm::radians(facing), glm::vec3(0, 1, 0));
    rightPane = myRotate(rightPane, rad, glm::vec3(0, 1, 0)); // swing open other way
    rightPane = glm::translate(rightPane, glm::vec3(0.085f, 0, 0));
    rightPane = glm::scale(rightPane, glm::vec3(0.15f, 1.0f, 0.02f));
    drawHexModel(rightPane, glass);
    glDisable(GL_BLEND);
    setFloat(shaderProgram, "alpha", 1.0f);
}

// =====================================================
// Fractal Tree — recursive branching
// =====================================================
void drawFractalBranch(glm::vec3 pos, glm::vec3 dir, float length, float thickness, int depth, int maxDepth) {
    if (depth > maxDepth || length < 0.05f) return;

    // Draw this branch segment as a hex
    glm::vec3 endPos = pos + dir * length;
    glm::vec3 midPos = (pos + endPos) * 0.5f;

    // Branch color: brown for trunk, green for leaves
    glm::vec3 color = (depth < maxDepth - 1) ?
        glm::vec3(0.35f, 0.22f, 0.1f) : // wood
        glm::vec3(0.1f + (float)(depth % 3) * 0.1f, 0.5f + (float)(depth % 2) * 0.2f, 0.15f); // leaf green

    // Two things were wrong with this one draw call, both masked by the NaN bug
    // below that stopped any child branch from ever rendering:
    //
    //   1. scale.y multiplies HEX_HEIGHT, which is 1.0, so scale.y IS the segment
    //      length. It passed length * 0.5f — half length, leaving a gap between
    //      each segment and the children that attach at pos + dir * length.
    //   2. drawHex applies translate * scale only. A branch running diagonally was
    //      still drawn as an upright prism, so a tree came out as a cloud of
    //      floating vertical sticks.
    //
    // Align the prism's +Y with the branch direction and give it its full length.
    glm::mat4 seg = glm::translate(glm::mat4(1.0f), midPos);
    {
        float d = glm::dot(glm::vec3(0, 1, 0), dir);
        d = (d < -1.0f) ? -1.0f : (d > 1.0f ? 1.0f : d);
        glm::vec3 axis = glm::cross(glm::vec3(0, 1, 0), dir);
        // Parallel: nothing to rotate. Antiparallel: cross is zero but the segment
        // does need flipping, and any perpendicular axis will do.
        if (glm::length(axis) > 1e-5f)  seg = myRotate(seg, acosf(d), axis);
        else if (d < 0.0f)              seg = myRotate(seg, 3.14159265f, glm::vec3(1, 0, 0));
    }
    seg = glm::scale(seg, glm::vec3(thickness, length, thickness));
    drawHexModel(seg, color);

    // At leaf nodes, draw a cluster of leaf hexes
    if (depth >= maxDepth - 1) {
        glm::vec3 leafGreen(0.2f, 0.55f, 0.15f);
        // Was 3.0 / 1.5. With the branches finally visible (see the two fixes
        // above) those read as buds on bare twigs rather than as foliage.
        drawHex(endPos, leafGreen, glm::vec3(thickness * 4.5f, thickness * 3.0f, thickness * 4.5f));
        return;
    }

    // Branch into 2-3 sub-branches using myRotate
    float branchAngle = 0.45f + 0.1f * (float)(depth % 3); // radians
    float childLen = length * 0.65f;
    float childThick = thickness * 0.6f;

    // Main continuation (slightly deviated)
    glm::vec3 up(0, 1, 0);
    // The degenerate case has to be caught BEFORE normalising, not after. This
    // used to normalise first and then test `right` — but normalising a zero
    // vector yields NaN, and `NaN < 0.01f` is false, so the fallback never fired.
    // drawFractalTree starts every trunk at dir = (0,1,0), which is exactly the
    // case cross(dir, up) degenerates on, so *every* fractal tree in the world
    // propagated NaN into all of its child directions and drew a bare trunk with
    // no branches at all. Surfaced by the Tree build recipe (plan_2 Step 4).
    glm::vec3 cr = glm::cross(dir, up);
    glm::vec3 right = (glm::length(cr) < 0.01f) ? glm::vec3(1, 0, 0) : glm::normalize(cr);
    glm::vec3 nup = glm::normalize(glm::cross(right, dir));

    // Use myRotate to rotate direction vectors for branches
    glm::mat4 rot1 = myRotate(glm::mat4(1.0f), branchAngle, right);
    glm::vec3 d1 = glm::vec3(rot1 * glm::vec4(dir, 0.0f));
    drawFractalBranch(endPos, glm::normalize(d1), childLen, childThick, depth + 1, maxDepth);

    glm::mat4 rot2 = myRotate(glm::mat4(1.0f), -branchAngle, right);
    glm::vec3 d2 = glm::vec3(rot2 * glm::vec4(dir, 0.0f));
    drawFractalBranch(endPos, glm::normalize(d2), childLen, childThick, depth + 1, maxDepth);

    // Third branch (angled in perpendicular direction)
    if (depth < 2) {
        glm::mat4 rot3 = myRotate(glm::mat4(1.0f), branchAngle * 0.8f, nup);
        glm::vec3 d3 = glm::vec3(rot3 * glm::vec4(dir, 0.0f));
        drawFractalBranch(endPos, glm::normalize(d3), childLen * 0.8f, childThick, depth + 1, maxDepth);
    }
}

// Defaults reproduce the three scenery trees exactly. The Tree build recipe
// passes a taller trunk and one more level of recursion — plan_2 asks for depth
// 4, and reusing this function with arguments is what it means by "re-use
// drawFractalTree; do not port FractalTree.h".
void drawFractalTree(glm::vec3 base, float trunkLen = 1.5f, int maxDepth = 3) {
    drawFractalBranch(base, glm::vec3(0, 1, 0), trunkLen, trunkLen * 0.1f, 0, maxDepth);
}

// =====================================================
// Craft-and-place structures — drawing (plan_2 Step 4)
// =====================================================
// Ported from gfx_b3 main.cpp:2043-2145 (drawMinimalHouse / drawCraftedLight /
// drawCraftedFireplace). gfx_b3 draws each of these at a fixed world position on
// a flat plate; here the player chooses the spot, so every one of them carries a
// yaw and a snapped ground height.
//
// Local space convention, shared by the house and the fireplace: the structure's
// *front* faces -Z, and yaw rotates that front back toward whoever built it. See
// craftStructure() for where the yaw comes from.

const float HOUSE_CS = 0.9f;                 // wall cube edge
const int   HOUSE_W = 6, HOUSE_D = 6, HOUSE_H = 3;
const int   HOUSE_DOOR_GX = 2;               // which front column is the doorway
const float HOUSE_DOOR_OPEN_DIST = 3.0f;     // player range that swings the door

void drawCraftedHouse(const PlacedStructure& h) {
    const float cs = HOUSE_CS;
    // Centre the footprint on h.pos so the stored position means "the middle of
    // the house", which is what the placement test measures flatness around.
    const float ox = -(HOUSE_W - 1) * cs * 0.5f;
    const float oz = -(HOUSE_D - 1) * cs * 0.5f;

    glm::mat4 base = glm::translate(glm::mat4(1.0f), h.pos);
    base = myRotate(base, h.yaw, glm::vec3(0, 1, 0));

    auto cell = [&](int gx, int gy, int gz) {
        return glm::vec3(ox + gx * cs, (gy + 0.5f) * cs, oz + gz * cs);
    };
    auto wallCube = [&](glm::vec3 local) {
        glm::mat4 m = glm::scale(glm::translate(base, local), glm::vec3(cs));
        drawBoxModel(m, glm::vec3(0.85f, 0.80f, 0.76f));
    };

    // Brick walls. textureMode 2 is lit colour * texture, and the box mesh's
    // per-face UVs run 0..1, so one brick image maps to one cube face.
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texBrick);
    setInt(shaderProgram, "texture1", 0);
    setInt(shaderProgram, "textureMode", 2);

    for (int gy = 0; gy < HOUSE_H; gy++) {
        for (int gx = 0; gx < HOUSE_W; gx++) {
            // Doorway: two cubes tall, so the 1.62 m eye height clears it.
            if (!(gx == HOUSE_DOOR_GX && gy < 2)) wallCube(cell(gx, gy, 0));
            wallCube(cell(gx, gy, HOUSE_D - 1));
        }
        for (int gz = 1; gz < HOUSE_D - 1; gz++) {
            wallCube(cell(0, gy, gz));
            wallCube(cell(HOUSE_W - 1, gy, gz));
        }
    }

    // Roof slab, one cube of overhang on every side.
    glBindTexture(GL_TEXTURE_2D, texRoof);
    {
        const float rT = cs * 0.30f;
        float ry = HOUSE_H * cs + rT * 0.5f;
        for (int gx = -1; gx <= HOUSE_W; gx++) {
            for (int gz = -1; gz <= HOUSE_D; gz++) {
                glm::mat4 m = glm::translate(base, glm::vec3(ox + gx * cs, ry, oz + gz * cs));
                m = glm::scale(m, glm::vec3(cs, rT, cs));
                drawBoxModel(m, glm::vec3(0.72f, 0.36f, 0.28f));
            }
        }
    }
    setInt(shaderProgram, "textureMode", 0);

    // Door — thin panel hinged on the left edge of the gap.
    //
    // gfx_b3 drives this from a key (O toggles g_doorOpen for its single
    // hard-coded house). That does not generalise to N houses the player has
    // scattered around, and every key in hexacraft is already spoken for, so the
    // angle is derived from player distance instead. Stateless: nothing to store
    // per house, and it cannot desync.
    {
        const float DT = 0.06f;
        glm::vec3 hinge(ox + (HOUSE_DOOR_GX - 0.5f) * cs, cs, oz - cs * 0.5f + DT);
        glm::vec3 doorWorld = glm::vec3(base * glm::vec4(hinge, 1.0f));
        float d = glm::length(glm::vec2(doorWorld.x - playerWorldPos.x,
                                        doorWorld.z - playerWorldPos.z));
        float openT = (HOUSE_DOOR_OPEN_DIST - d) / 1.2f;
        openT = (openT < 0.0f) ? 0.0f : (openT > 1.0f ? 1.0f : openT);

        glm::mat4 dm = glm::translate(base, hinge);
        dm = myRotate(dm, openT * 1.5707963f, glm::vec3(0, 1, 0));
        dm = glm::translate(dm, glm::vec3(cs * 0.5f, 0, 0));   // pivot at the hinge edge
        dm = glm::scale(dm, glm::vec3(cs, cs * 2.0f, DT));
        drawBoxModel(dm, glm::vec3(0.45f, 0.30f, 0.16f));
    }
}

void drawCraftedFireplace(const PlacedStructure& f, float time) {
    const float bs = 0.34f;
    glm::mat4 base = glm::translate(glm::mat4(1.0f), f.pos);
    base = myRotate(base, f.yaw, glm::vec3(0, 1, 0));

    auto stone = [&](glm::vec3 local, glm::vec3 scale) {
        drawBoxModel(glm::scale(glm::translate(base, local), scale),
                     glm::vec3(0.42f, 0.42f, 0.44f));
    };

    // Back wall, 3 wide x 2 tall — the closed side of the U.
    for (int gx = -1; gx <= 1; gx++)
        for (int gy = 0; gy < 2; gy++)
            stone(glm::vec3(gx * bs, gy * bs + bs * 0.5f, bs * 0.9f),
                  glm::vec3(bs, bs, bs * 0.25f));

    // Side pillars — the two arms, leaving the -Z face open toward the player.
    for (int gy = 0; gy < 2; gy++) {
        stone(glm::vec3(-1.5f * bs, gy * bs + bs * 0.5f, 0.0f), glm::vec3(bs, bs, bs * 1.8f));
        stone(glm::vec3( 1.5f * bs, gy * bs + bs * 0.5f, 0.0f), glm::vec3(bs, bs, bs * 1.8f));
    }

    // Two crossed logs in the mouth of the hearth, so the fire has something to
    // be burning. gfx_b3 has none — its flame floats on the floor slab.
    for (int i = 0; i < 2; i++) {
        glm::mat4 lm = glm::translate(base, glm::vec3(0, bs * 0.18f, 0));
        lm = myRotate(lm, (i ? 0.6f : -0.6f), glm::vec3(0, 1, 0));
        lm = glm::scale(lm, glm::vec3(bs * 1.6f, bs * 0.22f, bs * 0.22f));
        drawBoxModel(lm, glm::vec3(0.30f, 0.19f, 0.10f));
    }

    // Flame. The light for this is registered in gatherTorchLights(), not here —
    // that runs before the draw pass, so the lighting is not a frame behind.
    float fl = 0.55f + 0.45f * sinf(time * 8.2f + f.pos.z * 3.3f);
    glm::mat4 fm = glm::translate(base, glm::vec3(0, bs * 0.62f, 0.0f));
    fm = glm::scale(fm, glm::vec3(bs * 0.85f, bs * (1.0f + 0.45f * fl), bs * 0.6f));
    setBool(shaderProgram, "isEmissive", true);
    setVec3(shaderProgram, "emissiveColor", COL_FIRE_HEARTH);
    drawBoxModel(fm, COL_FIRE_HEARTH);
    setBool(shaderProgram, "isEmissive", false);
}

// =====================================================
// Craft-and-place structures — placement (plan_2 Step 4)
// =====================================================
// Is the ground under a footprint level enough to build on? gfx_b3 needs no such
// test: its world is a flat plate, so `player.position + forward * 3` is always
// valid. hexacraft has mountains, and without this a house lands half-buried in a
// slope with the far corner hanging over a drop.
bool buildSpotOk(glm::vec3 c, float radius, float& outGroundY) {
    float lo = 1e9f, hi = -1e9f;
    auto sample = [&](float px, float pz) {
        float g = getGroundYWorld(px, pz);
        if (g < lo) lo = g;
        if (g > hi) hi = g;
    };
    sample(c.x, c.z);
    const int N = 8;
    for (int i = 0; i < N; i++) {
        float a = (float)i / N * 6.2831853f;
        sample(c.x + cosf(a) * radius, c.z + sinf(a) * radius);
    }
    // Rest on the LOW sample. Resting on the high one leaves the opposite side of
    // the footprint floating; sinking the uphill corner into the slope instead
    // reads as a building cut into the hill, which is what a builder would do.
    outGroundY = lo;
    return (hi - lo) <= 2.0f;
}

// Spend the recipe and drop the structure ~3 m ahead of the player.
// Returns false (and spends nothing) if the materials or the ground say no.
bool craftStructure(int idx) {
    if (idx < 0 || idx >= NUM_BUILD_RECIPES) return false;
    const BuildRecipe& br = buildRecipes[idx];

    // Both ingredients are checked before either is removed — removeFromInventory
    // has no rollback, so a half-spend would silently eat the player's dirt.
    if (countInInventory(br.t1) < br.c1 || countInInventory(br.t2) < br.c2) {
        printf("[Build] %s — need %dx %s + %dx %s\n", br.name,
               br.c1, getBlockName(br.t1), br.c2, getBlockName(br.t2));
        return false;
    }

    glm::vec3 fwd(camFront.x, 0.0f, camFront.z);
    // Looking straight up or down leaves no horizontal facing to build along.
    if (glm::length(fwd) < 1e-4f) fwd = glm::vec3(0.0f, 0.0f, -1.0f);
    fwd = glm::normalize(fwd);

    // The structure's local -Z must end up pointing back at the player. myRotate
    // about +Y sends (0,0,-1) to (-sin a, 0, -cos a), and we want that to equal
    // -fwd, so a = atan2(fwd.x, fwd.z).
    float yaw = atan2f(fwd.x, fwd.z);

    // Footprint radius per recipe, so the *near face* lands about 3 m out (gfx_b3's
    // distance) rather than the centre — otherwise a 5.4 m house starts inside the
    // player.
    const float FOOTPRINT[NUM_BUILD_RECIPES] = {
        HOUSE_D * HOUSE_CS * 0.5f,   // house
        1.2f,                         // tree canopy
        0.3f,                         // torch
        0.7f,                         // fireplace
    };
    float r = FOOTPRINT[idx];
    glm::vec3 c = playerWorldPos + fwd * (3.0f + r);

    float groundY;
    if (!buildSpotOk(c, r, groundY)) {
        printf("[Build] %s — ground too uneven here\n", br.name);
        return false;
    }
    c.y = groundY;

    removeFromInventory(br.t1, br.c1);
    removeFromInventory(br.t2, br.c2);

    switch (idx) {
        case 0: craftedHouses.push_back({ c, yaw }); break;
        case 1: craftedTrees.push_back(c);           break;
        case 2: craftedLights.push_back(c);          break;
        case 3: craftedFireplaces.push_back({ c, yaw }); break;
    }
    printf("[Build] %s placed at %.1f, %.1f, %.1f (yaw %.0f deg)\n",
           br.name, c.x, c.y, c.z, yaw * 57.2957795f);
    return true;
}

// =====================================================
// Render all scene objects
// =====================================================
// Helper: check if a world position is within render range of camera
bool nearCamera(glm::vec3 objPos, float range = 45.0f) {
    glm::vec3 d = objPos - camPos;
    return (d.x*d.x + d.z*d.z) < range * range;
}

// Everything the player has built this session (plan_2 Step 4). Lives here
// rather than beside the draw helpers above because it needs nearCamera().
void renderCraftedStructures(float time) {
    for (const PlacedStructure& h : craftedHouses)
        if (nearCamera(h.pos, 60.0f)) drawCraftedHouse(h);

    // drawFractalTree is recursive and uncached, so it is the expensive one —
    // culled at the same 35 m as the three scenery trees below.
    for (const glm::vec3& t : craftedTrees)
        if (nearCamera(t, 35.0f)) drawFractalTree(t, 2.6f, 4);

    // Geometry only: gatherTorchLights() already pushed the point light, so an
    // off-screen torch keeps lighting what is on screen. drawTorch() would
    // double-register it.
    for (const glm::vec3& p : craftedLights)
        if (nearCamera(p, 45.0f)) drawTorchMesh(p);

    for (const PlacedStructure& f : craftedFireplaces)
        if (nearCamera(f.pos, 45.0f)) drawCraftedFireplace(f, time);
}

void renderObjects(float time) {
    // Player — draw at actual player position (only in third-person)
    if (cameraMode == 0) {
        drawPlayer(playerWorldPos, time, playerYaw, playerWalking, playerWalkTime);
    }

    // Fan — inside castle keep (great hall area)
    int fCol = -15, fRow = 5;
    glm::vec3 fanPos = hexGridPos(fCol, fRow, 0.0f);
    fanPos.y = getGroundY(fCol, fRow);
    if (nearCamera(fanPos)) drawFan(fanPos);

    // Door — castle keep entrance doorway
    int dCol = -11, dRow = 13;
    glm::vec3 doorPos = hexGridPos(dCol, dRow, 0.0f);
    doorPos.y = (UNDERGROUND_DEPTH + 1) * HEX_HEIGHT;
    drawDoor(doorPos);

    // Windows — on castle walls (P key toggles)
    {
        int wCol1 = -24, wRow1 = 6;
        glm::vec3 wPos1 = hexGridPos(wCol1, wRow1, 0.0f);
        wPos1.y = getGroundY(wCol1, wRow1) + 1.5f;
        drawWindow(wPos1, windowAngle, 0.0f);

        int wCol2 = 3, wRow2 = 6;
        glm::vec3 wPos2 = hexGridPos(wCol2, wRow2, 0.0f);
        wPos2.y = getGroundY(wCol2, wRow2) + 1.5f;
        drawWindow(wPos2, windowAngle, 180.0f);

        int wCol3 = -10, wRow3 = -1;
        glm::vec3 wPos3 = hexGridPos(wCol3, wRow3, 0.0f);
        wPos3.y = getGroundY(wCol3, wRow3) + 1.5f;
        drawWindow(wPos3, windowAngle, 90.0f);
    }

    // Clock — in castle courtyard on a pillar
    int cCol = -5, cRow = 20;
    glm::vec3 clockBase = hexGridPos(cCol, cRow, 0.0f);
    clockBase.y = getGroundY(cCol, cRow);
    drawHex(clockBase + glm::vec3(0, 1.0f, 0), COL_STONE, glm::vec3(0.3f, 2.0f, 0.3f));
    glm::vec3 clockPos = clockBase + glm::vec3(0, 2.8f, 0);
    drawClock(clockPos, time);

    // MineCar — draws at tracked position (carPos.y follows terrain smoothly)
    drawCar(glm::vec3(carPos.x, carPos.y, carPos.z), carYaw, wheelSpin, carSteer);

    // --- Curvy Objects Exhibition Area (all 6 grouped together) ---
    // All placed near col=-8, row=5 area so they're visible from one spot.

    // 1. Sphere — crystal ball with BLENDED grass texture
    glm::vec3 sphBase = hexGridPos(-52, -36, 0.0f);
    sphBase.y = getGroundY(-52, -36);
    if (nearCamera(sphBase)) {
        drawHex(sphBase + glm::vec3(0, 0.5f, 0), COL_STONE, glm::vec3(0.4f, 1.0f, 0.4f));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texGrass);
        setInt(shaderProgram, "texture1", 0);
        // Triplanar rather than the sphere's own UVs. A UV sphere's texture
        // coordinates converge at the poles, so a square texture arrives pinched
        // to a point at the top and bottom and stretched around the equator.
        // Projecting from three axes and blending by the normal has no poles.
        setInt(shaderProgram, "textureMode", 3);
        drawSphere(sphBase + glm::vec3(0, 1.8f, 0), 0.6f, glm::vec3(0.3f, 0.8f, 1.0f));
        setInt(shaderProgram, "textureMode", 0);
    }

    // 2. Cone — tent with SIMPLE wood texture
    glm::vec3 coneBase = hexGridPos(-49, -36, 0.0f);
    coneBase.y = getGroundY(-49, -36);
    if (nearCamera(coneBase)) {
        glBindTexture(GL_TEXTURE_2D, texWood);
        // Same problem as the sphere: a cone's UVs all meet at the apex.
        setInt(shaderProgram, "textureMode", 3);
        drawCone(coneBase + glm::vec3(0, 0.1f, 0), 1.5f, glm::vec3(0.85f, 0.55f, 0.25f));
        setInt(shaderProgram, "textureMode", 0);
    }

    // Brick wall panel (textured, near the exhibition)
    {
        glm::vec3 bwBase = hexGridPos(-55, -36, 0.0f);
        bwBase.y = getGroundY(-55, -36);
        if (nearCamera(bwBase)) {
            glBindTexture(GL_TEXTURE_2D, texBrick);
            setInt(shaderProgram, "textureMode", 1);
            drawHex(bwBase + glm::vec3(0, 1.0f, 0), glm::vec3(0.7f), glm::vec3(2.0f, 2.0f, 0.15f));
            setInt(shaderProgram, "textureMode", 0);
        }
    }

    // 3. Wine glass — surface of revolution on table
    {
        glm::vec3 wgBase = hexGridPos(-52, -33, 0.0f);
        wgBase.y = getGroundY(-52, -33);
        if (nearCamera(wgBase)) {
            drawHex(wgBase + glm::vec3(0, 0.5f, 0), glm::vec3(0.4f, 0.25f, 0.15f), glm::vec3(0.6f, 1.0f, 0.6f));
            drawHex(wgBase + glm::vec3(0, 1.1f, 0), glm::vec3(0.5f, 0.3f, 0.18f), glm::vec3(1.0f, 0.1f, 1.0f));
            setFloat(shaderProgram, "alpha", 0.6f);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            drawWineGlass(wgBase + glm::vec3(0, 1.2f, 0), 0.5f, glm::vec3(0.85f, 0.92f, 0.95f));
            drawWineGlass(wgBase + glm::vec3(0.6f, 1.2f, 0.3f), 0.4f, glm::vec3(0.9f, 0.85f, 0.95f));
            glDisable(GL_BLEND);
            setFloat(shaderProgram, "alpha", 1.0f);
        }
    }

    // 4. Bezier curve — decorative arch
    glm::vec3 bezBase = hexGridPos(-49, -33, 0.0f);
    bezBase.y = getGroundY(-49, -33);
    if (nearCamera(bezBase)) {
        drawBezier(bezBase + glm::vec3(0, 0.5f, 0), 2.0f, glm::vec3(0.7f, 0.5f, 0.9f));
    }

    // 5. Spline — winding fence
    glm::vec3 splBase = hexGridPos(-46, -36, 0.0f);
    splBase.y = getGroundY(-46, -36);
    if (nearCamera(splBase)) {
        drawSpline(splBase + glm::vec3(0, 0.3f, 0), 2.5f, glm::vec3(0.6f, 0.4f, 0.2f));
    }

    // 6. Ruled surface — ramp
    glm::vec3 rmpBase = hexGridPos(-46, -33, 0.0f);
    rmpBase.y = getGroundY(-46, -33);
    if (nearCamera(rmpBase)) {
        drawRuledSurface(rmpBase + glm::vec3(0, 0.3f, 0), 2.0f, glm::vec3(  1, 0.7f, 0.5f));
    }

    // --- Decorative: Grass Tufts (textured with texGrass, blended) ---
    {
        static const int gPos[][2] = {
            {5,3},{7,4},{2,6},{10,2},{6,8},{-3,5},{4,10},{8,1},{1,8},{-5,2},
            {12,6},{-1,4},{3,9},{9,-1},{0,7},{-4,9},{11,4},{7,-2},{-6,6},{5,12}
        };
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texGrass);
        setInt(shaderProgram, "texture1", 0);
        setInt(shaderProgram, "textureMode", 2);
        for (auto& gp : gPos) {
            glm::vec3 base = hexGridPos(gp[0], gp[1], 0.0f);
            base.y = getGroundY(gp[0], gp[1]) + HEX_HEIGHT * 0.52f;
            if (!nearCamera(base)) continue;
            unsigned int s = gridSeed(gp[0], gp[1]);
            float sz = 0.45f + (s & 0xF) * 0.04f;
            glm::vec3 gc = COL_GRASS * (0.75f + (s & 7) * 0.035f);
            // Main tuft
            drawHex(base, gc, glm::vec3(sz, 0.05f, sz));
            // Second smaller tuft offset
            drawHex(base + glm::vec3(0.22f, 0.02f, 0.15f), gc * 0.9f, glm::vec3(sz * 0.65f, 0.05f, sz * 0.65f));
        }
        setInt(shaderProgram, "textureMode", 0);
    }

    // --- Decorative: Stone Boulders (textured with texBrick as stone) ---
    {
        static const int bPos[][2] = {
            {15,5},{-8,12},{20,-3},{-18,8},{12,-10},
            {25,15},{-5,-8},{30,5},{18,-8},{-22,3}
        };
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texBrick);
        setInt(shaderProgram, "texture1", 0);
        setInt(shaderProgram, "textureMode", 1);
        for (auto& bp : bPos) {
            glm::vec3 base = hexGridPos(bp[0], bp[1], 0.0f);
            base.y = getGroundY(bp[0], bp[1]);
            if (!nearCamera(base)) continue;
            unsigned int s = gridSeed(bp[0], bp[1]);
            float w = 0.5f + (s & 0xF) * 0.05f;
            float h = 0.3f + ((s >> 4) & 0xF) * 0.04f;
            glm::vec3 sc = COL_STONE * (0.75f + (s & 7) * 0.04f);
            // Main boulder
            drawHex(base + glm::vec3(0, h * HEX_HEIGHT * 0.5f, 0), sc, glm::vec3(w, h, w));
            // Smaller rock beside it
            float w2 = w * 0.45f;
            drawHex(base + glm::vec3(0.38f, w2 * HEX_HEIGHT * 0.3f, 0.22f), sc * 0.88f, glm::vec3(w2, w2 * 0.6f, w2));
            // Pebble
            float w3 = w * 0.2f;
            drawHex(base + glm::vec3(-0.3f, w3 * HEX_HEIGHT * 0.3f, 0.3f), sc * 0.82f, glm::vec3(w3, w3 * 0.5f, w3));
        }
        setInt(shaderProgram, "textureMode", 0);
    }

    // --- Decorative: Flowers (color only, no texture) ---
    {
        struct FlowerSpec { int col, row; glm::vec3 color; };
        static const FlowerSpec flowers[] = {
            {3,4,  glm::vec3(0.9f,0.2f,0.2f)},  // red
            {6,3,  glm::vec3(0.9f,0.8f,0.1f)},  // yellow
            {-2,7, glm::vec3(0.7f,0.2f,0.8f)},  // purple
            {9,6,  glm::vec3(1.0f,0.4f,0.0f)},  // orange
            {4,-2, glm::vec3(1.0f,0.75f,0.8f)}, // pink
            {-7,3, glm::vec3(0.4f,0.6f,1.0f)},  // blue
            {11,9, glm::vec3(1.0f,0.3f,0.4f)},  // rose
            {-3,11,glm::vec3(0.9f,0.9f,0.3f)},  // sunflower yellow
            {13,3, glm::vec3(0.5f,0.2f,0.9f)},  // violet
        };
        setInt(shaderProgram, "textureMode", 0);
        for (auto& f : flowers) {
            glm::vec3 base = hexGridPos(f.col, f.row, 0.0f);
            base.y = getGroundY(f.col, f.row);
            if (!nearCamera(base)) continue;
            // Stem
            drawHex(base + glm::vec3(0, HEX_HEIGHT * 0.3f, 0),
                    glm::vec3(0.2f, 0.65f, 0.15f), glm::vec3(0.07f, 0.6f, 0.07f));
            // Petal cap
            drawHex(base + glm::vec3(0, HEX_HEIGHT * 0.72f, 0),
                    f.color, glm::vec3(0.35f, 0.04f, 0.35f));
            // Small centre dot (slightly lighter)
            drawHex(base + glm::vec3(0, HEX_HEIGHT * 0.74f, 0),
                    f.color * 1.3f + glm::vec3(0.1f), glm::vec3(0.12f, 0.025f, 0.12f));
        }
    }

    // --- Decorative: Mushrooms ---
    {
        static const int mPos[][2] = {
            {-10,6},{14,12},{-20,-5},{8,18},{-12,18},{22,8}
        };
        setInt(shaderProgram, "textureMode", 0);
        for (auto& mp : mPos) {
            glm::vec3 base = hexGridPos(mp[0], mp[1], 0.0f);
            base.y = getGroundY(mp[0], mp[1]);
            if (!nearCamera(base)) continue;
            unsigned int s = gridSeed(mp[0], mp[1]);
            bool redMush = (s & 1);
            glm::vec3 capCol = redMush
                ? glm::vec3(0.75f, 0.12f, 0.08f)
                : glm::vec3(0.55f, 0.38f, 0.12f);
            float scale = 0.7f + (s & 7) * 0.07f;
            // Stalk
            drawHex(base + glm::vec3(0, HEX_HEIGHT * 0.22f * scale, 0),
                    glm::vec3(0.85f, 0.82f, 0.78f), glm::vec3(0.12f * scale, 0.45f * scale, 0.12f * scale));
            // Cap
            drawHex(base + glm::vec3(0, HEX_HEIGHT * 0.58f * scale, 0),
                    capCol, glm::vec3(0.52f * scale, 0.07f * scale, 0.52f * scale));
            // White spots on red mushroom
            if (redMush) {
                drawHex(base + glm::vec3(0.09f, HEX_HEIGHT * 0.61f * scale, 0.05f),
                        glm::vec3(0.95f, 0.95f, 0.9f), glm::vec3(0.09f, 0.02f, 0.09f));
                drawHex(base + glm::vec3(-0.06f, HEX_HEIGHT * 0.60f * scale, 0.1f),
                        glm::vec3(0.95f, 0.95f, 0.9f), glm::vec3(0.06f, 0.015f, 0.06f));
            }
        }
    }

    // Fractal Trees — distance-culled
    {
        glm::vec3 ftBase1 = hexGridPos(20, 15, 0.0f); ftBase1.y = getGroundY(20, 15);
        if (nearCamera(ftBase1, 35.0f)) drawFractalTree(ftBase1);

        glm::vec3 ftBase2 = hexGridPos(-15, 25, 0.0f); ftBase2.y = getGroundY(-15, 25);
        if (nearCamera(ftBase2, 35.0f)) drawFractalTree(ftBase2);

        glm::vec3 ftBase3 = hexGridPos(30, -20, 0.0f); ftBase3.y = getGroundY(30, -20);
        if (nearCamera(ftBase3, 35.0f)) drawFractalTree(ftBase3);
    }

    // Player-built houses, trees, torches and fireplaces (plan_2 Step 4)
    renderCraftedStructures(time);

    // --- Flying Birds ---
    for (auto& b : birds) {
        if (nearCamera(b.pos)) drawBird(b, time);
    }

    // --- Mobs (render alive + dying mobs) ---
    for (auto& m : mobs) {
        if ((m.alive || m.deathTimer >= 0.0f) && nearCamera(m.pos)) drawMob(m, time);
    }

    // --- Arrow projectiles ---
    drawArrows();

    // --- Item drops: update physics, render, pickup ---
    for (int i = (int)itemDrops.size() - 1; i >= 0; i--) {
        ItemDrop& d = itemDrops[i];
        d.lifetime += deltaTime;

        // Despawn
        if (d.lifetime > ITEM_DESPAWN_TIME) {
            itemDrops.erase(itemDrops.begin() + i);
            continue;
        }

        // Physics: gravity + bounce
        d.vel.y -= 10.0f * deltaTime;
        d.pos += d.vel * deltaTime;

        // Ground collision for drop
        int dc, dr;
        worldToColRow(d.pos.x, d.pos.z, dc, dr);
        float groundY = getGroundYWorld(d.pos.x, d.pos.z);
        if (d.pos.y < groundY + 0.3f) {
            d.pos.y = groundY + 0.3f;
            d.vel = glm::vec3(0.0f); // stop bouncing
        }

        // Bobbing + spinning animation
        float bob = sinf(time * 2.0f + d.bobPhase) * 0.15f;
        float spin = time * 2.0f + d.bobPhase;
        glm::vec3 drawPos = d.pos + glm::vec3(0, bob, 0);

        // Draw as small rotating hex
        glm::vec3 color = getBlockColor(d.type);
        drawHexRotated(drawPos, color, spin, glm::vec3(0, 1, 0), glm::vec3(0.3f, 0.3f, 0.3f));

        // Pickup: check distance to player
        float dist = glm::length(d.pos - playerWorldPos);
        if (dist < ITEM_PICKUP_RADIUS) {
            bool pickedUp = false;
            // 1. Try to stack in existing slot
            for (int j = 0; j < 36; j++) {
                if (playerInventory[j].type == d.type && playerInventory[j].count < 64) {
                    playerInventory[j].count++;
                    pickedUp = true;
                    break;
                }
            }
            // 2. Try empty slot
            if (!pickedUp) {
                for (int j = 0; j < 36; j++) {
                    if (playerInventory[j].type == BLOCK_AIR) {
                        playerInventory[j].type = d.type;
                        playerInventory[j].count = 1;
                        pickedUp = true;
                        break;
                    }
                }
            }
            if (pickedUp) {
                printf("[Pickup] Got %d\n", d.type);
                itemDrops.erase(itemDrops.begin() + i);
            }
        }
    }

    // --- Break particles: update physics + render ---
    for (int i = (int)breakParticles.size() - 1; i >= 0; i--) {
        BreakParticle& bp = breakParticles[i];
        bp.lifetime += deltaTime;
        if (bp.lifetime >= bp.maxLifetime) {
            breakParticles.erase(breakParticles.begin() + i);
            continue;
        }
        // Physics: gravity pulls them down, no bounce
        bp.vel.y -= 14.0f * deltaTime;
        bp.pos   += bp.vel * deltaTime;
        bp.spin  += bp.spinSpeed * deltaTime;

        // Fade out in last 20% of lifetime
        float t = bp.lifetime / bp.maxLifetime;
        float alpha = (t > 0.8f) ? (1.0f - (t - 0.8f) / 0.2f) : 1.0f;

        // Draw as tiny rotating hex chip
        float sz = 0.07f * (1.0f - t * 0.4f); // shrinks slightly
        glm::vec3 col = bp.color * alpha;
        drawHexRotated(bp.pos, col, bp.spin, glm::vec3(0.577f, 0.577f, 0.577f),
                       glm::vec3(sz, sz * 0.5f, sz));
    }

    // --- Carried block (K) ---
    // Last object drawn: it is the nearest thing to the eye, so leaving it until
    // the end means every earlier depth test is against terrain rather than
    // against a block hanging two units from the camera.
    drawCarriedBlock(time);
}

// =====================================================
// Update camera direction from pitch/yaw
// =====================================================
void updateCameraVectors() {
    glm::vec3 front;
    front.x = cosf(glm::radians(camYaw)) * cosf(glm::radians(camPitch));
    front.y = sinf(glm::radians(camPitch));
    front.z = sinf(glm::radians(camYaw)) * cosf(glm::radians(camPitch));
    camFront = glm::normalize(front);

    // Apply roll using myRotate
    glm::vec3 worldUp(0, 1, 0);
    glm::vec3 right = glm::normalize(glm::cross(camFront, worldUp));

    // Roll rotates the up vector around the front axis
    glm::mat4 rollMat = myRotate(glm::mat4(1.0f), glm::radians(camRoll), camFront);
    glm::vec4 upRolled = rollMat * glm::vec4(glm::cross(right, camFront), 1.0f);
    camUp = glm::normalize(glm::vec3(upRolled));
}

// =====================================================
// Key handling
// =====================================================
void processInput(GLFWwindow* window) {
    // Block all movement/gameplay input while inventory is open
    if (inventoryOpen) return;

    float speed = camSpeed * deltaTime;
    playerWalking = false;

    if (cameraMode == 2) {
        // --- Free-fly mode: WASD moves camera directly ---
        glm::vec3 right = glm::normalize(glm::cross(camFront, camUp));
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camPos += camFront * speed;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camPos -= camFront * speed;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camPos -= right * speed;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camPos += right * speed;
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) camPos += camUp * speed;
        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) camPos -= camUp * speed;
    } else {
        if (!controlCar) {
            // --- Player mode (first/third person): WASD moves the player ---
            // Q key: hold to fly up, release to fall (gravity)
            bool flying = (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) && !playerDead;

        // Movement direction
        float moveYaw = glm::radians(camYaw);

        // Sprinting (Shift held + moving forward + has stamina)
        playerSprinting = (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
                          && playerStamina > 0.0f && !playerDead;
        float speedMult = playerSprinting ? 1.5f : 1.0f;
        float pSpeed = 4.5f * speedMult * deltaTime;

        if (flying) {
            // --- Flying mode: WASD moves in full 3D camera direction ---
            playerOnGround = false;

            glm::vec3 flyForward = camFront; // full 3D direction (where you look)
            glm::vec3 flyRight = glm::normalize(glm::cross(camFront, glm::vec3(0, 1, 0)));

            glm::vec3 moveDir(0.0f);
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) { moveDir += flyForward; playerWalking = true; }
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) { moveDir -= flyForward; playerWalking = true; }
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) { moveDir -= flyRight; playerWalking = true; }
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) { moveDir += flyRight; playerWalking = true; }

            if (glm::length(moveDir) > 0.001f) {
                moveDir = glm::normalize(moveDir);
                playerWorldPos += moveDir * pSpeed * 1.5f; // slightly faster in flight
                playerYaw = atan2f(moveDir.z, moveDir.x);
            }

            // Q held: fly upward steadily
            playerVelY = 5.0f;

        } else {
            // --- Ground mode: WASD moves horizontally only ---
            glm::vec3 forward(cosf(moveYaw), 0.0f, sinf(moveYaw));
            glm::vec3 right(-sinf(moveYaw), 0.0f, cosf(moveYaw));

            glm::vec3 moveDir(0.0f);
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) { moveDir += forward; playerWalking = true; }
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) { moveDir -= forward; playerWalking = true; }
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) { moveDir -= right; playerWalking = true; }
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) { moveDir += right; playerWalking = true; }

            if (glm::length(moveDir) > 0.001f) {
                moveDir = glm::normalize(moveDir);
                float newX = playerWorldPos.x + moveDir.x * pSpeed;
                float newZ = playerWorldPos.z + moveDir.z * pSpeed;
                // Horizontal collision with auto-step (climb 1-block ledges)
                // Try full move first, then axis-separated (wall sliding)
                if (!tryMoveWithStep(playerWorldPos.x, playerWorldPos.y, playerWorldPos.z, newX, newZ)) {
                    // Wall sliding: try X-only, then Z-only
                    if (!tryMoveWithStep(playerWorldPos.x, playerWorldPos.y, playerWorldPos.z, newX, playerWorldPos.z)) {
                        tryMoveWithStep(playerWorldPos.x, playerWorldPos.y, playerWorldPos.z, playerWorldPos.x, newZ);
                    }
                }
                // Face movement direction
                playerYaw = atan2f(moveDir.z, moveDir.x);
            }

            // Jump
            if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && playerOnGround && !playerDead) {
                playerVelY = 6.0f;
                playerOnGround = false;
            }
        }

        // E/R for vertical fly (alternative controls, override gravity)
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
            playerVelY = 8.0f;
            playerOnGround = false;
            flying = true;
        }
        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
            playerVelY = -6.0f;
            flying = true;
        }

        // Ladder climbing: detect if player is overlapping a ladder block
        {
            int lCol, lRow;
            worldToColRow(playerWorldPos.x, playerWorldPos.z, lCol, lRow);
            int lH = (int)floorf(playerWorldPos.y / HEX_HEIGHT);
            int btFeet = getBlock(lCol, lRow, lH);
            int btBody = getBlock(lCol, lRow, lH + 1);
            playerOnLadder = (btFeet == BLOCK_LADDER || btBody == BLOCK_LADDER);
        }
        if (playerOnLadder && !flying) {
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
                playerVelY = 4.0f;
                playerOnGround = false;
            } else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
                playerVelY = -4.0f;
            } else {
                playerVelY = 0.0f; // hold position on ladder
            }
            flying = true; // suppress gravity while on ladder
        }

        // Track fall start
        if (!playerOnGround && !trackingFall && !flying) {
            fallStartY = playerWorldPos.y;
            trackingFall = true;
        }

        // Gravity (disabled while flying)
        if (!flying) {
            playerVelY -= 15.0f * deltaTime;
        }
        playerWorldPos.y += playerVelY * deltaTime;

        // Ground collision — Y-aware: finds ground below player, not roof above
        {
            int col, row;
            worldToColRow(playerWorldPos.x, playerWorldPos.z, col, row);
            float groundY = getGroundYAtHeight(col, row, playerWorldPos.y);
            if (playerWorldPos.y <= groundY) {
                playerWorldPos.y = groundY;

                // Fall damage: damage if fell more than 3 blocks
                if (trackingFall) {
                    float fallDist = fallStartY - groundY;
                    if (fallDist > 3.0f * HEX_HEIGHT) {
                        float dmg = (fallDist / HEX_HEIGHT) - 3.0f; // 1 damage per block above 3
                        playerHealth -= dmg;
                        if (playerHealth < 0.0f) playerHealth = 0.0f;
                        printf("[Player] Fall damage: %.1f (fell %.1f blocks)\n", dmg, fallDist / HEX_HEIGHT);
                        if (playerHealth <= 0.0f && !playerDead) {
                            playerDead = true;
                            printf("[Player] YOU DIED! Press R to respawn.\n");
                        }
                    }
                    trackingFall = false;
                }

                playerVelY = 0.0f;
                playerOnGround = true;
            }
        }

        // Sprinting drains stamina
        if (playerSprinting && playerWalking) {
            playerStamina -= 15.0f * deltaTime;
            if (playerStamina < 0.0f) playerStamina = 0.0f;
        } else {
            // Stamina regenerates when not sprinting
            playerStamina += 10.0f * deltaTime;
            if (playerStamina > playerMaxStamina) playerStamina = playerMaxStamina;
        }

        // Hunger depletes over time
        hungerTimer += deltaTime;
        float hungerRate = playerSprinting ? 0.3f : 0.1f; // faster when sprinting
        if (hungerTimer >= 5.0f) { // every 5 seconds
            playerHunger -= hungerRate;
            if (playerHunger < 0.0f) playerHunger = 0.0f;
            hungerTimer = 0.0f;
        }

        // Health regen when hunger > 14 (70%)
        if (playerHunger > 14.0f && playerHealth < playerMaxHealth) {
            playerHealth += 0.5f * deltaTime;
            if (playerHealth > playerMaxHealth) playerHealth = playerMaxHealth;
        }
        // Starving: lose health when hunger is 0
        if (playerHunger <= 0.0f) {
            playerHealth -= 0.5f * deltaTime;
            if (playerHealth < 0.0f) playerHealth = 0.0f;
            if (playerHealth <= 0.0f && !playerDead) {
                playerDead = true;
                printf("[Player] YOU DIED from starvation! Press R to respawn.\n");
            }
        }

        // Walk animation timer
        if (playerWalking) {
            playerWalkTime += deltaTime * 6.0f;
        }
        } // Close if (!controlCar)

        // Update camera to follow player (Minecraft-style)
        // Eye height: 1.62 blocks from feet (Minecraft standard)
        float eyeHeight = 1.62f;
            if (controlCar && cameraMode != 2) {
                // Drive cameras, ported from gfx_b3 Car.h:305-320. Before this
                // the camera stayed on the player while the car drove off on its
                // own, which is why the car was hard to steer at all.
                //
                // C still cycles: 0 = chase, 1 = driver's seat, 2 = free fly
                // (left alone, so the debug flycam still works while driving).
                glm::vec3 fwd(cosf(carYaw), 0.0f, sinf(carYaw));
                if (cameraMode == 0) {
                    glm::vec3 pivot = carPos + glm::vec3(0, 1.1f, 0);
                    glm::vec3 want  = -fwd * 6.0f + glm::vec3(0, 2.6f, 0);
                    // Through collideCamera, which theirs has no equivalent of —
                    // its world is flat, so a fixed 6-unit boom never hits
                    // anything. Here it would sit inside the hillside the car is
                    // parked against.
                    camPos = pivot + collideCamera(pivot, want);
                } else {
                    camPos = carPos + fwd * 0.35f + glm::vec3(0, 1.15f, 0);
                }
                // Point the view down the car's heading. camYaw is in degrees and
                // measured the same way carYaw is, so this is a direct handover;
                // updateCameraVectors() rebuilds camFront from it.
                camYaw = glm::degrees(carYaw);
                camPitch = (cameraMode == 0) ? -12.0f : 0.0f;
                updateCameraVectors();
            } else if (cameraMode == 0) {
                // Third-person: camera behind and above player's head
                float camYawRad = glm::radians(camYaw);
                float camPitchRad = glm::radians(camPitch);
                glm::vec3 offset;
                offset.x = -cosf(camYawRad) * cosf(camPitchRad) * thirdPersonDist;
                offset.y = -sinf(camPitchRad) * thirdPersonDist + thirdPersonHeight;
                offset.z = -sinf(camYawRad) * cosf(camPitchRad) * thirdPersonDist;
                glm::vec3 pivot = playerWorldPos + glm::vec3(0, eyeHeight, 0);
                camPos = pivot + collideCamera(pivot, offset);
            } else {
                // First-person: camera at player eye level
                camPos = playerWorldPos + glm::vec3(0, eyeHeight, 0);
            }

        // E/R fly is now handled above with proper gravity override
    }

    // Arrow keys = drive MineCar (realistic physics)
    {
        float maxCarSpeed = 12.0f;   // max speed units
        float carAccelRate = 8.0f;   // acceleration per second
        float carBrakeRate = 14.0f;  // braking deceleration per second
        float carFrictionRate = 0.92f; // drag per frame (rolling resistance)

        // Acceleration / braking
        bool gasPressed = (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS);
        bool brakePressed = (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS);

        if (gasPressed) {
            carSpeed += carAccelRate * deltaTime;
            if (carSpeed > maxCarSpeed) carSpeed = maxCarSpeed;
        }
        if (brakePressed) {
            // If moving forward, brake first; once stopped, reverse
            if (carSpeed > 0.1f) {
                carSpeed -= carBrakeRate * deltaTime;
                if (carSpeed < 0.0f) carSpeed = 0.0f;
            } else {
                carSpeed -= carAccelRate * 0.6f * deltaTime; // reverse is slower
                if (carSpeed < -maxCarSpeed * 0.4f) carSpeed = -maxCarSpeed * 0.4f;
            }
        }

        // Friction (rolling resistance when no input)
        if (!gasPressed && !brakePressed) {
            carSpeed *= powf(carFrictionRate, deltaTime * 60.0f);
            if (fabsf(carSpeed) < 0.05f) carSpeed = 0.0f;
        }

        // Steering — bicycle model, ported from gfx_b3 Car.h:139-181.
        //
        // This replaces a constant yaw rate (carYaw += turnRate * dt), which let
        // the car pirouette on the spot at walking pace. Here the steering wheel
        // has a POSITION, and the yaw rate falls out of geometry:
        //
        //     turnRadius = wheelbase / tan(steer)
        //     yawRate    = speed / turnRadius
        //
        // so yaw rate is proportional to speed. At a standstill the car cannot
        // turn at all, and the circle tightens as it slows. That is the whole
        // difference between an arcade blob and something that drives.
        bool steerLeft  = (glfwGetKey(window, GLFW_KEY_LEFT)  == GLFW_PRESS);
        bool steerRight = (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS);
        if (steerLeft) {
            carSteer = fminf(carSteer + CAR_STEER_RATE * deltaTime,  CAR_MAX_STEER);
        } else if (steerRight) {
            carSteer = fmaxf(carSteer - CAR_STEER_RATE * deltaTime, -CAR_MAX_STEER);
        } else {
            // Self-centre at 1.5x the input rate, without overshooting zero.
            float back = CAR_STEER_RATE * 1.5f * deltaTime;
            if (carSteer > 0.0f)      carSteer = fmaxf(0.0f, carSteer - back);
            else if (carSteer < 0.0f) carSteer = fminf(0.0f, carSteer + back);
        }

        // Grip limit. This is BEYOND gfx_b3 and it is not optional: the bare
        // bicycle model puts lateral acceleration at v^2*tan(steer)/L, so at this
        // car's top speed of 12 and full 35-degree lock that is a yaw rate of
        // 7 rad/s — 400 degrees per second. Unusable. (Theirs is equally unusable
        // on paper at 229 deg/s; their world is small enough that top speed and
        // full lock never coincide.)
        //
        // Capping lateral acceleration is the physical statement of what a tire
        // can actually do, and it produces the right feel for free: full lock
        // available at parking speed, barely 9 degrees at top speed. Note this
        // limits the STEERING ANGLE, not the yaw rate — so the wheels are drawn
        // at the angle the car is really turning at, and the geometry stays
        // honest. The old code scaled the yaw rate directly, which is what broke
        // the relationship between the wheels and the path in the first place.
        const float CAR_MAX_LAT_ACCEL = 18.0f;
        float vSq = carSpeed * carSpeed;
        if (vSq > 0.01f) {
            float tanLimit = CAR_MAX_LAT_ACCEL * CAR_WHEELBASE / vSq;
            float steerLimit = atanf(tanLimit);
            if (steerLimit < CAR_MAX_STEER)
                carSteer = fmaxf(-steerLimit, fminf(steerLimit, carSteer));
        }

        if (fabsf(carSpeed) > 0.01f) {
            // The epsilon keeps the radius finite at dead-ahead. Sign is carried
            // by carSteer and by carSpeed independently, which is what makes
            // reversing steer backwards for free — no special case needed, unlike
            // the turnDir hack this replaces.
            float turnRadius = CAR_WHEELBASE / tanf(fabsf(carSteer) + 0.001f);
            float yawRate = carSpeed / turnRadius;
            if (carSteer < 0.0f) yawRate = -yawRate;
            carYaw += yawRate * deltaTime;
        }

        // Move car — cosf(yaw) for X, sinf(yaw) for Z to match model forward (+X local).
        // Gated on carCanBeAt: before this the car integrated position with no
        // collision test at all and drove straight through castle walls, only
        // sampling ground height afterwards to set carPos.y.
        {
            float nx = carPos.x + cosf(carYaw) * carSpeed * deltaTime;
            float nz = carPos.z + sinf(carYaw) * carSpeed * deltaTime;
            if (carCanBeAt(nx, nz, carYaw, carPos.y)) {
                carPos.x = nx;
                carPos.z = nz;
            } else {
                // Stop dead rather than slide along the obstruction. The player
                // slides because being snagged on scenery while walking is
                // maddening; a car that keeps its speed while grinding down a
                // wall just feels broken.
                carSpeed = 0.0f;
            }
        }

        // If the car ends up on top of the player, shove them clear instead of
        // trapping them — canMoveTo treats the parked car as solid, so a player
        // left standing inside its footprint could not walk out. Push along the
        // car's short axis, which is always the shorter way out.
        //
        // carOverlaps, not carBlocks: the gate in carBlocks is off while driving,
        // and driving is precisely when the car runs someone over.
        float pLx, pLz;
        if (!playerDead &&
            carOverlaps(playerWorldPos.x, playerWorldPos.z, playerWorldPos.y, 0.28f, &pLx, &pLz)) {
            float outLz = (pLz >= 0.0f ? 1.0f : -1.0f) * (CAR_HALF_WID + 0.32f);
            float c = cosf(carYaw), s = sinf(carYaw);
            // car-local -> world, keeping the player's position along the car's
            // length so they are nudged sideways rather than flung to the bumper.
            float px = carPos.x + pLx * c - outLz * s;
            float pz = carPos.z + pLx * s + outLz * c;
            if (canMoveTo(px, pz, playerWorldPos.y)) {
                playerWorldPos.x = px;
                playerWorldPos.z = pz;
            }
        }

        // Terrain-following Y (smooth interpolation)
        int carCol, carRow;
        worldToColRow(carPos.x, carPos.z, carCol, carRow);
        float targetY = getGroundY(carCol, carRow);
        // Smooth Y transition so car doesn't teleport over bumps
        carPos.y += (targetY - carPos.y) * fminf(1.0f, 8.0f * deltaTime);

        // Wheel spin based on speed (realistic: proportional to distance traveled)
        wheelSpin += carSpeed * deltaTime * 3.0f;

    }

    // Pitch (X), Yaw (Y), Roll (Z) — always available
    float rotSpeed = 60.0f * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) {
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
            camPitch -= rotSpeed;
        else
            camPitch += rotSpeed;
    }
    if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS) {
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
            camYaw -= rotSpeed;
        else
            camYaw += rotSpeed;
    }
    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) {
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
            camRoll -= rotSpeed;
        else
            camRoll += rotSpeed;
    }

    // Rotate around look-at point (F) — only in free-fly mode
    if (cameraMode == 2 && glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
        float orbitSpeed = 40.0f * deltaTime;
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
            orbitAngle -= orbitSpeed;
        else
            orbitAngle += orbitSpeed;

        float orbitRadius = glm::length(camPos - lookAtTarget);
        camPos.x = lookAtTarget.x + orbitRadius * cosf(glm::radians(orbitAngle));
        camPos.z = lookAtTarget.z + orbitRadius * sinf(glm::radians(orbitAngle));
        // Point camera at target
        camFront = glm::normalize(lookAtTarget - camPos);
        camYaw = glm::degrees(atan2f(camFront.z, camFront.x));
        camPitch = glm::degrees(asinf(camFront.y));
    }

    // Clamp pitch
    if (camPitch > 89.0f) camPitch = 89.0f;
    if (camPitch < -89.0f) camPitch = -89.0f;

    updateCameraVectors();

    // --- Hold-to-break tick ---
    static float breakDebugTimer = 0.0f;
    breakDebugTimer -= deltaTime;
    if (isBreaking && breakDebugTimer <= 0.0f) {
        breakDebugTimer = 0.5f;
        printf("[Break] isBreaking=%d hasTarget=%d inventoryOpen=%d target=(%d,%d,%d) hand=%s\n",
               (int)isBreaking, (int)hasTarget, (int)inventoryOpen,
               targetCol, targetRow, targetHeight,
               getBlockName(playerInventory[hotbarSlot].type));
        if (hasTarget) {
            int _dbt = getBlock(targetCol, targetRow, targetHeight);
            printf("[Break]   block=%d hardness=%.2f speed=%.2f holdTime=%.3f\n",
                   _dbt, getBlockHardness(_dbt),
                   getToolSpeedMultiplier(playerInventory[hotbarSlot].type, _dbt),
                   breakHoldTime);
        }
    }
    if (isBreaking && hasTarget && !inventoryOpen) {
        // If player looked away to a different block, reset progress
        if (targetCol != breakTargetCol || targetRow != breakTargetRow || targetHeight != breakTargetH) {
            breakHoldTime = 0.0f;
            breakTargetCol = targetCol;
            breakTargetRow = targetRow;
            breakTargetH   = targetHeight;
        }

        int bt = getBlock(targetCol, targetRow, targetHeight);
        float hardness = getBlockHardness(bt);

        if (bt == BLOCK_AIR) {
            // Nothing to break — reset progress but keep isBreaking so it resumes when crosshair returns to a solid block
            breakHoldTime = 0.0f;
        } else if (hardness < 0.0f) {
            // Unbreakable (bedrock)
        } else {
            int heldType = playerInventory[hotbarSlot].type;
            float speed = getToolSpeedMultiplier(heldType, bt);
            if (speed <= 0.0f) {
                // Can't break (e.g. obsidian without diamond pick)
            } else {
                float duration = hardness / speed;
                breakHoldTime += deltaTime;

                // Emit break particles — throttled to ~12 per second
                breakParticleTimer -= deltaTime;
                if (breakParticleTimer <= 0.0f && (int)breakParticles.size() < MAX_BREAK_PARTICLES) {
                    breakParticleTimer = 0.083f; // ~12 Hz

                    glm::vec3 blockCenter = hexGridPos(targetCol, targetRow, 0.0f);
                    blockCenter.y = targetHeight * HEX_HEIGHT + HEX_HEIGHT * 0.5f;
                    glm::vec3 blockColor = getBlockColor(bt);

                    // Emit 3 chips per burst
                    for (int p = 0; p < 3; p++) {
                        if ((int)breakParticles.size() >= MAX_BREAK_PARTICLES) break;

                        // Random offset on block surface
                        float rx = ((gridSeed(targetCol + p, targetRow + (int)(breakHoldTime * 100)) % 100) - 50) * 0.01f;
                        float ry = ((gridSeed(targetRow + p, targetCol + (int)(breakHoldTime * 137)) % 100) - 50) * 0.01f;
                        float rz = ((gridSeed(targetCol * 3 + p, targetRow + 7) % 100) - 50) * 0.01f;

                        BreakParticle bp;
                        bp.pos = blockCenter + glm::vec3(rx, ry, rz) * 0.5f;
                        // Velocity: burst outward toward camera + random spread
                        glm::vec3 outDir = glm::normalize(camPos - blockCenter);
                        float spreadX = ((gridSeed(targetCol + p * 7, targetRow) % 100) - 50) * 0.06f;
                        float spreadY = ((gridSeed(targetRow + p * 5, targetCol) % 100)) * 0.04f + 1.5f;
                        float spreadZ = ((gridSeed(targetCol, targetRow + p * 11) % 100) - 50) * 0.06f;
                        bp.vel = outDir * 2.5f + glm::vec3(spreadX, spreadY, spreadZ);
                        // Slight color variation (darker/lighter chips)
                        float tint = 0.7f + ((gridSeed(p, targetCol + targetRow) % 30)) * 0.01f;
                        bp.color = blockColor * tint;
                        bp.lifetime = 0.0f;
                        bp.maxLifetime = 0.35f + ((gridSeed(p * 3, targetCol) % 30)) * 0.01f;
                        bp.spin = 0.0f;
                        bp.spinSpeed = ((gridSeed(targetCol + p, targetRow * 2) % 200) - 100) * 0.12f;
                        breakParticles.push_back(bp);
                    }
                }

                if (breakHoldTime >= duration) {
                    breakBlock();
                    // Reset so next block (if still holding) starts fresh
                    breakHoldTime = 0.0f;
                    breakTargetCol = -9999; // force target mismatch next frame
                    breakTargetRow = -9999;
                    breakTargetH   = -9999;
                }
            }
        }
    } else if (!isBreaking) {
        breakHoldTime = 0.0f;
        breakParticleTimer = 0.0f;
    }
}

void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastMouseX = xpos;
        lastMouseY = ypos;
        firstMouse = false;
        return;
    }
    
    if (inventoryOpen) {
        lastMouseX = xpos;
        lastMouseY = ypos;
        return;
    }

    float dx = (float)(xpos - lastMouseX) * mouseSensitivity;
    float dy = (float)(lastMouseY - ypos) * mouseSensitivity;
    lastMouseX = xpos;
    lastMouseY = ypos;

    // Always look around with mouse movement (like Minecraft)
    camYaw += dx;
    camPitch += dy;
    if (camPitch > 89.0f) camPitch = 89.0f;
    if (camPitch < -89.0f) camPitch = -89.0f;
    updateCameraVectors();
}

// Defined below, next to the key handler that normally calls it. The Build tab
// closes the inventory on a successful build, and it has to go through the same
// path as pressing E — that is what returns items left in the crafting grid.
void closeInventory(GLFWwindow* window);

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    // Handle right/left-button RELEASE first (before any early returns) so isBreaking always clears
    if ((button == GLFW_MOUSE_BUTTON_LEFT || button == GLFW_MOUSE_BUTTON_RIGHT) && action == GLFW_RELEASE) {
        isBreaking = false;
        // Don't reset breakHoldTime — progress pauses on release, resumes on re-press of same block
        return;
    }

    if (action != GLFW_PRESS) return;

    if (inventoryOpen) {
        if (button == GLFW_MOUSE_BUTTON_LEFT || button == GLFW_MOUSE_BUTTON_RIGHT) {
            bool isLeft = (button == GLFW_MOUSE_BUTTON_LEFT);
            bool isRight = (button == GLFW_MOUSE_BUTTON_RIGHT);
            bool isShift = (mods & GLFW_MOD_SHIFT) != 0;

            int screenW, screenH;
            glfwGetWindowSize(window, &screenW, &screenH);

            // Replicate centered panel layout math from hud.h
            float slotSize = 40.0f;
            float slotGap = 4.0f;
            float slotStep = slotSize + slotGap; // 44
            float totalW = HOTBAR_SIZE * slotSize + (HOTBAR_SIZE - 1) * slotGap; // 392

            float panelPad = 15.0f;
            float sectionGap = 14.0f;
            int craftDim = 3; // always 3x3
            float craftGridW = craftDim * slotStep - slotGap;
            float craftRowW = 3 * slotStep - slotGap;  // kept for recipe book compat
            float arrowW = 30.0f;
            float arrowGapL = 10.0f, arrowGapR = 10.0f;
            float outputW = slotSize;

            float panelW = panelPad + totalW + panelPad;
            float craftAreaH = craftDim * slotStep - slotGap;
            float panelH = panelPad + slotSize + sectionGap + 3 * slotStep - slotGap + sectionGap + craftAreaH + panelPad;
            float panelX = (screenW - panelW) / 2.0f;
            float panelY = (screenH - panelH) / 2.0f;
            float gridX = panelX + panelPad;

            float hotbarY = panelY + panelPad;
            float storageY = hotbarY + slotSize + sectionGap;
            float craftY = storageY + 3 * slotStep - slotGap + sectionGap;
            float craftTotalW = craftGridW + arrowGapL + arrowW + arrowGapR + outputW;
            float craftOffX = gridX + (totalW - craftTotalW) / 2.0f;

            // Mouse Y: GLFW top=0, HUD bottom=0
            float my = screenH - lastMouseY;
            float mx = lastMouseX;

            int clickedSlot = -1;

            // Check hotbar (slots 0-8)
            for (int i = 0; i < HOTBAR_SIZE; i++) {
                float sx = gridX + i * slotStep;
                float sy = hotbarY;
                if (mx >= sx && mx <= sx + slotSize && my >= sy && my <= sy + slotSize) {
                    clickedSlot = i; break;
                }
            }

            // Check storage grid (slots 9-35)
            if (clickedSlot == -1) {
                for (int r = 0; r < 3; r++) {
                    for (int c = 0; c < 9; c++) {
                        float sx = gridX + c * slotStep;
                        float sy = storageY + (2 - r) * slotStep; // row 0 at top
                        if (mx >= sx && mx <= sx + slotSize && my >= sy && my <= sy + slotSize) {
                            clickedSlot = 9 + r * 9 + c; break;
                        }
                    }
                    if (clickedSlot != -1) break;
                }
            }

            // Check crafting grid (2x2 or 3x3)
            if (clickedSlot == -1) {
                for (int r = 0; r < craftDim; r++) {
                    for (int c = 0; c < craftDim; c++) {
                        float sx = craftOffX + c * slotStep;
                        float sy = craftY + (craftDim - 1 - r) * slotStep;
                        if (mx >= sx && mx <= sx + slotSize && my >= sy && my <= sy + slotSize) {
                            clickedSlot = 36 + r * 3 + c; break;
                        }
                    }
                    if (clickedSlot != -1) break;
                }
            }

            // Check crafting output slot (slot 45)
            if (clickedSlot == -1) {
                float arrowX = craftOffX + craftGridW + arrowGapL;
                float outX = arrowX + arrowW + arrowGapR;
                float outY = craftY + (craftDim == 3 ? slotStep : slotSize * 0.5f);
                if (mx >= outX && mx <= outX + slotSize && my >= outY && my <= outY + slotSize) {
                    clickedSlot = 45;
                }
            }

            // Recipe Book toggle button (always available)
            float bookBtnX = craftOffX - 40.0f;
            float bookBtnY = craftY + slotStep;
            if (mx >= bookBtnX && mx <= bookBtnX + 24.0f && my >= bookBtnY && my <= bookBtnY + 24.0f) {
                recipeBookOpen = !recipeBookOpen;
                recipeSearchText = "";
                recipePage = 0;
                // Both panels occupy the strip left of the inventory, so opening
                // one closes the other rather than drawing them on top of each
                // other and hit-testing both.
                if (recipeBookOpen) buildTabOpen = false;
                printf("[RecipeBook] Toggled: %d\n", recipeBookOpen);
                return;
            }

            // Build tab toggle (plan_2 Step 4) — mirrors hud.h's layout exactly
            float buildBtnX = craftOffX - 40.0f;
            float buildBtnY = craftY + slotStep - 30.0f;
            if (mx >= buildBtnX && mx <= buildBtnX + 24.0f && my >= buildBtnY && my <= buildBtnY + 24.0f) {
                buildTabOpen = !buildTabOpen;
                if (buildTabOpen) recipeBookOpen = false;
                printf("[Build] Tab toggled: %d\n", buildTabOpen);
                return;
            }

            // Build recipe rows. Left click builds; the row is clickable even when
            // unaffordable so craftStructure() can say *why* it refused.
            if (buildTabOpen && isLeft) {
                float bpW = 180.0f;
                float bpX = panelX - bpW - 10.0f;
                float bpY = panelY;
                const float rowH = 58.0f;
                float rowTop = bpY + panelH - 48.0f;
                for (int i = 0; i < NUM_BUILD_RECIPES; i++) {
                    float ry = rowTop - (i + 1) * rowH;
                    if (mx >= bpX + 8.0f && mx <= bpX + bpW - 8.0f &&
                        my >= ry && my <= ry + rowH - 6.0f) {
                        // Close the inventory on success: the structure lands in
                        // front of the player, and there is no point showing it to
                        // someone staring at a menu.
                        if (craftStructure(i)) {
                            closeInventory(window);
                            firstMouse = true;   // or the view snaps on the next mouse move
                        }
                        return;
                    }
                }
            }

            // Recipe Book interactions
            if (recipeBookOpen) {
                float rbW = 180.0f;
                float rbH = panelH;
                float rbX = panelX - rbW - 10.0f;
                float rbY = panelY;

                // Build same filtered list as hud.h
                std::vector<int> filtered;
                for (int i = 0; i < NUM_RECIPES; i++) {
                    if (recipeSearchText.empty()) {
                        filtered.push_back(i);
                    } else {
                        std::string name = getBlockName(recipes[i].resultType);
                        bool found = false;
                        if (name.size() >= recipeSearchText.size()) {
                            for (size_t k = 0; k <= name.size() - recipeSearchText.size(); k++) {
                                bool match = true;
                                for (size_t m = 0; m < recipeSearchText.size(); m++) {
                                    if (tolower(name[k+m]) != recipeSearchText[m]) { match = false; break; }
                                }
                                if (match) { found = true; break; }
                            }
                        }
                        if (found) filtered.push_back(i);
                    }
                }
                int totalPages = ((int)filtered.size() + 3) / 4;

                // Pagination buttons
                float btnY = rbY + 15.0f;
                float btnW = 30.0f;
                float btnH = 20.0f;
                float prevX = rbX + 30.0f;
                float nextX = rbX + rbW - 30.0f - btnW;

                if (my >= btnY && my <= btnY + btnH) {
                    if (mx >= prevX && mx <= prevX + btnW) {
                        if (recipePage > 0) recipePage--;
                        return;
                    }
                    if (mx >= nextX && mx <= nextX + btnW) {
                        if (recipePage < totalPages - 1) recipePage++;
                        return;
                    }
                }

                // Recipe row clicking (uses filtered list)
                float sbY = rbY + rbH - 26.0f;
                int startIdx = recipePage * 4;
                float startY = sbY - 18.0f - 60.0f;
                for (int i = 0; i < 4; i++) {
                    int fi = startIdx + i;
                    if (fi >= (int)filtered.size()) break;
                    int rIdx = filtered[fi];
                    float rowY = startY - i * 70.0f;
                    if (rowY < rbY + 40.0f) break;
                    if (mx >= rbX + 10 && mx <= rbX + 10 + rbW - 20 && my >= rowY && my <= rowY + 60) {
                        autoCraftRecipe(rIdx);
                        return;
                    }
                }
            }


            // Handle clicks
            if (clickedSlot >= 0 && clickedSlot <= 35) {
                InventorySlot* target = &playerInventory[clickedSlot];
                if (isShift && isLeft) {
                    if (target->type != BLOCK_AIR) {
                        int startIdx = (clickedSlot < 9) ? 9 : 0;
                        int endIdx = (clickedSlot < 9) ? 36 : 9;
                        for (int k = startIdx; k < endIdx; k++) {
                            if (playerInventory[k].type == target->type && playerInventory[k].count < 64) {
                                int space = 64 - playerInventory[k].count;
                                int move = (target->count < space) ? target->count : space;
                                playerInventory[k].count += move;
                                target->count -= move;
                                if (target->count == 0) { target->type = BLOCK_AIR; break; }
                            }
                        }
                        if (target->count > 0) {
                            for (int k = startIdx; k < endIdx; k++) {
                                if (playerInventory[k].type == BLOCK_AIR) {
                                    playerInventory[k] = *target;
                                    target->type = BLOCK_AIR;
                                    target->count = 0;
                                    break;
                                }
                            }
                        }
                    }
                } else if (isRight) {
                    if (draggedSlot.type == BLOCK_AIR && target->type != BLOCK_AIR) {
                        int half = (target->count + 1) / 2;
                        draggedSlot.type = target->type;
                        draggedSlot.count = half;
                        target->count -= half;
                        if (target->count == 0) target->type = BLOCK_AIR;
                    } else if (draggedSlot.type != BLOCK_AIR) {
                        if (target->type == BLOCK_AIR) {
                            target->type = draggedSlot.type;
                            target->count = 1;
                            draggedSlot.count--;
                            if (draggedSlot.count == 0) draggedSlot.type = BLOCK_AIR;
                        } else if (target->type == draggedSlot.type && target->count < 64) {
                            target->count++;
                            draggedSlot.count--;
                            if (draggedSlot.count == 0) draggedSlot.type = BLOCK_AIR;
                        }
                    }
                } else if (isLeft) {
                    if (draggedSlot.type == target->type && draggedSlot.type != BLOCK_AIR && target->count < 64) {
                        int space = 64 - target->count;
                        int move = (draggedSlot.count < space) ? draggedSlot.count : space;
                        target->count += move;
                        draggedSlot.count -= move;
                        if (draggedSlot.count == 0) draggedSlot.type = BLOCK_AIR;
                    } else {
                        InventorySlot temp = *target;
                        *target = draggedSlot;
                        draggedSlot = temp;
                    }
                }
            } else if (clickedSlot >= 36 && clickedSlot <= 44) {
                int ci = clickedSlot - 36;
                InventorySlot* target = &craftingGrid[ci];
                if (isRight) {
                    if (draggedSlot.type == BLOCK_AIR && target->type != BLOCK_AIR) {
                        int half = (target->count + 1) / 2;
                        draggedSlot.type = target->type;
                        draggedSlot.count = half;
                        target->count -= half;
                        if (target->count == 0) target->type = BLOCK_AIR;
                    } else if (draggedSlot.type != BLOCK_AIR) {
                        if (target->type == BLOCK_AIR || (target->type == draggedSlot.type && target->count < 64)) {
                            target->type = draggedSlot.type;
                            target->count++;
                            draggedSlot.count--;
                            if (draggedSlot.count == 0) draggedSlot.type = BLOCK_AIR;
                        }
                    }
                } else if (isShift && isLeft) {
                    // Shift-click: move crafting grid item back to inventory
                    if (target->type != BLOCK_AIR) {
                        bool placed = false;
                        for (int k = 0; k < 36; k++) {
                            if (playerInventory[k].type == target->type && playerInventory[k].count + target->count <= 64) {
                                playerInventory[k].count += target->count;
                                placed = true; break;
                            }
                        }
                        if (!placed) {
                            for (int k = 0; k < 36; k++) {
                                if (playerInventory[k].type == BLOCK_AIR) {
                                    playerInventory[k] = *target;
                                    placed = true; break;
                                }
                            }
                        }
                        if (placed) { target->type = BLOCK_AIR; target->count = 0; }
                    }
                } else if (isLeft) {
                    if (draggedSlot.type == target->type && draggedSlot.type != BLOCK_AIR && target->count < 64) {
                        int space = 64 - target->count;
                        int move = (draggedSlot.count < space) ? draggedSlot.count : space;
                        target->count += move;
                        draggedSlot.count -= move;
                        if (draggedSlot.count == 0) draggedSlot.type = BLOCK_AIR;
                    } else {
                        InventorySlot temp = *target;
                        *target = draggedSlot;
                        draggedSlot = temp;
                    }
                }
                checkCraftingRecipes();
            } else if (clickedSlot == 45) {
                if (craftingOutput.type != BLOCK_AIR) {
                    if (isShift && isLeft) {
                        // Mass craft: keep crafting until inputs run out or inventory full
                        int outputType = craftingOutput.type;
                        int crafted = 0;
                        while (craftingOutput.type == outputType && craftingOutput.type != BLOCK_AIR) {
                            bool placed = false;
                            for (int k = 0; k < 36; k++) {
                                if (playerInventory[k].type == craftingOutput.type && playerInventory[k].count + craftingOutput.count <= 64) {
                                    playerInventory[k].count += craftingOutput.count;
                                    placed = true; break;
                                }
                            }
                            if (!placed) {
                                for (int k = 0; k < 36; k++) {
                                    if (playerInventory[k].type == BLOCK_AIR) {
                                        playerInventory[k] = craftingOutput;
                                        placed = true; break;
                                    }
                                }
                            }
                            if (!placed) break; // inventory full
                            consumeCraftingInputs(); // consumes inputs and re-checks recipe
                            crafted++;
                            if (crafted >= 64) break; // safety limit
                        }
                    } else if (isLeft) {
                        if (draggedSlot.type == BLOCK_AIR) {
                            draggedSlot = craftingOutput;
                            consumeCraftingInputs();
                        } else if (draggedSlot.type == craftingOutput.type && draggedSlot.count + craftingOutput.count <= 64) {
                            draggedSlot.count += craftingOutput.count;
                            consumeCraftingInputs();
                        }
                    }
                }
            }
        }
        return;
    }

    // Left-click = attack mob (instant) + place block
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            // Melee attack is still instant
            attackMob(camPos, camFront);
            // Sculpt tools take over left-click entirely: they act on the
            // targeted column, not on the empty cell in front of it, and they
            // are never consumed. placeBlock() would reject them anyway
            // (isItem), so this is a redirect rather than an override.
            int held = playerInventory[hotbarSlot].type;
            if (held == ITEM_TOOL_HILL || held == ITEM_TOOL_POND) {
                if (hasTarget) {
                    if (held == ITEM_TOOL_HILL) sculptHill(targetCol, targetRow);
                    else                        sculptPond(targetCol, targetRow);
                }
                return;
            }
            // Place block
            placeBlock();
        }
        return;
    }

    // Right-click = interact with special block OR hold-to-break
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (hasTarget) {
            int tbt = getBlock(targetCol, targetRow, targetHeight);
            BlockProperties tprops = getBlockProps(tbt);
            if (tbt == BLOCK_CRAFTING_TABLE) {
                // Open crafting/inventory UI with 3x3 grid
                inventoryOpen = true;
                useCraftingTable = true;
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                printf("[Block] Opened crafting table at (%d,%d,%d)\n",
                       targetCol, targetRow, targetHeight);
                return;
            }
            if (tprops.isInteractive) {
                // Toggle open/closed (bit 2 of state)
                uint16_t state = getBlockState(targetCol, targetRow, targetHeight);
                state ^= (1 << 2);
                setBlockState(targetCol, targetRow, targetHeight, state);
                printf("[Block] Toggled interactive block at (%d,%d,%d) -> %s\n",
                       targetCol, targetRow, targetHeight, (state >> 2) & 1 ? "open" : "closed");
                return;
            }
        }
        // Begin hold-to-break — only reset progress if targeting a different block
        isBreaking = true;
        if (targetCol != breakTargetCol || targetRow != breakTargetRow || targetHeight != breakTargetH) {
            breakHoldTime = 0.0f;
            breakTargetCol = targetCol;
            breakTargetRow = targetRow;
            breakTargetH   = targetHeight;
        }
        return;
    }

    // Middle click = pick block (copy targeted block to hotbar)
    if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
        if (hasTarget) {
            int picked = getBlock(targetCol, targetRow, targetHeight);
            if (picked != BLOCK_AIR) {
                // Check if already in hotbar — select it
                for (int i = 0; i < HOTBAR_SIZE; i++) {
                    if (playerInventory[i].type == picked) {
                        hotbarSlot = i;
                        printf("[PickBlock] Selected existing slot %d (%s)\n", i + 1, getBlockName(picked));
                        return;
                    }
                }
                // Otherwise put in current hotbar slot
                playerInventory[hotbarSlot] = {picked, 1};
                printf("[PickBlock] Picked %s into slot %d\n", getBlockName(picked), hotbarSlot + 1);
            }
        }
        return;
    }
}

void scrollCallback(GLFWwindow* window, double xoff, double yoff) {
    // Scroll always cycles hotbar
    // Hold Ctrl + Scroll for zoom
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS) {
        if (cameraMode == 0) {
            thirdPersonDist -= (float)yoff * 0.5f;
            if (thirdPersonDist < 1.5f) thirdPersonDist = 1.5f;
            if (thirdPersonDist > 10.0f) thirdPersonDist = 10.0f;
        } else if (cameraMode == 2) {
            camPos += camFront * (float)yoff * 1.5f;
        }
    } else {
        hotbarSlot = ((hotbarSlot - (int)yoff) % HOTBAR_SIZE + HOTBAR_SIZE) % HOTBAR_SIZE;
        printf("[Hotbar] Selected: slot %d (%s)\n", hotbarSlot + 1,
               getBlockName(playerInventory[hotbarSlot].type));
    }
}

// Typed character callback — feeds search box when recipe book is open
void charCallback(GLFWwindow* window, unsigned int codepoint) {
    if (!inventoryOpen || !recipeBookOpen) return;
    if (codepoint < 128) { // ASCII only
        recipeSearchText += (char)tolower((int)codepoint);
        recipePage = 0; // reset to first page of results
    }
}

void closeInventory(GLFWwindow* window) {
    inventoryOpen = false;
    recipeBookOpen = false;
    buildTabOpen = false;
    recipeSearchText = "";
    useCraftingTable = false;
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    // Return crafting grid items to inventory
    for (int ci = 0; ci < 9; ci++) {
        if (craftingGrid[ci].type != BLOCK_AIR) {
            bool placed = false;
            for (int j = 0; j < 36; j++) {
                if (playerInventory[j].type == craftingGrid[ci].type && playerInventory[j].count + craftingGrid[ci].count <= 64) {
                    playerInventory[j].count += craftingGrid[ci].count;
                    placed = true; break;
                }
            }
            if (!placed) {
                for (int j = 0; j < 36; j++) {
                    if (playerInventory[j].type == BLOCK_AIR) {
                        playerInventory[j] = craftingGrid[ci];
                        placed = true; break;
                    }
                }
            }
            if (!placed) {
                printf("[Inventory] Dropped %dx %s (inventory full)\n",
                       craftingGrid[ci].count, getBlockName(craftingGrid[ci].type));
            }
            craftingGrid[ci] = {BLOCK_AIR, 0};
        }
    }
    craftingOutput = {BLOCK_AIR, 0};
    // Return dragged item
    if (draggedSlot.type != BLOCK_AIR) {
        bool placed = false;
        for (int j = 0; j < 36; j++) {
            if (playerInventory[j].type == draggedSlot.type && playerInventory[j].count + draggedSlot.count <= 64) {
                playerInventory[j].count += draggedSlot.count;
                placed = true; break;
            }
        }
        if (!placed) {
            for (int j = 0; j < 36; j++) {
                if (playerInventory[j].type == BLOCK_AIR) {
                    playerInventory[j] = draggedSlot;
                    placed = true; break;
                }
            }
        }
        if (!placed) {
            printf("[Inventory] Dropped %dx %s (inventory full)\n",
                   draggedSlot.count, getBlockName(draggedSlot.type));
        }
        draggedSlot = {BLOCK_AIR, 0};
    }
    printf("[Inventory] Closed\n");
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action != GLFW_PRESS) return;

    // While inventory is open: only Escape and Backspace (for search) are allowed
    if (inventoryOpen) {
        if (key == GLFW_KEY_ESCAPE) {
            closeInventory(window);
        } else if (key == GLFW_KEY_BACKSPACE) {
            if (recipeBookOpen && !recipeSearchText.empty()) {
                recipeSearchText.pop_back();
                recipePage = 0;
            }
        }
        return; // block all other keys
    }

    switch (key) {
        case GLFW_KEY_ESCAPE:
            if (inventoryOpen) {
                closeInventory(window);
            } else {
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            }
            break;
        case GLFW_KEY_B:
            controlCar = !controlCar;
            if (controlCar) {
                printf("[Control] Now driving the car!\n");
            } else {
                printf("[Control] Back to player.\n");
            }
            break;
        // Rain. R is taken by fly-down.
        case GLFW_KEY_N:
            rainOn = !rainOn;
            printf("[Weather] Rain: %s\n", rainOn ? "ON" : "OFF");
            break;

        case GLFW_KEY_V:
            fourViewport = !fourViewport;
            printf("[Camera] 4-Viewport: %s\n", fourViewport ? "ON" : "OFF");
            break;
            
        // Inventory UI Toggle
        case GLFW_KEY_BACKSPACE:
            if (inventoryOpen && recipeBookOpen && !recipeSearchText.empty()) {
                recipeSearchText.pop_back();
                recipePage = 0;
            }
            break;

        case GLFW_KEY_I:
            if (inventoryOpen) {
                closeInventory(window);
            } else {
                inventoryOpen = true;
                useCraftingTable = false;
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                printf("[Inventory] Opened (3x3 crafting)\n");
            }
            break;

        // Lighting type toggles
        case GLFW_KEY_1:
            dirLightOn = !dirLightOn;
            printf("[Light] Directional: %s\n", dirLightOn ? "ON" : "OFF");
            break;
        case GLFW_KEY_2:
            pointLightOn = !pointLightOn;
            printf("[Light] Point lights: %s\n", pointLightOn ? "ON" : "OFF");
            break;
        case GLFW_KEY_3:
            spotLightOn = !spotLightOn;
            printf("[Light] Spot light: %s\n", spotLightOn ? "ON" : "OFF");
            break;

        // Lighting component toggles
        case GLFW_KEY_5:
            ambientOn = !ambientOn;
            printf("[Light] Ambient: %s\n", ambientOn ? "ON" : "OFF");
            break;
        case GLFW_KEY_6:
            diffuseOn = !diffuseOn;
            printf("[Light] Diffuse: %s\n", diffuseOn ? "ON" : "OFF");
            break;
        case GLFW_KEY_7:
            specularOn = !specularOn;
            printf("[Light] Specular: %s\n", specularOn ? "ON" : "OFF");
            break;

        // Master light toggle
        case GLFW_KEY_L:
            lightOn = !lightOn;
            printf("[Light] Master: %s\n", lightOn ? "ON" : "OFF");
            break;

        // Gouraud/Phong shading toggle
        case GLFW_KEY_H:
            useGouraud = !useGouraud;
            printf("[Shading] %s\n", useGouraud ? "Gouraud (per-vertex)" : "Phong (per-fragment)");
            break;

        // Day-night cycle
        case GLFW_KEY_T: {
            dayMode = (dayMode + 1) % 4;
            const char* names[] = {"Night", "Dawn", "Noon", "Dusk"};
            float factors[] = {0.0f, 0.4f, 1.0f, 0.3f};
            dayFactor = factors[dayMode];
            printf("[Time] %s (dayFactor=%.1f)\n", names[dayMode], dayFactor);
            break;
        }

        // Phase 19B A/B: baked tree VBOs vs. the old per-frame fractal walk
        case GLFW_KEY_F7:
            useBakedTrees = !useBakedTrees;
            printf("[Trees] Baked meshes %s\n", useBakedTrees ? "ON" : "OFF (live fractal)");
            break;

        // Grab / carry the targeted block. K because G is the fan and the
        // building keys (click, click, scroll) are all mouse.
        case GLFW_KEY_K:
            toggleCarry();
            break;

        // Interactive objects
        case GLFW_KEY_G:
            fanOn = !fanOn;
            printf("[Fan] %s\n", fanOn ? "ON" : "OFF");
            break;
        case GLFW_KEY_O:
            doorOpen = !doorOpen;
            printf("[Door] %s\n", doorOpen ? "Opening" : "Closing");
            break;

        // P key: respawn if dead, toggle windows if alive
        case GLFW_KEY_P:
            if (playerDead) {
                glm::vec3 spawnGrid = hexGridPos(3, 5, 0.0f);
                playerWorldPos = glm::vec3(spawnGrid.x, getGroundY(3, 5), spawnGrid.z);
                playerHealth = playerMaxHealth;
                playerStamina = playerMaxStamina;
                playerHunger = playerMaxHunger;
                playerVelY = 0.0f;
                playerOnGround = true;
                playerDead = false;
                trackingFall = false;
                printf("[Player] Respawned!\n");
            } else {
                windowOpen = !windowOpen;
                printf("[Window] %s\n", windowOpen ? "Opening" : "Closing");
            }
            break;

        // Camera mode cycle
        case GLFW_KEY_C:
            cameraMode = (cameraMode + 1) % 3;
            {
                const char* modeNames[] = {"Third-Person", "First-Person", "Free-Fly"};
                printf("[Camera] Mode: %s\n", modeNames[cameraMode]);
            }
            break;

        // Hotbar quick-select (keys 1-9 conflict with lighting, use F1-F9 or numpad)
        // Using KP_1..KP_9 (numpad)
        case GLFW_KEY_KP_1: hotbarSlot = 0; printf("[Hotbar] Slot 1\n"); break;
        case GLFW_KEY_KP_2: hotbarSlot = 1; printf("[Hotbar] Slot 2\n"); break;
        case GLFW_KEY_KP_3: hotbarSlot = 2; printf("[Hotbar] Slot 3\n"); break;
        case GLFW_KEY_KP_4: hotbarSlot = 3; printf("[Hotbar] Slot 4\n"); break;
        case GLFW_KEY_KP_5: hotbarSlot = 4; printf("[Hotbar] Slot 5\n"); break;
        case GLFW_KEY_KP_6: hotbarSlot = 5; printf("[Hotbar] Slot 6\n"); break;
        case GLFW_KEY_KP_7: hotbarSlot = 6; printf("[Hotbar] Slot 7\n"); break;
        case GLFW_KEY_KP_8: hotbarSlot = 7; printf("[Hotbar] Slot 8\n"); break;
        case GLFW_KEY_KP_9: hotbarSlot = 8; printf("[Hotbar] Slot 9\n"); break;
    }
}

